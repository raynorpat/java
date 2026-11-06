/* Reconstructed from Rhapsody DR2 libjava.A.dylib.
 * Allocation: 5C144970; init: 5C144CFC; self/enumeration: 5C1451FC-5C14529C.
 * This backend is not selected for the VM until its lifecycle is complete.
 */
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "java_lang_Thread.h"
#include "monitor_cache.h"

/* Shared VM thread-queue state, declared in threads.h. */
extern sys_mon_t *_queue_lock;
extern int ActiveThreadCount;
extern int UserThreadCount;
extern stackp_t mainstktop;
static sys_thread_t *threads;

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
    sysMonitorEnter(_queue_lock);
    ++ActiveThreadCount;
    t->next = threads;
    threads = t;
    if (!t->system_thread) ++UserThreadCount;
    sysMonitorExit(_queue_lock);
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

    sysMonitorEnter(_queue_lock);
    ++ActiveThreadCount;
    t->next = threads;
    threads = t;
    if (!t->system_thread) ++UserThreadCount;
    sysMonitorExit(_queue_lock);
    *result = t;
    return SYS_OK;
}

/* 5C144A38, VM portion. Foundation's NSThread initialization belongs to the
 * later AppKit integration, not this Sun VM's C platform backend. */
int sysThreadBootstrap(sys_thread_t **result, void *cookie)
{
    int status = sysThreadAlloc(result, mainstktop, cookie);
    if (status == SYS_OK) (*result)->primordial_thread = TRUE;
    return status;
}

sys_thread_t *sysThreadSelf(void)
{
    sys_thread_t *t;
    memcpy(&t, (char *)cthread_self() + 116, sizeof(t));
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

    if (h != NULL) {
        ee = (ExecEnv *)unhand(h)->eetop;
        unhand(h)->PrivateInfo = 0;
        unhand(h)->eetop = 0;
    }
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
