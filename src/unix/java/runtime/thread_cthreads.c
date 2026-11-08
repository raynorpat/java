/* Reconstructed from Rhapsody DR2 libjava.A.dylib.
 * Allocation: 5C144970; init: 5C144CFC; self/enumeration: 5C1451FC-5C14529C.
 * Selected explicitly with JDK115_UNIX_THREADS=cthreads.
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mach/mach.h>
#include <mach/thread_info.h>
#include <mach/thread_status.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "java_lang_Thread.h"
#include "monitor_cache.h"
#include "finalize.h"
#include "cthreads/runtime.h"

/* Shared VM thread-queue state, declared in threads.h. */
extern sys_mon_t *_queue_lock;
extern int ActiveThreadCount;
extern int UserThreadCount;
extern stackp_t mainstktop;
static sys_thread_t *threads;
static sys_thread_t *primordial_pending;
long ProcStackSize;

/* DR2 5c138064/5c13807c: use the supplied JNI environment's Java thread. */
void *get_awt_cookie(void *env)
{
    Hjava_lang_Thread *thread = (Hjava_lang_Thread *)JNIEnv2EE(env)->thread;
    return ((sys_thread_t *)unhand(thread)->PrivateInfo)->awt_cookie;
}

void set_awt_cookie(void *env, void *cookie)
{
    Hjava_lang_Thread *thread = (Hjava_lang_Thread *)JNIEnv2EE(env)->thread;
    ((sys_thread_t *)unhand(thread)->PrivateInfo)->awt_cookie = cookie;
}

static void register_thread(sys_thread_t *t)
{
    sysMonitorEnter(_queue_lock);
    ++ActiveThreadCount;
    t->next = threads; threads = t;
    if (!t->system_thread) ++UserThreadCount;
    sysMonitorExit(_queue_lock);
}

/* 5C144B78: the child cannot enter Java until the creator releases this gate. */
static void *thread_start_func(void *arg)
{
    sys_thread_t *t = (sys_thread_t *)arg;
    if (!mutex_try_lock(&t->start_mutex)) mutex_wait_lock(&t->start_mutex);
    t->start_mutex.lock = 0;
    /* Startup rollback releases the child without calling Java. */
    if (t->start_func == NULL) return NULL;
    return t->start_func(t->start_arg);
}

/* 5C144BB4. cthread_fork uses its own stack size; the original ignores size.
 * Deviation: delay detach until startup succeeds, so a failed suspension can
 * release and join the unpublished child before freeing its start argument.
 */
int sysThreadCreate(long size, unsigned int flags, void *(*func)(void *),
                    sys_thread_t **result, void *cookie)
{
    sys_thread_t *t;
    thread_t kernel_thread;
    int objc_thread = 0;
    (void)size;
    t = (sys_thread_t *)malloc(sizeof(*t));
    if (t == NULL) return SYS_NOMEM;
    memset(t, 0, sizeof(*t));
    t->cookie = cookie;
    t->state = SUSPENDED;
    t->system_thread = flags != 1;
    t->run_static_initializers = TRUE;
    if (!mutex_try_lock(&t->start_mutex)) mutex_wait_lock(&t->start_mutex);
    t->start_func = func;
    t->start_arg = cookie;
    t->cthread = cthread_fork(thread_start_func, t);
    if (t->cthread == NULL) {
        free(t);
        return SYS_NORESOURCE;
    }
    do {
        /* DR2 cthread real_thread is at byte4, independently of SDK layout. */
        memcpy(&kernel_thread, (char *)t->cthread + 4, sizeof(kernel_thread));
        if (kernel_thread == 0) cthread_yield();
    } while (kernel_thread == 0);
    if (thread_suspend(kernel_thread) != 0) {
        t->start_func = NULL;
        t->start_mutex.lock = 0;
        cthread_join(t->cthread);
        free(t);
        return SYS_ERR;
    }
    t->start_mutex.lock = 0;
    memcpy((char *)t->cthread + 124, &objc_thread, sizeof(objc_thread));
    memcpy((char *)t->cthread + 116, &t, sizeof(t));
    register_thread(t);
    cthread_detach(t->cthread);
    *result = t;
    return SYS_OK;
}

int sysThreadAlloc(sys_thread_t **result, stackp_t base, void *cookie)
{
    sys_thread_t *t = (sys_thread_t *)malloc(sizeof(sys_thread_t));
    if (t == NULL) return SYS_NOMEM;
    memset(t, 0, sizeof(*t));
    t->cookie = cookie;
    t->cthread = cthread_self();
    t->state = RUNNABLE;
    t->stack_base = base;
    t->run_static_initializers = TRUE;
    /* DR2 libjava accesses TLS at byte 116. The available external SDK
     * describes a different cthread ABI (data at 124); do not use its macro.
     * Byte copies avoid imposing an invented full cthread record/type.
     */
    memcpy((char *)t->cthread + 116, &t, sizeof(t));

    register_thread(t);
    *result = t;
    return SYS_OK;
}

/* 5C144A38, VM portion. Foundation's NSThread initialization belongs to the
 * later AppKit integration, not this Sun VM's C platform backend. */
int sysThreadBootstrap(sys_thread_t **result, void *cookie)
{
    int status;
    if (primordial_pending != NULL && primordial_pending == sysThreadSelf()) {
        *result = primordial_pending; primordial_pending = NULL;
        (*result)->cookie = cookie;
        (*result)->stack_base = mainstktop;
        register_thread(*result);
        status = SYS_OK;
    } else status = sysThreadAlloc(result, mainstktop, cookie);
    if (status == SYS_OK) (*result)->primordial_thread = TRUE;
    return status;
}

sys_thread_t *sysThreadSelf(void)
{
    sys_thread_t *t;
    ur_cthread_t proc = ur_cthread_self();
    if (proc == NULL || proc->incarnation == NULL) return NULL;
    memcpy(&t, (char *)proc->incarnation + 116, sizeof(t));
    return t;
}

/* 5C144D0C: detach Java state, unregister, wake joiners, then release TLS.
 * Preserve the existing attachment fix: a native thread may have no Java
 * cookie. The original DR2 path unconditionally dereferences that cookie.
 */
int sysThreadFree(sys_thread_t *t)
{
    Hjava_lang_Thread *h = (Hjava_lang_Thread *)t->cookie;
    ExecEnv *ee = NULL;
    sys_thread_t **link;
    sys_thread_t *empty = NULL;
    monitor_t *mon;

    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    if (h != NULL) {
        ee = (ExecEnv *)unhand(h)->eetop;
        unhand(h)->PrivateInfo = 0;
        unhand(h)->eetop = 0;
    }
    t->mutex.lock = 0;
    sysMonitorEnter(_queue_lock);
    --ActiveThreadCount;
    if (!t->system_thread) {
        --UserThreadCount;
        sysMonitorNotify(_queue_lock);
    }
    for (link = &threads; *link != t; link = &(*link)->next) {}
    *link = t->next;
    sysMonitorExit(_queue_lock);
    t->next = NULL;
    if (ee != NULL) DeleteExecEnv(ee);

    if (h != NULL) {
        mon = lookupMonitor((unsigned int)h);
        if (mon != NULL && sysMonitorInUse(&sysmon(mon))) {
            sysMonitorEnter(&sysmon(mon));
            if (sysmon(mon).mwait_queue.count > 0)
                sysMonitorNotifyAll(&sysmon(mon));
            sysMonitorExit(&sysmon(mon));
        }
    }
    t->cacheKey = 0;
    memcpy((char *)t->cthread + 116, &empty, sizeof(empty));
    free(t);
    return SYS_OK;
}

/* Native cthreads exit path from 5C144E24. The AppKit-owned NSThread path
 * belongs to the later Objective-C integration. In particular, do not copy
 * the original's read of t->cthread after sysThreadFree has released t. */
void sysThreadExit(void)
{
    sysThreadFree(sysThreadSelf());
    cthread_exit(NULL);
}

static thread_t mach_thread(sys_thread_t *t)
{
    thread_t port;
    memcpy(&port, (char *)t->cthread + 4, sizeof(port));
    return port;
}

/* GC callers hold _queue_lock across Single, root scanning and Multi.
 * Side storage preserves the original 192-byte thread ABI and its exception
 * jump buffer. Allocate before suspending anyone: malloc may need their locks.
 */
struct gc_thread_snapshot {
    sys_thread_t *thread;
    void *registers[i386_THREAD_STATE_COUNT];
};
static struct gc_thread_snapshot *gc_snapshots;
static int gc_stopped;

/* 5C145440, with rollback and register capture required by the shared GC. */
int sysThreadSingle(void)
{
    sys_thread_t *self = sysThreadSelf(), *t;
    i386_thread_state_t state;
    unsigned int count;
    int n = 0, result = SYS_OK;
    sysMonitorEnter(_queue_lock);
    for (t = threads; t != NULL; t = t->next) if (t != self) ++n;
    if (n != 0) {
        gc_snapshots = (struct gc_thread_snapshot *)calloc(n, sizeof(*gc_snapshots));
        if (gc_snapshots == NULL) {
            sysMonitorExit(_queue_lock);
            return SYS_NOMEM;
        }
    }
    for (t = threads; t != NULL; t = t->next) {
        if (t == self) continue;
        if (thread_suspend(mach_thread(t)) != 0) { result = SYS_ERR; break; }
        gc_snapshots[gc_stopped].thread = t;
        ++gc_stopped; /* Include this thread in rollback if capture fails. */
        count = i386_THREAD_STATE_COUNT;
        if (thread_get_state(mach_thread(t), i386_THREAD_STATE,
                             (thread_state_t)&state, &count) != 0 ||
            count != i386_THREAD_STATE_COUNT) {
            result = SYS_ERR;
            break;
        }
        memcpy(gc_snapshots[gc_stopped-1].registers, &state, sizeof(state));
    }
    sysMonitorExit(_queue_lock);
    if (result != SYS_OK) sysThreadMulti();
    return result;
}

/* 5C1454B0 stopped on the first error. Always attempt every owned suspension;
 * a resume failure cannot safely return to a VM with stranded threads. */
void sysThreadMulti(void)
{
    int i, failed = 0;
    if (gc_snapshots == NULL) return;
    sysMonitorEnter(_queue_lock);
    for (i = 0; i < gc_stopped; ++i) {
        if (thread_resume(mach_thread(gc_snapshots[i].thread)) != 0) failed = 1;
    }
    sysMonitorExit(_queue_lock);
    if (failed) {
        sysAbort();
        abort(); /* A user-supplied JNI abort hook is allowed to return. */
    }
    free(gc_snapshots);
    gc_snapshots = NULL;
    gc_stopped = 0;
}

void **sysThreadRegisters(sys_thread_t *t, int *count)
{
    int i;
    *count = 0;
    for (i = 0; i < gc_stopped; ++i) {
        if (gc_snapshots[i].thread == t) {
            *count = i386_THREAD_STATE_COUNT;
            return gc_snapshots[i].registers;
        }
    }
    return NULL;
}

void *sysThreadStackPointer(sys_thread_t *t)
{
    void **registers;
    int count;
    /* Preserve the original sentinel: shared gc.c replaces self's SP with
     * its own frame, and gc_locked spills self's registers using setjmp. */
    if (t == sysThreadSelf()) return (void *)1;
    registers = sysThreadRegisters(t, &count);
    return registers != NULL ? registers[7] : NULL; /* i386 esp */
}

/* 5C145020. These states are hints (see the original threads_md.h).
 * Match the original update-before-Mach-call ordering, including on error. */
int sysThreadSuspend(sys_thread_t *t)
{
    if (t == sysThreadSelf()) {
        t->state = SUSPENDED;
    } else {
        if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
        switch (t->state) {
        case RUNNABLE:
        case RUNNING:
            t->state = SUSPENDED;
            break;
        case RUNNING_AT_BREAKPOINT:
            t->state = SUSPENDED_AT_BREAKPOINT;
            break;
        case MONITOR_WAIT:
            t->state = SUSPENDED;
            t->mpending_suspend = TRUE;
            break;
        case CONDVAR_WAIT:
            t->state = SUSPENDED;
            t->cpending_suspend = TRUE;
            break;
        case SUSPENDED:
        case MONITOR_SUSPENDED:
        case SUSPENDED_AT_BREAKPOINT:
            t->mutex.lock = 0;
            return SYS_ERR;
        default:
            break;
        }
        t->mutex.lock = 0;
    }
    return thread_suspend(mach_thread(t)) == 0 ? SYS_OK : SYS_ERR;
}

/* 5C145160: pending monitor/condition suspension takes precedence. */
int sysThreadResume(sys_thread_t *t)
{
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    if (t->mpending_suspend) {
        t->mpending_suspend = FALSE;
        t->state = MONITOR_WAIT;
    } else if (t->cpending_suspend) {
        t->cpending_suspend = FALSE;
        t->state = CONDVAR_WAIT;
    } else if (t->state == MONITOR_SUSPENDED) {
        t->state = MONITOR_WAIT;
    } else if (t->state == SUSPENDED || t->state == SUSPENDED_AT_BREAKPOINT) {
        t->state = RUNNING;
    }
    t->mutex.lock = 0;
    return thread_resume(mach_thread(t)) == 0 ? SYS_OK : SYS_ERR;
}

/* 5C144F9C/5C144FDC query the current Mach priority, not base_priority.
 * Deviation: use the shared HPI status convention and do not publish an
 * uninitialized stack word when thread_info fails, as the binary does. */
int sysThreadGetPriority(sys_thread_t *t, int *priority)
{
    thread_basic_info_data_t info;
    unsigned int count = THREAD_BASIC_INFO_COUNT;
    if (thread_info(mach_thread(t), THREAD_BASIC_INFO,
                    (thread_info_t)&info, &count) != 0) return SYS_ERR;
    *priority = info.cur_priority;
    return SYS_OK;
}

int sysThreadSetPriority(sys_thread_t *t, int priority)
{
    return cthread_priority(t->cthread, priority, 0) == 0 ? SYS_OK : SYS_ERR;
}

int sysThreadEnumerateOver_unlocked(int (*func)(sys_thread_t *, void *), void *arg)
{
    sys_thread_t *t;
    int result = 0;
    for (t = threads; t != NULL; t = t->next) {
        result = func(t, arg);
        if (result != 0) break;
    }
    return result;
}

int sysThreadEnumerateOver(int (*func)(sys_thread_t *, void *), void *arg)
{
    int result;
    sysMonitorEnter(_queue_lock);
    result = sysThreadEnumerateOver_unlocked(func, arg);
    sysMonitorExit(_queue_lock);
    return result;
}

void sysThreadInit(sys_thread_t *t, stackp_t base)
{
    t->stack_base = base;
}

stackp_t sysThreadStackBase(sys_thread_t *t)
{
    return t->stack_base;
}

void sysThreadSetStackBase(sys_thread_t *t, stackp_t base)
{
    t->stack_base = base;
}

void *sysThreadGetBackPtr(sys_thread_t *t)
{
    return t->cookie;
}

/* The shared Sun bootstrap needs TLS before it creates the Java Thread.
 * Do not replace that provisional record later: PrivateInfo already names it. */
void intrInit(void)
{
    sys_thread_t *t;
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    if (ur_cthread_self() == NULL) cthread_init();
    if (sysThreadSelf() != NULL) return;
    t = (sys_thread_t *)malloc(sizeof(*t));
    if (t == NULL || ur_cthread_self() == NULL || cthread_self() == NULL) {
        sysAbort(); abort();
    }
    memset(t, 0, sizeof(*t));
    t->cthread = cthread_self(); t->stack_base = mainstktop;
    t->run_static_initializers = TRUE;
    memcpy((char *)t->cthread + 116, &t, sizeof(t));
    primordial_pending = t;
}

bool_t threadsInitialized(void) { return threads != NULL; }
void sysThreadSetBackPtr(sys_thread_t *t, void *cookie) { t->cookie = cookie; }
void sysThreadYield(void) { cthread_yield(); }
int sysThreadCheckStack(void) { return 1; }
int sysInterruptsPending(void) { return 0; }
int sysThreadVMSuspend(sys_thread_t *t, sys_thread_t *self) { (void)t; (void)self; return 0; }
int sysThreadVMUnsuspend(sys_thread_t *t) { (void)t; return 0; }
void sysThreadVMSuspendMe(void) { }

void sysThreadSetAtBreakpoint(sys_thread_t *t)
{
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    if (t->state == RUNNABLE || t->state == RUNNING) t->state = RUNNING_AT_BREAKPOINT;
    else if (t->state == SUSPENDED) t->state = SUSPENDED_AT_BREAKPOINT;
    t->mutex.lock = 0;
}

void sysThreadInterrupt(sys_thread_t *t)
{
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    t->interrupted = TRUE;
    t->mutex.lock = 0;
}

int sysThreadIsInterrupted(sys_thread_t *t, long clear)
{
    int result;
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    result = t->interrupted;
    if (clear == TRUE) t->interrupted = FALSE;
    t->mutex.lock = 0;
    return result;
}

int sysThreadHasPendingException(sys_thread_t *t)
{
    Hjava_lang_Thread *h;
    ExecEnv *ee;
    int result;
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    h = (Hjava_lang_Thread *)t->cookie;
    ee = h ? (ExecEnv *)unhand(h)->eetop : NULL;
    result = ee != NULL && exceptionOccurred(ee);
    t->mutex.lock = 0;
    return result;
}

void sysThreadPostException(sys_thread_t *t, void *exc)
{
    Hjava_lang_Thread *h;
    ExecEnv *ee;
    if (!mutex_try_lock(&t->mutex)) mutex_wait_lock(&t->mutex);
    h = (Hjava_lang_Thread *)t->cookie;
    ee = h ? (ExecEnv *)unhand(h)->eetop : NULL;
    if (ee) { exceptionThrow(ee, (JHandle *)exc); }
    t->mutex.lock = 0;
    /* Preserve the shared VM's deferred stop checks. The monitor's bounded
     * waits observe this without overwriting a suspended native PC. */
}

/* Query the public VM-region API rather than guess private cproc stack fields.
 * Registration precedes Java allocation, retaining the JNI GC-root fix. */
int jni_AttachHostThread(sys_thread_t **result)
{
    vm_address_t address = (vm_address_t)&address, marker = address;
    vm_size_t size;
    vm_prot_t protection, max_protection;
    vm_inherit_t inheritance;
    boolean_t shared;
    memory_object_name_t object;
    vm_offset_t offset;
    if (sysThreadSelf() != NULL) { *result = sysThreadSelf(); return SYS_OK; }
    if (ur_cthread_self() == NULL || cthread_self() == NULL) return SYS_NORESOURCE;
    if (vm_region(task_self(), &address, &size, &protection, &max_protection,
                  &inheritance, &shared, &object, &offset) != 0) return SYS_ERR;
    if (object != 0) port_deallocate(task_self(), object);
    if (address > marker || size == 0 || marker - address >= size || !(protection & 1))
        return SYS_ERR;
    return sysThreadAlloc(result, (stackp_t)(address + size), NULL);
}
void jni_DetachHostThread(void) { sysThreadFree(sysThreadSelf()); }

static struct mutex attach_lock;
void sysAttachThreadLock(void)
{ if (!mutex_try_lock(&attach_lock)) mutex_wait_lock(&attach_lock); }
void sysAttachThreadUnlock(void) { attach_lock.lock = 0; }

void WaitToDie(void)
{
    sys_thread_t *self = sysThreadSelf();
    sysMonitorEnter(_queue_lock);
    while (UserThreadCount > (self->system_thread ? 0 : 1))
        sysMonitorWait(_queue_lock, -1, FALSE);
    sysMonitorExit(_queue_lock);
}

#ifdef SIGQUIT
extern void DumpMonitors(void);
static void sigquit_handler(int sig)
{
    (void)sig;
    DumpThreads(); DumpMonitors();
}
#endif
void sysThreadInitializeSystemThreads(void)
{
    InitializeFinalizerThread();
#ifdef SIGQUIT
    signal(SIGQUIT, sigquit_handler);
#endif
}

void sysThreadDumpInfo(sys_thread_t *t)
{
    fprintf(stderr, "native thread %u, stack base %p, state %d\n",
            (unsigned int)mach_thread(t), t->stack_base, (int)t->state);
}
