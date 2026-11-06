#ifndef sun_java_threads_md_h___
#define sun_java_threads_md_h___

/*
 * Win32 native threads (md/thread_md.c). Windows types are spelled as
 * their underlying C types so this header does not pull in <windows.h>.
 */
struct sys_mon;

struct sys_thread {
    void *handle;		/* HANDLE usable from other threads */
    unsigned long id;		/* Win32 thread id */
    void *cookie;		/* back pointer to the Java thread object */
    void *stack_base;		/* highest address scanned by the GC */
    void *stack_ptr;		/* ESP while suspended for GC */
    void *regs[7];		/* integer registers while suspended for GC */
    int priority;
    int system;			/* daemon thread */
    void *(*start)(void *);
    volatile int stop_pending;	/* Thread.stop() posted an exception */
    struct sys_mon *volatile waiting_on;	/* monitor it waits on, if any */
    struct sys_thread *next;	/* thread list, guarded by _queue_lock */
};

extern void intrInit(void);
extern void InitializeAsyncIO(void);
extern void InitializeMem(void);

#endif /* sun_java_threads_md_h___ */
