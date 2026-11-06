/* Reconstructed from Rhapsody DR2 libjava.A.dylib 5C13660C-5C1366EC. */
#include <stddef.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/condvar_md.h"

int condvarInit(condition_t c)
{
    c->lock = 0;
    c->queue.tail = NULL;
    c->queue.head = NULL;
    return 0;
}

int condvarDestroy(condition_t c)
{
    if (c->queue.head) cond_broadcast(c);
    spin_lock(&c->lock);
    return 0;
}

int condvarWait(condition_t c, mutex_t m, int state)
{
    sys_thread_t *thread = sysThreadSelf();
    int result;
    if (thread) thread->state = state;
    result = condition_wait_timeout(c, m, (unsigned int)-1);
    if (thread) thread->state = RUNNING;
    return result;
}

int condvarTimedWait(condition_t c, mutex_t m, unsigned int millisecs, int state)
{
    sys_thread_t *thread = sysThreadSelf();
    int result;
    if (thread) thread->state = state;
    result = condition_wait_timeout(c, m, millisecs);
    if (thread) thread->state = RUNNING;
    return result;
}

int condvarSignal(condition_t c)
{
    condition_signal_timeout(c);
    return 0;
}

int condvarBroadcast(condition_t c)
{
    condition_broadcast_timeout(c);
    return 0;
}
