/* Deterministic Mach monitor contracts; OS waits are explicit test doubles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static sys_mon_t mon;
static sys_thread_t self, other, *current = &self;
static int initialized = 1, signals, broadcasts, waits, enters, destroys;
static int action, contention, pending, fail_entry, aborted;
static int gap_delivered;
static int64_t now;
static void test_notify_selection(void);
sys_thread_t *sysThreadSelf(void) { return current; }
bool_t threadsInitialized(void) { return initialized; }
int sysThreadHasPendingException(sys_thread_t *t) { CHECK(t == &self); return pending; }
int sysThreadIsInterrupted(sys_thread_t *t, long clear)
{
    int result = t->interrupted;
    if (clear) t->interrupted = 0;
    return result;
}
int64_t sysTimeMillis(void) { return now; }
int mutex_try_lock(mutex_t m) { CHECK(!m->lock); m->lock = 1; return 1; }
void mutex_wait_lock(mutex_t m) { (void)m; CHECK(0); }
int condvarInit(condition_t c) { memset(c, 0, sizeof(*c)); return 0; }
int condvarDestroy(condition_t c) { (void)c; ++destroys; return 0; }
int condvarSignal(condition_t c)
{
    CHECK(mon.mutex.lock && (c == &mon.cv_monitor || c == &mon.cv_waiters));
    ++signals; return 0;
}
int condvarBroadcast(condition_t c)
{
    CHECK(mon.mutex.lock && c == &mon.cv_monitor); ++broadcasts; return 0;
}
int condvarWait(condition_t c, mutex_t m, int state)
{
    CHECK(c == &mon.cv_waiters && m == &mon.mutex && m->lock);
    CHECK(state == MONITOR_WAIT && self.mon_wait == &mon);
    CHECK(mon.waiters_queue.count == 1 && mon.waiters_queue.head->waiting_thread == &self);
    CHECK(mon.mwait_queue.count == 0 && mon.monitor_owner == &other);
    ++enters;
    if (fail_entry) return SYS_ERR;
    mon.monitor_owner = NULL; mon.entry_count = 0;
    return 0;
}
int condvarTimedWait(condition_t c, mutex_t m, unsigned int ms, int state)
{
    CHECK(c == &mon.cv_monitor && m == &mon.mutex && m->lock);
    CHECK(state == CONDVAR_WAIT && ms > 0 && ms <= 50);
    CHECK((mon.monitor_owner == NULL && !mon.entry_count) ||
          (mon.monitor_owner == &other && mon.entry_count == 1));
    CHECK(mon.mwait_queue.count == 1 && self.monitor_entry_count == 2);
    CHECK(mon.mwait_queue.head->waiting_thread == &self && self.mon_wait == &mon);
    CHECK(mon.mwait_queue.head->prev == &mon.mwait_queue.head);
    ++waits; now += ms;
    if (action == 0 || action == 5 || action == 6) {
        CHECK(!gap_delivered); gap_delivered = 1;
        /* Native timeout already dequeued us, before mutex reacquisition. */
        m->lock = 0; current = &other;
        CHECK(sysMonitorEnter(&mon) == 0);
        CHECK((action == 6 ? sysMonitorNotifyAll(&mon) : sysMonitorNotify(&mon)) == 0);
        CHECK(sysMonitorExit(&mon) == 0);
        current = &self; m->lock = 1;
        if (contention && action == 0) { mon.monitor_owner = &other; mon.entry_count = 1; }
        return action == 0 ? SYS_OK : SYS_TIMEOUT;
    }
    if (contention) { mon.monitor_owner = &other; mon.entry_count = 1; }
    if (action == 1) return SYS_TIMEOUT;
    if (action == 2) { self.interrupted = 1; return SYS_TIMEOUT; }
    if (action == 3) { pending = 1; return SYS_TIMEOUT; }
    if (action == 4) return SYS_ERR;
    return SYS_OK;
}
void sysAbort(void) { CHECK(fail_entry && mon.monitor_owner == &other); ++aborted; }
static void test_abort(void) { CHECK(aborted == 1); puts("entry failure cannot steal ownership"); exit(0); }

int main(int argc, char **argv)
{
    int i, before, status[7] = { SYS_OK, SYS_TIMEOUT, SYS_INTRPT, SYS_OK, SYS_ERR, SYS_OK, SYS_OK };
    (void)argv;
    CHECK(sysMonitorSizeof() == 68 && sysMonitorInit(&mon) == 0);
    CHECK(!sysMonitorInUse(&mon) && !sysMonitorEntered(&mon));
    initialized = 0; CHECK(sysMonitorEntered(&mon)); initialized = 1;
    CHECK(sysMonitorExit(&mon) == SYS_ERR && sysMonitorNotify(&mon) == SYS_ERR);
    CHECK(sysMonitorWait(&mon, 10, TRUE) == SYS_ERR);
    CHECK(sysMonitorEnter(&mon) == 0 && sysMonitorEnter(&mon) == 0);
    CHECK(mon.monitor_owner == &self && mon.entry_count == 2);
    /* A suspended contender may own the native mutex during GC. Recursion
     * by its logical owner must not touch that mutex, including enumeration. */
    mon.mutex.lock = 1;
    CHECK(sysMonitorEnter(&mon) == 0 && mon.entry_count == 3);
    CHECK(sysMonitorExit(&mon) == 0 && mon.entry_count == 2 && mon.mutex.lock == 1);
    mon.mutex.lock = 0;
    CHECK(sysMonitorEntered(&mon) && sysMonitorDestroy(&mon) == SYS_ERR);
    current = &other;
    CHECK(!sysMonitorEntered(&mon) && sysMonitorExit(&mon) == SYS_ERR);
    CHECK(sysMonitorNotifyAll(&mon) == SYS_ERR); current = &self;
    before = broadcasts;
    CHECK(sysMonitorNotify(&mon) == 0 && sysMonitorNotifyAll(&mon) == 0);
    CHECK(broadcasts == before); /* no eligible Java waiter */
    self.interrupted = 1; before = waits;
    CHECK(sysMonitorWait(&mon, 10, FALSE) == SYS_INTRPT && self.interrupted);
    CHECK(sysMonitorWait(&mon, 10, TRUE) == SYS_INTRPT && !self.interrupted);
    CHECK(waits == before && mon.entry_count == 2);
    for (i = 0; i < 7; ++i) {
        action = i; contention = 1; pending = 0; gap_delivered = 0;
        CHECK(sysMonitorWait(&mon, i == 1 ? 125 : -1, TRUE) == status[i]);
        CHECK(mon.monitor_owner == &self && mon.entry_count == 2);
        CHECK(!mon.mutex.lock && !self.mon_wait && !self.monitor_entry_count);
        CHECK(!mon.waiters_queue.head && !mon.waiters_queue.count);
        CHECK(!mon.mwait_queue.head && !mon.mwait_queue.count);
        CHECK(!self.interrupted);
    }
    CHECK(enters == 5);
    test_notify_selection();
    CHECK(sysMonitorExit(&mon) == 0 && mon.entry_count == 1);
    CHECK(sysMonitorExit(&mon) == 0 && !sysMonitorInUse(&mon));
    mon.waiters_queue.count = 1; CHECK(sysMonitorInUse(&mon)); mon.waiters_queue.count = 0;
    mon.monitor_owner = &other; mon.entry_count = 1;
    if (argc > 1) fail_entry = 1;
    CHECK(sysMonitorEnter(&mon) == 0 && mon.monitor_owner == &self);
    CHECK(sysMonitorExit(&mon) == 0 && sysMonitorDestroy(&mon) == 0 && destroys == 2);
    puts("Mach monitor contracts passed"); return 0;
}
#define abort test_abort
#include "../../src/unix/java/runtime/mon_cthreads.c"

static void test_notify_selection(void)
{
    struct java_waiter a, b;
    a.notified = b.notified = 0;
    enqueue(&mon.mwait_queue, &a.node, &self);
    enqueue(&mon.mwait_queue, &b.node, &other);
    CHECK(sysMonitorNotify(&mon) == SYS_OK && b.notified && !a.notified);
    CHECK(mon.mwait_queue.count == 2); /* selected waiters remain live */
    CHECK(sysMonitorNotify(&mon) == SYS_OK && a.notified);
    a.notified = b.notified = 0;
    CHECK(sysMonitorNotifyAll(&mon) == SYS_OK && a.notified && b.notified);
    unlink_waiter(&mon.mwait_queue, &a.node);
    unlink_waiter(&mon.mwait_queue, &b.node);
}
