/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * Reentrant monitors (sys_api.h) on POSIX mutexes and condition
 * variables. Object.wait() waiters queue FIFO, each with its own flag, so a
 * notify() only wakes a thread that was already waiting when it was issued.
 */
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "sys_api.h"
#include "monitor_md.h"

struct sys_mon_waiter {
    struct sys_mon_waiter *next;
    int notified;
};

/* Serializes first-use initialization of zero-filled monitors. */
static pthread_mutex_t ready_lock = PTHREAD_MUTEX_INITIALIZER;

static void ready(sys_mon_t *mon)
{
    if (mon->ready)
	return;
    pthread_mutex_lock(&ready_lock);
    if (!mon->ready) {
	pthread_mutex_init(&mon->lock, NULL);
	pthread_cond_init(&mon->enter_cv, NULL);
	pthread_cond_init(&mon->wait_cv, NULL);
	mon->ready = 1;
    }
    pthread_mutex_unlock(&ready_lock);
}

int sysMonitorSizeof(void)
{
    return sizeof(struct sys_mon);
}

int sysMonitorInit(sys_mon_t *mon)
{
    memset(mon, 0, sizeof(*mon));	/* initialized on first use */
    return SYS_OK;
}

/*
 * shortcut: the pthread objects are not destroyed; this is harmless on
 * Linux and macOS, where they hold no kernel resources.
 */
int sysMonitorCleanup(sys_mon_t *mon, sys_thread_t *owner)
{
    return SYS_OK;
}

int sysMonitorDestroy(sys_mon_t *mon)
{
    if (mon->ready) {
	if (pthread_cond_destroy(&mon->wait_cv) != 0 ||
	    pthread_cond_destroy(&mon->enter_cv) != 0 ||
	    pthread_mutex_destroy(&mon->lock) != 0)
	    return SYS_ERR;
	mon->ready = 0;
    }
    return SYS_OK;
}

bool_t sysMonitorInUse(sys_mon_t *mon)
{
    return mon->owner != NULL || mon->first != NULL;
}

bool_t sysMonitorEntered(sys_mon_t *mon)
{
    return mon->owner != NULL && mon->owner == sysThreadSelf();
}

/* Take ownership; called holding the lock. */
static void acquire_locked(sys_mon_t *mon, sys_thread_t *self, int count)
{
    while (mon->owner != NULL)
	pthread_cond_wait(&mon->enter_cv, &mon->lock);
    mon->owner = self;
    mon->count = count;
}

int sysMonitorEnter(sys_mon_t *mon)
{
    sys_thread_t *self = sysThreadSelf();

    if (self != NULL && mon->owner == self) {	/* only this thread can have set it */
	mon->count++;
	return SYS_OK;
    }
    ready(mon);
    pthread_mutex_lock(&mon->lock);
    acquire_locked(mon, self, 1);
    pthread_mutex_unlock(&mon->lock);
    return SYS_OK;
}

int sysMonitorExit(sys_mon_t *mon)
{
    if (mon->owner == NULL || mon->owner != sysThreadSelf())
	return SYS_ERR;
    if (--mon->count > 0)
	return SYS_OK;
    pthread_mutex_lock(&mon->lock);
    mon->owner = NULL;
    pthread_mutex_unlock(&mon->lock);
    pthread_cond_signal(&mon->enter_cv);	/* after unlocking: no hurry-up-and-wait */
    return SYS_OK;
}

static int notify(sys_mon_t *mon, int all)
{
    struct sys_mon_waiter *w;

    if (mon->owner == NULL || mon->owner != sysThreadSelf())
	return SYS_ERR;
    pthread_mutex_lock(&mon->lock);
    while ((w = mon->first) != NULL) {
	mon->first = w->next;
	w->notified = 1;
	if (!all)
	    break;
    }
    if (mon->first == NULL)
	mon->last = NULL;
    pthread_mutex_unlock(&mon->lock);
    if (w != NULL || all)
	pthread_cond_broadcast(&mon->wait_cv);
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

int sysMonitorWait(sys_mon_t *mon, int millis, bool_t clear)
{
    sys_thread_t *self = sysThreadSelf();
    struct sys_mon_waiter me;
    struct timespec deadline;
    int saved, stopped = 0, ret = SYS_OK;

    if (self == NULL || mon->owner != self)
	return SYS_ERR;
    if (sysThreadIsInterrupted(self, clear))
	return SYS_INTRPT;
    if (millis != SYS_TIMEOUT_INFINITY) {
	struct timeval now;
	gettimeofday(&now, NULL);
	deadline.tv_sec = now.tv_sec + millis / 1000;
	deadline.tv_nsec = now.tv_usec * 1000L + (millis % 1000) * 1000000L;
	if (deadline.tv_nsec >= 1000000000L) {
	    deadline.tv_sec++;
	    deadline.tv_nsec -= 1000000000L;
	}
    }

    pthread_mutex_lock(&mon->lock);
    me.next = NULL;
    me.notified = 0;
    if (mon->last)
	mon->last->next = &me;
    else
	mon->first = &me;
    mon->last = &me;

    /* Release the monitor completely while waiting. */
    saved = mon->count;
    mon->owner = NULL;
    mon->count = 0;
    pthread_cond_signal(&mon->enter_cv);

    stopped = sysThreadSetWaitingOn(self, mon);
    while (!me.notified && !stopped && !self->stop_pending &&
	   !sysThreadIsInterrupted(self, FALSE)) {
	int err;
	if (millis == SYS_TIMEOUT_INFINITY)
	    err = pthread_cond_wait(&mon->wait_cv, &mon->lock);
	else
	    err = pthread_cond_timedwait(&mon->wait_cv, &mon->lock, &deadline);
	if (err == ETIMEDOUT && !me.notified) {
	    ret = SYS_TIMEOUT;
	    break;
	}
    }
    sysThreadSetWaitingOn(self, NULL);
    self->stop_pending = 0;	/* the posted exception is now pending in the EE */
    if (!me.notified)
	unlink_waiter(mon, &me);

    acquire_locked(mon, self, saved);
    pthread_mutex_unlock(&mon->lock);
    if (sysThreadIsInterrupted(self, clear))
	return SYS_INTRPT;
    return ret;
}

/* Wake a thread blocked in wait() so it sees a posted exception. */
void sysMonitorInterrupt(sys_mon_t *mon)
{
    pthread_mutex_lock(&mon->lock);
    pthread_cond_broadcast(&mon->wait_cv);
    pthread_mutex_unlock(&mon->lock);
}

void sysMonitorDumpInfo(sys_mon_t *mon)
{
    if (mon->owner)
	fprintf(stderr, "owner thread %p, %d entr%s\n", (void *) mon->owner,
		mon->count, mon->count == 1 ? "y" : "ies");
    else
	fprintf(stderr, "<unowned>\n");
}
