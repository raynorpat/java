/* Reconstructed from Rhapsody DR2 libjava.A.dylib.
 * Allocation: 5C144970; init: 5C144CFC; self/enumeration: 5C1451FC-5C14529C.
 * This backend is not selected for the VM until its lifecycle is complete.
 */
#include <stdlib.h>
#include <string.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"

/* Shared VM thread-queue state, declared in threads.h. */
extern sys_mon_t *_queue_lock;
extern int ActiveThreadCount;
extern int UserThreadCount;
static sys_thread_t *threads;

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

sys_thread_t *sysThreadSelf(void)
{
    sys_thread_t *t;
    memcpy(&t, (char *)cthread_self() + 116, sizeof(t));
    return t;
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
