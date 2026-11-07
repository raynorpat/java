#ifndef sun_java_monitor_md_h___
#define sun_java_monitor_md_h___

#ifdef JDK115_CTHREADS
#include "cthreads/monitor_md.h"
#else
#include <pthread.h>
#include "threads_md.h"

/*
 * Reentrant monitor (md/unix/mon_md.c). Monitors may be used while still
 * zero-filled, so the pthread objects are initialized on first use.
 */
struct sys_mon_waiter;

struct sys_mon {
    volatile int ready;		/* the pthread objects below are initialized */
    pthread_mutex_t lock;	/* guards the fields below */
    pthread_cond_t enter_cv;	/* monitor became free */
    pthread_cond_t wait_cv;	/* a waiter was notified */
    struct sys_thread *volatile owner;	/* owning thread, NULL if free */
    int count;			/* recursive entry count */
    struct sys_mon_waiter *first, *last;	/* threads in wait(), FIFO */
};

#define SYS_TIMEOUT_INFINITY -1

/* Collection is synchronous; there is no idle-time GC thread. */
#define SYS_INTERRUPTS_PENDING() 0

extern int sysMonitorDestroy(struct sys_mon *);

#endif /* JDK115_CTHREADS */
#endif /* sun_java_monitor_md_h___ */
