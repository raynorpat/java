/* Reconstructed from DR2 libjava.A.dylib 5C136060-5C13660C.
 * Preserve the original records/queues and restore the shared VM's wait,
 * interruption and ownership guarantees. See the Phase 3 evidence ledger.
 */
#include <stdlib.h>
#include <stdio.h>
#include "typedefs.h"
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "cthreads/runtime.h"

static void lock_monitor(sys_mon_t *m)
{
    if (!mutex_try_lock(&m->mutex)) mutex_wait_lock(&m->mutex);
}

/* Keep notification eligibility for the entire Java wait, including gaps
 * between native polling waits. The original queue node remains first. */
struct java_waiter {
    monitor_waiter_t node;
    int notified;
};

static void enqueue(monitor_wait_queue_t *q, monitor_waiter_t *w, sys_thread_t *t)
{
    w->waiting_thread = t;
    w->next = q->head;
    w->prev = &q->head;
    if (w->next) w->next->prev = &w->next;
    q->head = w;
    ++q->count;
}

static void unlink_waiter(monitor_wait_queue_t *q, monitor_waiter_t *w)
{
    *w->prev = w->next;
    if (w->next) w->next->prev = w->prev;
    --q->count;
}

int sysMonitorSizeof(void) { return sizeof(sys_mon_t); }

int sysMonitorInit(sys_mon_t *m)
{
    int result;
    m->mutex.lock = 0;
    result = condvarInit(&m->cv_waiters);
    if (result == SYS_OK) result = condvarInit(&m->cv_monitor);
    m->entry_count = 0; m->monitor_owner = NULL;
    INIT_MONITOR_WAIT_QUEUE(m->waiters_queue);
    INIT_MONITOR_WAIT_QUEUE(m->mwait_queue);
    return result;
}

bool_t sysMonitorInUse(sys_mon_t *m)
{
    return m->monitor_owner != NULL || m->mwait_queue.count != 0 ||
           m->waiters_queue.count != 0;
}

int sysMonitorDestroy(sys_mon_t *m)
{
    if (sysMonitorInUse(m)) return SYS_ERR;
    condvarDestroy(&m->cv_waiters);
    condvarDestroy(&m->cv_monitor);
    return SYS_OK;
}

int sysMonitorCleanup(sys_mon_t *m, sys_thread_t *owner)
{
    (void)m; (void)owner;
    return SYS_OK;
}

bool_t sysMonitorEntered(sys_mon_t *m)
{
    if (!threadsInitialized()) return TRUE;
    return m->monitor_owner != NULL && m->monitor_owner == sysThreadSelf();
}

static void acquire(sys_mon_t *m, sys_thread_t *self)
{
    monitor_waiter_t waiter;
    enqueue(&m->waiters_queue, &waiter, self);
    self->mon_wait = m;
    while (m->monitor_owner != NULL) {
        if (condvarWait(&m->cv_waiters, &m->mutex, MONITOR_WAIT) == SYS_ERR) {
            /* Never return claiming ownership of another thread's monitor. */
            sysAbort(); abort();
        }
    }
    unlink_waiter(&m->waiters_queue, &waiter);
    self->mon_wait = NULL;
    m->monitor_owner = self;
}

int sysMonitorEnter(sys_mon_t *m)
{
    sys_thread_t *self = sysThreadSelf();
    if (self == NULL) return SYS_ERR;
    /* Only the logical owner changes its recursion depth. GC may have
     * suspended a contender while that contender holds the native mutex. */
    if (m->monitor_owner == self) { ++m->entry_count; return SYS_OK; }
    lock_monitor(m);
    if (m->monitor_owner != self) acquire(m, self);
    ++m->entry_count;
    m->mutex.lock = 0;
    return SYS_OK;
}

int sysMonitorExit(sys_mon_t *m)
{
    int result = SYS_OK;
    sys_thread_t *self = sysThreadSelf();
    if (self != NULL && m->monitor_owner == self && m->entry_count > 1) {
        --m->entry_count; return SYS_OK;
    }
    lock_monitor(m);
    if (self == NULL || m->monitor_owner != self) result = SYS_ERR;
    else if (--m->entry_count == 0) {
        m->monitor_owner = NULL;
        if (m->waiters_queue.count) condvarSignal(&m->cv_waiters);
    }
    m->mutex.lock = 0;
    return result;
}

static int notify(sys_mon_t *m, int all)
{
    int result = SYS_OK, wake = 0;
    monitor_waiter_t *node;
    sys_thread_t *self = sysThreadSelf();
    lock_monitor(m);
    if (self == NULL || m->monitor_owner != self) result = SYS_ERR;
    else {
        for (node = m->mwait_queue.head; node; node = node->next) {
            struct java_waiter *waiter = (struct java_waiter *)node;
            if (waiter->notified) continue;
            waiter->notified = 1; wake = 1;
            if (!all) break;
        }
        /* The selected waiter may be between native waits. Wake all native
         * sleepers; only selected Java waiters consume the notification. */
        if (wake) condvarBroadcast(&m->cv_monitor);
    }
    m->mutex.lock = 0;
    return result;
}

int sysMonitorNotify(sys_mon_t *m) { return notify(m, 0); }
int sysMonitorNotifyAll(sys_mon_t *m) { return notify(m, 1); }

int sysMonitorWait(sys_mon_t *m, int millis, bool_t clear)
{
    sys_thread_t *self = sysThreadSelf();
    struct java_waiter waiter;
    int result = SYS_OK, delay;
    int64_t deadline = 0, remaining;
    lock_monitor(m);
    if (self == NULL || m->monitor_owner != self) {
        m->mutex.lock = 0; return SYS_ERR;
    }
    if (sysThreadIsInterrupted(self, clear)) {
        m->mutex.lock = 0; return SYS_INTRPT;
    }
    if (millis != SYS_TIMEOUT_INFINITY) deadline = sysTimeMillis() + millis;
    self->mon_wait = m;
    self->monitor_entry_count = m->entry_count;
    m->entry_count = 0; m->monitor_owner = NULL;
    condvarSignal(&m->cv_waiters);
    waiter.notified = 0;
    enqueue(&m->mwait_queue, &waiter.node, self);
    /* The original merely sets an interrupt bit. Bound native waits so an
     * interrupt/stop cannot be lost before a Mach wait port is ready. No
     * asynchronous dereference of a possibly retired monitor is needed. */
    while (!waiter.notified && !sysThreadIsInterrupted(self, FALSE) && !sysThreadHasPendingException(self)) {
        delay = 50;
        if (millis != SYS_TIMEOUT_INFINITY) {
            remaining = deadline - sysTimeMillis();
            if (remaining <= 0) { result = SYS_TIMEOUT; break; }
            if (remaining < delay) delay = (int)remaining;
        }
        result = condvarTimedWait(&m->cv_monitor, &m->mutex, delay, CONDVAR_WAIT);
        if (result != SYS_TIMEOUT && result != SYS_OK) break;
        result = SYS_OK;
    }
    unlink_waiter(&m->mwait_queue, &waiter.node);
    /* Preserve the wait result across entry contention (the original loses
     * timeout status there), and always reacquire the saved recursion depth. */
    acquire(m, self);
    m->entry_count = self->monitor_entry_count;
    self->monitor_entry_count = 0;
    if (sysThreadIsInterrupted(self, clear)) result = SYS_INTRPT;
    m->mutex.lock = 0;
    return result;
}

void sysMonitorDumpInfo(sys_mon_t *m)
{
    fprintf(stderr, "owner %p, entries %ld, entering %d, waiting %d\n",
            (void *)m->monitor_owner, m->entry_count,
            m->waiters_queue.count, m->mwait_queue.count);
}
