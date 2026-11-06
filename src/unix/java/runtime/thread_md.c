/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * POSIX threads for the HPI (sys_api.h).
 *
 * Every Java thread is on a list guarded by _queue_lock. The garbage
 * collector holds that lock while it stops the other threads
 * (sysThreadSingle), so no stopped thread can be part-way through
 * changing the list.
 *
 * POSIX cannot suspend another thread, so a thread is stopped by sending
 * it SIG_STOP_MD: the handler records its stack pointer, acknowledges, and
 * waits in sigsuspend() until its want_stop bits are clear and SIG_GO_MD
 * arrives. The interrupted registers are saved in the signal frame, which
 * lies on the thread's stack between the handler and the stack base, so
 * the GC's stack scan finds them.
 */
#ifdef __linux__
#define _GNU_SOURCE		/* pthread_getattr_np */
#endif
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define SIG_STOP_MD	SIGUSR1
#define SIG_GO_MD	SIGUSR2

#define STOP_GC		1	/* stopped by sysThreadSingle */
#define STOP_SUSPEND	2	/* Thread.suspend(), or not yet started */

long ProcStackSize = 0;		/* 0: the system's default stack size */

static pthread_key_t tls;
static int tls_ready;
static sys_thread_t *threads;	/* guarded by _queue_lock */
static int user_threads;	/* non-daemon threads on the list */

/* Guards want_stop and running; held by the GC while the world is stopped. */
static pthread_mutex_t stop_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t start_cv = PTHREAD_COND_INITIALIZER;
/* Orders stop_pending against waiting_on for Thread.stop(). */
static pthread_mutex_t post_lock = PTHREAD_MUTEX_INITIALIZER;

static void *stack_high(void)
{
#if defined(__APPLE__)
    return pthread_get_stackaddr_np(pthread_self());
#elif defined(__linux__)
    pthread_attr_t attr;
    void *addr = NULL;
    size_t size = 0;

    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
	pthread_attr_getstack(&attr, &addr, &size);
	pthread_attr_destroy(&attr);
    }
    return (char *) addr + size;
#else
#error "md/unix/thread_md.c: find the stack base of the current thread"
#endif
}

static sys_thread_t *new_thread(void)
{
    return (sys_thread_t *) calloc(1, sizeof(sys_thread_t));
}

/* Called holding _queue_lock. */
static void add_thread(sys_thread_t *t)
{
    t->next = threads;
    threads = t;
    if (!t->system)
	user_threads++;
}

/* Called holding _queue_lock. */
static void remove_thread(sys_thread_t *t)
{
    sys_thread_t **pp;

    for (pp = &threads; *pp != NULL; pp = &(*pp)->next) {
	if (*pp == t) {
	    *pp = t->next;
	    if (!t->system)
		user_threads--;
	    return;
	}
    }
}

/************************************************************************/
/* Stopping threads                                                     */

static void stop_handler(int sig)
{
    sys_thread_t *self = sysThreadSelf();
    int saved_errno = errno;
    sigset_t wait_mask;

    if (self == NULL)
	return;
    self->stack_ptr = (void *) &wait_mask;
    self->ack_seq = self->stop_seq;
    sigfillset(&wait_mask);
    sigdelset(&wait_mask, SIG_GO_MD);
    /* SIG_GO_MD is blocked here (sa_mask), so a release can't be missed. */
    while (self->want_stop) {
	/* A new stop may have been asked for while this one was ending. */
	self->ack_seq = self->stop_seq;
	sigsuspend(&wait_mask);
    }
    errno = saved_errno;
}

static void go_handler(int sig)
{
}

/*
 * Ask t to stop for reason bit and wait until it has. Called holding
 * stop_lock, for a running thread other than the caller.
 */
static void stop_thread(sys_thread_t *t, int bit)
{
    int was = t->want_stop;

    t->want_stop |= bit;
    if (!was) {
	t->stop_seq++;
	if (pthread_kill(t->handle, SIG_STOP_MD) != 0)
	    return;
    }
    /* Already stopping (maybe self-suspending): wait for that stop too. */
    while (t->ack_seq != t->stop_seq)
	sched_yield();
}

/* Called holding stop_lock. */
static void release_thread(sys_thread_t *t, int bit)
{
    if (!(t->want_stop & bit))
	return;
    t->want_stop &= ~bit;
    if (t->want_stop == 0)
	pthread_kill(t->handle, SIG_GO_MD);
}

static void install_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof sa);
    sa.sa_handler = stop_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIG_GO_MD);
    sigaction(SIG_STOP_MD, &sa, NULL);

    sa.sa_handler = go_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_GO_MD, &sa, NULL);
}

/************************************************************************/

/*
 * ASSUMED to be the first thing called: makes sysThreadSelf() work for the
 * primordial thread before the thread package is bootstrapped.
 */
void intrInit(void)
{
    sys_thread_t *t;

    if (tls_ready)
	return;
    pthread_key_create(&tls, NULL);
    tls_ready = 1;
    install_handlers();
    t = new_thread();
    t->handle = pthread_self();
    t->stack_base = stack_high();
    t->priority = NormalPriority;
    t->running = 1;
    pthread_setspecific(tls, t);
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
    while (user_threads > (self->system ? 0 : 1))
	QUEUE_WAIT();
    QUEUE_UNLOCK();
}

static void *start_thread(void *arg)
{
    sys_thread_t *t = (sys_thread_t *) arg;

    pthread_setspecific(tls, t);
    t->stack_base = stack_high();

    /* Created suspended; Thread.start() resumes it. */
    pthread_mutex_lock(&stop_lock);
    while (t->want_stop)
	pthread_cond_wait(&start_cv, &stop_lock);
    t->running = 1;
    pthread_mutex_unlock(&stop_lock);

    (*t->start)(t->cookie);
    sysThreadExit();		/* not reached: ThreadRT0 exits the thread */
    return NULL;
}

int sysThreadCreate(long stackSize, uint_t flags,
		    void *(*func)(void *),
		    sys_thread_t **tp,
		    void *cookie)
{
    sys_thread_t *t = new_thread();
    pthread_attr_t attr;
    int err;

    *tp = NULL;
    if (t == NULL)
	return SYS_NORESOURCE;
    t->start = func;
    t->cookie = cookie;
    t->system = (flags == THR_SYSTEM);
    t->priority = NormalPriority;
    t->want_stop = STOP_SUSPEND;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (stackSize > 0)
	pthread_attr_setstacksize(&attr, (size_t) stackSize);
    QUEUE_LOCK();
    err = pthread_create(&t->handle, &attr, start_thread, t);
    pthread_attr_destroy(&attr);
    if (err != 0) {
	QUEUE_UNLOCK();
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
    pthread_setspecific(tls, NULL);
    free(t);
    /* deviation: original leaves CRT free's unspecified EAX as int result. */
    return SYS_OK;
}

void sysThreadExit(void)
{
    sysThreadFree(sysThreadSelf());
    pthread_exit(NULL);
}

sys_thread_t *sysThreadSelf(void)
{
    return tls_ready ? (sys_thread_t *) pthread_getspecific(tls) : NULL;
}

/* Invocation API: register a host-owned thread without starting a new one. */
int sysThreadAlloc(sys_thread_t **result, stackp_t base, void *cookie)
{
    sys_thread_t *t = new_thread();
    if (!t) return SYS_NOMEM;
    t->handle = pthread_self();
    t->stack_base = base;
    t->cookie = cookie;
    t->priority = NormalPriority;
    t->running = 1;
    pthread_setspecific(tls, t);
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
    sched_yield();
}

int sysThreadSuspend(sys_thread_t *t)
{
    pthread_mutex_lock(&stop_lock);
    if (t == sysThreadSelf()) {
	t->want_stop |= STOP_SUSPEND;
	t->stop_seq++;		/* acknowledged when the signal lands */
	pthread_mutex_unlock(&stop_lock);
	pthread_kill(t->handle, SIG_STOP_MD);	/* returns once resumed */
	return SYS_OK;
    }
    if (t->running)
	stop_thread(t, STOP_SUSPEND);
    else
	t->want_stop |= STOP_SUSPEND;
    pthread_mutex_unlock(&stop_lock);
    return SYS_OK;
}

int sysThreadResume(sys_thread_t *t)
{
    pthread_mutex_lock(&stop_lock);
    if (t->running) {
	release_thread(t, STOP_SUSPEND);
    } else {
	t->want_stop &= ~STOP_SUSPEND;
	pthread_cond_broadcast(&start_cv);
    }
    pthread_mutex_unlock(&stop_lock);
    return SYS_OK;
}

/* Rhapsody's named sysThreadVMSuspend (5c145000), VMUnsuspend and
 * VMSuspendMe (5c14500c) are native-thread no-ops, as on primary Win32. */
int sysThreadVMSuspend(sys_thread_t *thread, sys_thread_t *self) { return 0; }
int sysThreadVMUnsuspend(sys_thread_t *thread) { return 0; }
void sysThreadVMSuspendMe(void) {}

/*
 * shortcut: the priority is only recorded; ordinary (SCHED_OTHER) threads
 * have no portable priority levels. Map onto nice values per platform if
 * Java priorities ever need to matter.
 */
int sysThreadSetPriority(sys_thread_t *t, int n)
{
    if (n < MinimumPriority || n > MaximumPriority)
	return SYS_ERR;
    t->priority = n;
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
    if (t == sysThreadSelf()) {
	void *volatile sp = (void *) &t;	/* GCC folds a plain &t return to NULL */
	return sp;
    }
    return t->stack_ptr;
}

stackp_t sysThreadStackBase(sys_thread_t *t)
{
    return (stackp_t) t->stack_base;
}

/* A stopped thread's registers are in its signal frame, on the scanned stack. */
void **sysThreadRegisters(sys_thread_t *t, int *count)
{
    *count = 0;
    return NULL;
}

/*
 * Stop every other Java thread for garbage collection. Called holding
 * _queue_lock (and the heap lock), so the list cannot change; stop_lock
 * is held until sysThreadMulti so no thread starts in between.
 */
int sysThreadSingle(void)
{
    sys_thread_t *self = sysThreadSelf(), *t;

    sysAssert(QUEUE_LOCKED());
    pthread_mutex_lock(&stop_lock);
    for (t = threads; t != NULL; t = t->next) {
	if (t != self && t->running)
	    stop_thread(t, STOP_GC);
    }
    return SYS_OK;
}

void sysThreadMulti(void)
{
    sys_thread_t *self = sysThreadSelf(), *t;

    for (t = threads; t != NULL; t = t->next) {
	if (t != self && t->running)
	    release_thread(t, STOP_GC);
    }
    pthread_mutex_unlock(&stop_lock);
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

/*
 * Called by sysMonitorWait before it checks stop_pending: records the
 * monitor t waits on (NULL when done) and returns stop_pending.
 */
int sysThreadSetWaitingOn(sys_thread_t *t, sys_mon_t *mon)
{
    int pending;

    pthread_mutex_lock(&post_lock);
    t->waiting_on = mon;
    pending = t->stop_pending;
    pthread_mutex_unlock(&post_lock);
    return pending;
}

/* Thread.stop(): make the exception pending and wake the thread if it waits. */
void sysThreadPostException(sys_thread_t *t, void *exc)
{
    TID tid = (TID) t->cookie;
    ExecEnv *ee = tid ? (ExecEnv *) THREAD(tid)->eetop : NULL;
    sys_mon_t *mon;
    extern void sysMonitorInterrupt(sys_mon_t *);

    if (ee != NULL)
	exceptionThrow(ee, (JHandle *) exc);
    pthread_mutex_lock(&post_lock);
    t->stop_pending = 1;
    mon = t->waiting_on;
    pthread_mutex_unlock(&post_lock);
    if (mon != NULL)
	sysMonitorInterrupt(mon);
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

/* deviation: preserve the pthreads port, using its post lock and wake handshake
 * for the JDK 1.1.5 interruption shadow instead of a Win32 event. */
void sysThreadInterrupt(sys_thread_t *t)
{
    sys_mon_t *mon;
    extern void sysMonitorInterrupt(sys_mon_t *);

    pthread_mutex_lock(&post_lock);
    t->interrupted = 1;
    mon = t->waiting_on;
    pthread_mutex_unlock(&post_lock);
    if (mon != NULL)
	sysMonitorInterrupt(mon);
}

int sysThreadIsInterrupted(sys_thread_t *t, long clear)
{
    int interrupted;

    pthread_mutex_lock(&post_lock);
    interrupted = t->interrupted;
    if (clear == TRUE)
	t->interrupted = 0;
    pthread_mutex_unlock(&post_lock);
    return interrupted;
}

/* POSIX host adaptation of the original invocation attachment mutex. */
static pthread_mutex_t attach_lock = PTHREAD_MUTEX_INITIALIZER;
void sysAttachThreadLock(void) { pthread_mutex_lock(&attach_lock); }
void sysAttachThreadUnlock(void) { pthread_mutex_unlock(&attach_lock); }
