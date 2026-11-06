/* Win32 monitor reconstruction from JDK 1.1.5 javai_g.dll.
 * See tests/native/monitor-evidence.md for addresses and interim deviations.
 */
#include <windows.h>
#include <stdio.h>
#include "sys_api.h"
#include "monitor_md.h"

int sysMonitorSizeof(void)
{
    return sizeof(sys_mon_t);
}

int sysMonitorInit(sys_mon_t *mon)
{
    sysAssert(mon != NULL);
    mon->monitor_owner = NULL;
    mon->entry_count = 0;
    mutexInit(&mon->mutex);
    if (condvarInit(&mon->condvar)) return SYS_OK;
    mutexDestroy(&mon->mutex);
    return SYS_ERR;
}

int sysMonitorDestroy(sys_mon_t *mon)
{
    sysAssert(mon != NULL);
    mutexDestroy(&mon->mutex);
    condvarDestroy(&mon->condvar);
    return SYS_OK;
}

/* Compatibility with the old shared monitor thread-exit cleanup contract.
 * This preserves the former Windows no-op; it is not Sun sysMonitorDestroy.
 */
int sysMonitorCleanup(sys_mon_t *mon, sys_thread_t *owner)
{
    return SYS_OK;
}

int sysMonitorEnter(sys_mon_t *mon)
{
    sys_thread_t *self;
    self = sysThreadSelf();
    sysAssert(mon != NULL);
    if (self == NULL) return ThreadsInitialized ? SYS_ERR : SYS_OK;
    sysAssert(ThreadsInitialized);
    self->state = MONITOR_WAIT;
    mutexLock(&mon->mutex);
    self->state = RUNNABLE;
    if (mon->monitor_owner != NULL) {
        sysAssert(mon->monitor_owner == self);
        sysAssert(mon->entry_count > 0);
        mutexUnlock(&mon->mutex);
    } else {
        mon->monitor_owner = self;
    }
    ++mon->entry_count;
    return SYS_OK;
}

bool_t sysMonitorEntered(sys_mon_t *mon)
{
    sys_thread_t *self;
    self = sysThreadSelf();
    sysAssert(mon != NULL);
    return !ThreadsInitialized ||
        (self != NULL && mon->monitor_owner == self);
}

int sysMonitorExit(sys_mon_t *mon)
{
    sys_thread_t *self;
    self = sysThreadSelf();
    sysAssert(mon != NULL);
    if (self == NULL) return ThreadsInitialized ? SYS_ERR : SYS_OK;
    sysAssert(ThreadsInitialized);
    if (mon->monitor_owner != self) return SYS_ERR;
    sysAssert(mon->entry_count > 0);
    if (--mon->entry_count == 0) {
        mon->monitor_owner = NULL;
        mutexUnlock(&mon->mutex);
    }
    return SYS_OK;
}

int sysMonitorNotify(sys_mon_t *mon)
{
    sysAssert(mon != NULL);
    if (!sysMonitorEntered(mon)) return SYS_ERR;
    return condvarSignal(&mon->condvar) ? SYS_OK : SYS_ERR;
}

int sysMonitorNotifyAll(sys_mon_t *mon)
{
    sysAssert(mon != NULL);
    if (!sysMonitorEntered(mon)) return SYS_ERR;
    return condvarBroadcast(&mon->condvar) ? SYS_OK : SYS_ERR;
}

int sysMonitorWait(sys_mon_t *mon, int millis, bool_t clear)
{
    sys_thread_t *self;
    long saved;
    self = sysThreadSelf();
    sysAssert(mon != NULL);
    saved = mon->entry_count;
    if (self == NULL) return SYS_ERR;
    sysAssert(ThreadsInitialized);
    if (mon->monitor_owner != self) return SYS_ERR;
    if (sysThreadIsInterrupted(self, clear)) return SYS_INTRPT;
    mon->monitor_owner = NULL;
    mon->entry_count = 0;
    self->state = CONDVAR_WAIT;
    if (millis == SYS_TIMEOUT_INFINITY)
        condvarWait(self, &mon->condvar, &mon->mutex);
    else
        condvarTimedWait(self, &mon->condvar, &mon->mutex, (DWORD)millis);
    self->state = RUNNABLE;
    mon->entry_count = saved;
    mon->monitor_owner = self;
    return sysThreadIsInterrupted(self, clear) ? SYS_INTRPT : SYS_OK;
}

bool_t sysMonitorInUse(sys_mon_t *mon)
{
    return mon->monitor_owner != NULL || mon->condvar.waiters != 0 ||
        mon->condvar.signals != 0;
}

/* Diagnostic deviation: retain this port's numeric owner identification. */
void sysMonitorDumpInfo(sys_mon_t *mon)
{
    sysAssert(mon != NULL);
    if (mon->monitor_owner != NULL)
        fprintf(stderr, "owner thread %lu, %ld entr%s\n",
                mon->monitor_owner->id, mon->entry_count,
                mon->entry_count == 1 ? "y" : "ies");
    else
        fprintf(stderr, "<unowned>\n");
    if (mon->condvar.waiters != 0)
        fprintf(stderr, "waiters: %u\n", mon->condvar.waiters);
    if (mon->condvar.signals != 0)
        fprintf(stderr, "tickets: %u\n", mon->condvar.signals);
}
