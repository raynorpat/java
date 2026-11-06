/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * Win32 native threads for the HPI (sys_api.h).
 *
 * Every Java thread is on a list guarded by _queue_lock. The garbage
 * collector holds that lock while it stops the other threads
 * (sysThreadSingle), so no stopped thread can be part-way through
 * changing the list.
 */
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys_api.h"
#include "threads_md.h"
#include "monitor_md.h"
#include "threads.h"
#include "interpreter.h"	/* obj_monitor, ExecEnv */
#include "monitor.h"
#include "monitor_cache.h"
#include "exceptions.h"
#include "finalize.h"
#include "javaString.h"

long ProcStackSize = 0;		/* 0: the executable's default stack size */

static DWORD tls = TLS_OUT_OF_INDEXES;
static sys_thread_t *threads;	/* guarded by _queue_lock */
static int user_threads;	/* non-daemon threads on the list */
bool_t ThreadsInitialized;

static void *stack_high(void)
{
    ULONG_PTR low, high;
    GetCurrentThreadStackLimits(&low, &high);
    return (void *) high;
}

static sys_thread_t *new_thread(void)
{
    sys_thread_t *t = (sys_thread_t *) calloc(1, sizeof(sys_thread_t));

    if (t != NULL) {
	t->interrupt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (t->interrupt_event == NULL) {
	    free(t);
	    return NULL;
	}
    }
    return t;
}

/* Called holding _queue_lock. */
static void add_thread(sys_thread_t *t)
{
    t->next = threads;
    threads = t;
    if (!t->system_thread)
	user_threads++;
}

/* Called holding _queue_lock. */
static void remove_thread(sys_thread_t *t)
{
    sys_thread_t **pp;

    for (pp = &threads; *pp != NULL; pp = &(*pp)->next) {
	if (*pp == t) {
	    *pp = t->next;
	    if (!t->system_thread)
		user_threads--;
	    return;
	}
    }
}

/*
 * ASSUMED to be the first thing called: makes sysThreadSelf() work for the
 * primordial thread before the thread package is bootstrapped.
 */
void intrInit(void)
{
    sys_thread_t *t;

    if (tls != TLS_OUT_OF_INDEXES)
	return;
    tls = TlsAlloc();
    t = new_thread();
    if (tls == TLS_OUT_OF_INDEXES || t == NULL) {
	fprintf(stderr, "Unable to initialize primordial thread\n");
	exit(1);
    }
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
		    &t->handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
    t->id = GetCurrentThreadId();
    t->stack_base = stack_high();
    t->priority = NormalPriority;
    TlsSetValue(tls, t);
    ThreadsInitialized = TRUE;
}

int sysThreadBootstrap(sys_thread_t **tp, void *cookie)
{
    sys_thread_t *t = sysThreadSelf();

    t->cookie = cookie;
    QUEUE_LOCK();
    add_thread(t);
    QUEUE_UNLOCK();
    *tp = t;
    return SYS_OK;
}

void sysThreadInitializeSystemThreads(void)
{
    InitializeFinalizerThread();
}

/*
 * The primordial thread may not exit until all user (non-daemon)
 * threads have.
 */
void WaitToDie(void)
{
    sys_thread_t *self = sysThreadSelf();

    QUEUE_LOCK();
    while (user_threads > (self->system_thread ? 0 : 1))
	QUEUE_WAIT();
    QUEUE_UNLOCK();
}

static unsigned __stdcall start_thread(void *arg)
{
    sys_thread_t *t = (sys_thread_t *) arg;

    TlsSetValue(tls, t);
    t->stack_base = stack_high();
    (*t->start_proc)(t->start_parm);
    sysThreadExit();		/* not reached: ThreadRT0 exits the thread */
    return 0;
}

int sysThreadCreate(long stackSize, uint_t flags,
		    void *(*func)(void *),
		    sys_thread_t **tp,
		    void *cookie)
{
    sys_thread_t *t = new_thread();

    *tp = NULL;
    if (t == NULL)
	return SYS_NORESOURCE;
    t->start_proc = func;
    t->start_parm = cookie;
    t->cookie = cookie;
    t->system_thread = (flags == THR_SYSTEM);
    t->priority = NormalPriority;

    /* Created suspended; Thread.start() resumes it. */
    QUEUE_LOCK();
    t->handle = (void *) _beginthreadex(NULL, (unsigned) stackSize, start_thread,
					t, CREATE_SUSPENDED, (unsigned *) &t->id);
    if (t->handle == NULL) {
	QUEUE_UNLOCK();
	CloseHandle(t->interrupt_event);
	free(t);
	return SYS_NORESOURCE;
    }
    add_thread(t);
    QUEUE_UNLOCK();

    *tp = t;
    return SYS_OK;
}

/* Primary 1004cbc5: detach Java state before removing the host thread. */
int sysThreadFree(sys_thread_t *t)
{
    JHandle *p = (JHandle *)t->cookie;
    ExecEnv *eep = NULL;
    monitor_t *mon;
    if (p) {
        eep = (ExecEnv *)THREAD(p)->eetop;
        THREAD(p)->PrivateInfo = 0;
        THREAD(p)->eetop = 0;
        mon = lookupMonitor(obj_monitor(p));
        if (mon && sysMonitorInUse(&sysmon(mon))) {
            sysMonitorEnter(&sysmon(mon));
            sysMonitorNotifyAll(&sysmon(mon));
            sysMonitorExit(&sysmon(mon));
        }
    }
    QUEUE_LOCK();
    remove_thread(t);
    QUEUE_NOTIFY();
    QUEUE_UNLOCK();
    if (eep) DeleteExecEnv(eep);
    TlsSetValue(tls, NULL);
    CloseHandle(t->handle);
    CloseHandle(t->interrupt_event);
    free(t);
    /* deviation: original leaves CRT free's unspecified EAX as int result. */
    return SYS_OK;
}

void sysThreadExit(void)
{
    sysThreadFree(sysThreadSelf());
    _endthreadex(0);
}

sys_thread_t *sysThreadSelf(void)
{
    return tls == TLS_OUT_OF_INDEXES ? NULL : (sys_thread_t *) TlsGetValue(tls);
}

/* Invocation API: register a host-owned thread without starting a new one. */
int sysThreadAlloc(sys_thread_t **result, stackp_t base, void *cookie)
{
    sys_thread_t *t = new_thread();
    if (!t) return SYS_NOMEM;
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
                         &t->handle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        CloseHandle(t->interrupt_event); free(t); return SYS_NORESOURCE;
    }
    t->id = GetCurrentThreadId();
    t->stack_base = base;
    t->cookie = cookie;
    t->priority = NormalPriority;
    TlsSetValue(tls, t);
    QUEUE_LOCK(); add_thread(t); QUEUE_UNLOCK();
    *result = t;
    return SYS_OK;
}
/* Compatibility for JNI attachment, which creates its Java cookie later. */
int jni_AttachHostThread(sys_thread_t **result)
{
    return sysThreadAlloc(result, stack_high(), NULL);
}
void jni_DetachHostThread(void)
{
    sysThreadFree(sysThreadSelf());
}

void sysThreadSetStackBase(sys_thread_t *thread, stackp_t base)
{
    thread->stack_base = base;
}

void sysThreadYield(void)
{
    SwitchToThread();
}

int sysThreadSuspend(sys_thread_t *t)
{
    return SuspendThread(t->handle) == (DWORD) -1 ? SYS_ERR : SYS_OK;
}

int sysThreadResume(sys_thread_t *t)
{
    return ResumeThread(t->handle) == (DWORD) -1 ? SYS_ERR : SYS_OK;
}

/* Primary javai_g.dll 1004cdaa/1004cdc7/1004cdbc contain only return 0,
 * return 0, and return respectively. These are distinct from Thread.suspend. */
int sysThreadVMSuspend(sys_thread_t *thread, sys_thread_t *self) { return 0; }
int sysThreadVMUnsuspend(sys_thread_t *thread) { return 0; }
void sysThreadVMSuspendMe(void) {}

int sysThreadSetPriority(sys_thread_t *t, int n)
{
    /* Java 1..10, 5 is normal */
    static const int win32[] = {
	THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_LOWEST,
	THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_BELOW_NORMAL,
	THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL,
	THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_HIGHEST
    };

    if (n < MinimumPriority || n > MaximumPriority)
	return SYS_ERR;
    t->priority = n;
    SetThreadPriority(t->handle, win32[n - 1]);
    return SYS_OK;
}

int sysThreadGetPriority(sys_thread_t *t, int *pri)
{
    *pri = t->priority;
    return SYS_OK;
}

/* Valid for the calling thread and for threads stopped by sysThreadSingle. */
void *sysThreadStackPointer(sys_thread_t *t)
{
    if (t == sysThreadSelf())
	return (void *) &t;
    return t->stack_ptr;
}

stackp_t sysThreadStackBase(sys_thread_t *t)
{
    return (stackp_t) t->stack_base;
}

void **sysThreadRegisters(sys_thread_t *t, int *count)
{
    *count = t->stack_ptr ? sizeof(t->regs) / sizeof(t->regs[0]) : 0;
    return t->regs;
}

/*
 * Stop every other Java thread for garbage collection. Called holding
 * _queue_lock (and the heap lock), so the list cannot change.
 */
int sysThreadSingle(void)
{
    sys_thread_t *self = sysThreadSelf(), *t;
    CONTEXT ctx;

    sysAssert(QUEUE_LOCKED());
    for (t = threads; t != NULL; t = t->next) {
	if (t == self)
	    continue;
	if (SuspendThread(t->handle) == (DWORD) -1) {
	    t->stack_ptr = NULL;
	    continue;
	}
	/* GetThreadContext waits until the thread is really stopped. */
	ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	if (!GetThreadContext(t->handle, &ctx)) {
	    ResumeThread(t->handle);
	    t->stack_ptr = NULL;
	    continue;
	}
	t->stack_ptr = (void *) ctx.Esp;
	t->regs[0] = (void *) ctx.Eax;
	t->regs[1] = (void *) ctx.Ebx;
	t->regs[2] = (void *) ctx.Ecx;
	t->regs[3] = (void *) ctx.Edx;
	t->regs[4] = (void *) ctx.Esi;
	t->regs[5] = (void *) ctx.Edi;
	t->regs[6] = (void *) ctx.Ebp;
    }
    return SYS_OK;
}

void sysThreadMulti(void)
{
    sys_thread_t *self = sysThreadSelf(), *t;

    for (t = threads; t != NULL; t = t->next) {
	if (t != self && t->stack_ptr != NULL) {
	    t->stack_ptr = NULL;
	    ResumeThread(t->handle);
	}
    }
}

int sysThreadEnumerateOver(int (*func)(sys_thread_t *, void *), void *arg)
{
    sys_thread_t *t;
    int ret = SYS_OK;

    QUEUE_LOCK();
    for (t = threads; t != NULL && ret == SYS_OK; t = t->next) {
	if (t->cookie != NULL)		/* not yet a Java thread */
	    ret = (*func)(t, arg);
    }
    QUEUE_UNLOCK();
    return ret;
}

void sysThreadSetBackPtr(sys_thread_t *t, void *cookie)
{
    t->cookie = cookie;
}

void *sysThreadGetBackPtr(sys_thread_t *t)
{
    return t->cookie;
}

void sysThreadInit(sys_thread_t *t, stackp_t sb)
{
    /* The stack base is the thread's real stack top, set when it starts. */
}

int sysThreadCheckStack(void)
{
    return 1;
}

int sysInterruptsPending(void)
{
    return 0;
}

/* Thread.stop(): make the exception pending and wake the thread if it waits. */
void sysThreadPostException(sys_thread_t *t, void *exc)
{
    TID tid = (TID) t->cookie;
    ExecEnv *ee = tid ? (ExecEnv *) THREAD(tid)->eetop : NULL;

    if (ee != NULL)
	exceptionThrow(ee, (JHandle *) exc);
    SetEvent(t->interrupt_event);
}

JAVA_PUBLIC_API(void)
sysThreadDumpInfo(sys_thread_t *t)
{
    FILE *out = stderr;
    TID tid = (TID) sysThreadGetBackPtr(t);

    if (tid) {
	JavaFrame *frame;
	struct Classjava_lang_Thread *thread = THREAD(tid);
	ExecEnv *ee = (ExecEnv *) thread->eetop;
	struct Hjava_lang_ThreadGroup *group = thread->group;

	if (group) {
	    char name[256];
	    javaString2CString(unhand(group)->name, name, sizeof(name));
	    fprintf(out, "    in group %s\n", name);
	} else {
	    fprintf(out, "    no group\n");
	}
	if (ee) {
	    for (frame = ee->current_frame; frame != NULL; frame = frame->prev) {
		char where[256];
		pc2string(frame->lastpc, 0, where, where + sizeof(where));
		fprintf(out, "\t%s\n", where);
	    }
	}
    }
}

/* Reconstructed from javai_g.dll 0x1004d404 and 0x1004d426. */
void sysThreadInterrupt(sys_thread_t *t)
{
    t->interrupted = TRUE;
    SetEvent(t->interrupt_event);
}

int sysThreadIsInterrupted(sys_thread_t *t, long clear)
{
    int interrupted = t->interrupted;

    if (clear == TRUE) {
	t->interrupted = FALSE;
	ResetEvent(t->interrupt_event);
    }
    return interrupted;
}

/* Primary 10002925/10002962; called under invocation initialization. */
static CRITICAL_SECTION attach_lock;
static int attach_lock_initialized;
void sysAttachThreadLock(void)
{
    if (!attach_lock_initialized) {
        InitializeCriticalSection(&attach_lock);
        attach_lock_initialized = 1;
    }
    EnterCriticalSection(&attach_lock);
}
void sysAttachThreadUnlock(void) { LeaveCriticalSection(&attach_lock); }
