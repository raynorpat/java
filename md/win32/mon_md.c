/*
 * Reentrant monitors (sys_api.h) on Win32 SRW locks and condition
 * variables. Object.wait() waiters queue FIFO, each with its own flag, so a
 * notify() only wakes a thread that was already waiting when it was issued.
 */
#include <windows.h>
#include <stdio.h>

#include "sys_api.h"
#include "monitor_md.h"

struct sys_mon_waiter {
    struct sys_mon_waiter *next;
    int notified;
};

#define LOCK(m)		((PSRWLOCK) &(m)->lock)
#define ENTER_CV(m)	((PCONDITION_VARIABLE) &(m)->enter_cv)
#define WAIT_CV(m)	((PCONDITION_VARIABLE) &(m)->wait_cv)

int sysMonitorSizeof(void)
{
    return sizeof(struct sys_mon);
}

int sysMonitorInit(sys_mon_t *mon, bool_t in_cache)
{
    memset(mon, 0, sizeof(*mon));	/* zero is SRWLOCK_INIT / CONDITION_VARIABLE_INIT */
    return SYS_OK;
}

int sysMonitorDestroy(sys_mon_t *mon, sys_thread_t *owner)
{
    return SYS_OK;
}

bool_t sysMonitorEntered(sys_mon_t *mon)
{
    return mon->owner == GetCurrentThreadId();
}

/* Take ownership; called holding the lock. */
static void acquire_locked(sys_mon_t *mon, DWORD self, int count)
{
    while (mon->owner != 0)
	SleepConditionVariableSRW(ENTER_CV(mon), LOCK(mon), INFINITE, 0);
    mon->owner = self;
    mon->count = count;
}

int sysMonitorEnter(sys_mon_t *mon)
{
    DWORD self = GetCurrentThreadId();

    if (mon->owner == self) {	/* only this thread can have set it */
	mon->count++;
	return SYS_OK;
    }
    AcquireSRWLockExclusive(LOCK(mon));
    acquire_locked(mon, self, 1);
    ReleaseSRWLockExclusive(LOCK(mon));
    return SYS_OK;
}

int sysMonitorExit(sys_mon_t *mon)
{
    if (mon->owner != GetCurrentThreadId())
	return SYS_ERR;
    if (--mon->count > 0)
	return SYS_OK;
    AcquireSRWLockExclusive(LOCK(mon));
    mon->owner = 0;
    ReleaseSRWLockExclusive(LOCK(mon));
    WakeConditionVariable(ENTER_CV(mon));
    return SYS_OK;
}

static int notify(sys_mon_t *mon, int all)
{
    struct sys_mon_waiter *w;

    if (mon->owner != GetCurrentThreadId())
	return SYS_ERR;
    AcquireSRWLockExclusive(LOCK(mon));
    while ((w = mon->first) != NULL) {
	mon->first = w->next;
	w->notified = 1;
	if (!all)
	    break;
    }
    if (mon->first == NULL)
	mon->last = NULL;
    ReleaseSRWLockExclusive(LOCK(mon));
    if (w != NULL || all)
	WakeAllConditionVariable(WAIT_CV(mon));
    return SYS_OK;
}

int sysMonitorNotify(sys_mon_t *mon)
{
    return notify(mon, 0);
}

int sysMonitorNotifyAll(sys_mon_t *mon)
{
    return notify(mon, 1);
}

static void unlink_waiter(sys_mon_t *mon, struct sys_mon_waiter *me)
{
    struct sys_mon_waiter **pp, *prev = NULL;

    for (pp = &mon->first; *pp != NULL; prev = *pp, pp = &(*pp)->next) {
	if (*pp == me) {
	    *pp = me->next;
	    if (mon->last == me)
		mon->last = prev;
	    return;
	}
    }
}

int sysMonitorWait(sys_mon_t *mon, int millis)
{
    DWORD self = GetCurrentThreadId();
    sys_thread_t *t = sysThreadSelf();
    struct sys_mon_waiter me;
    ULONGLONG deadline = 0;
    int saved, ret = SYS_OK;

    if (mon->owner != self)
	return SYS_ERR;
    if (millis != SYS_TIMEOUT_INFINITY)
	deadline = GetTickCount64() + (ULONGLONG) millis;

    AcquireSRWLockExclusive(LOCK(mon));
    me.next = NULL;
    me.notified = 0;
    if (mon->last)
	mon->last->next = &me;
    else
	mon->first = &me;
    mon->last = &me;

    /* Release the monitor completely while waiting. */
    saved = mon->count;
    mon->owner = 0;
    mon->count = 0;
    WakeConditionVariable(ENTER_CV(mon));

    /* Full barrier: pairs with sysThreadPostException's write of stop_pending. */
    if (t)
	InterlockedExchangePointer((PVOID volatile *) &t->waiting_on, mon);
    while (!me.notified && !(t && t->stop_pending)) {
	DWORD wait = INFINITE;
	if (millis != SYS_TIMEOUT_INFINITY) {
	    ULONGLONG now = GetTickCount64();
	    if (now >= deadline) {
		ret = SYS_TIMEOUT;
		break;
	    }
	    wait = (DWORD) (deadline - now);
	}
	SleepConditionVariableSRW(WAIT_CV(mon), LOCK(mon), wait, 0);
    }
    if (t) {
	t->waiting_on = NULL;
	t->stop_pending = 0;	/* the posted exception is now pending in the EE */
    }
    if (!me.notified)
	unlink_waiter(mon, &me);

    acquire_locked(mon, self, saved);
    ReleaseSRWLockExclusive(LOCK(mon));
    return ret;
}

/* Wake a thread blocked in wait() so it sees a posted exception. */
void sysMonitorInterrupt(sys_mon_t *mon)
{
    AcquireSRWLockExclusive(LOCK(mon));
    ReleaseSRWLockExclusive(LOCK(mon));
    WakeAllConditionVariable(WAIT_CV(mon));
}

void sysMonitorDumpInfo(sys_mon_t *mon)
{
    if (mon->owner)
	fprintf(stderr, "owner thread %lu, %d entr%s\n", mon->owner,
		mon->count, mon->count == 1 ? "y" : "ies");
    else
	fprintf(stderr, "<unowned>\n");
}
