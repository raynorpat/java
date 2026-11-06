#ifndef sun_java_monitor_md_h___
#define sun_java_monitor_md_h___

#include "threads_md.h"

/*
 * Reentrant monitor (md/mon_md.c). The first three fields are a Win32
 * SRWLOCK and two CONDITION_VARIABLEs, which are valid when zero-filled, so
 * a memset-cleared monitor works before sysMonitorInit is called.
 */
struct sys_mon_waiter;

struct sys_mon {
    void *lock;			/* SRWLOCK guarding the fields below */
    void *enter_cv;		/* CONDITION_VARIABLE: monitor became free */
    void *wait_cv;		/* CONDITION_VARIABLE: a waiter was notified */
    volatile unsigned long owner;	/* owning Win32 thread id, 0 if free */
    int count;			/* recursive entry count */
    struct sys_mon_waiter *first, *last;	/* threads in wait(), FIFO */
};

#define SYS_TIMEOUT_INFINITY -1

/* Collection is synchronous; there is no idle-time GC thread. */
#define SYS_INTERRUPTS_PENDING() 0

extern int sysMonitorDestroy(sys_mon_t *, sys_thread_t *);

#endif /* sun_java_monitor_md_h___ */
