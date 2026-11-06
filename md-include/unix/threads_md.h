#ifndef sun_java_threads_md_h___
#define sun_java_threads_md_h___

#include <pthread.h>
#include <signal.h>

/*
 * POSIX threads (md/unix/thread_md.c). A thread is stopped, for the GC or
 * Thread.suspend(), by a signal whose handler waits until released.
 */
struct sys_mon;

struct sys_thread {
    pthread_t handle;
    void *cookie;		/* back pointer to the Java thread object */
    void *stack_base;		/* highest address scanned by the GC */
    void *volatile stack_ptr;	/* in the stop handler while stopped */
    int priority;
    int system;			/* daemon thread */
    int running;		/* past the start gate, guarded by stop_lock */
    int want_stop;		/* STOP_* bits, guarded by stop_lock */
    volatile sig_atomic_t stop_seq, ack_seq;	/* stop handshake */
    void *(*start)(void *);
    volatile int stop_pending;	/* Thread.stop() posted an exception */
    struct sys_mon *volatile waiting_on;	/* monitor it waits on, if any */
    struct sys_thread *next;	/* thread list, guarded by _queue_lock */
};

extern void intrInit(void);
extern void InitializeAsyncIO(void);
extern void InitializeMem(void);

/* sysMonitorWait's half of the Thread.stop() handshake (thread_md.c). */
extern int sysThreadSetWaitingOn(struct sys_thread *t, struct sys_mon *mon);

#endif /* sun_java_threads_md_h___ */
