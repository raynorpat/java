/*
 * @(#)threads_md.h	1.19 97/10/08
 * 
 * Copyright (c) 1995, 1996 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * This software is the confidential and proprietary information of Sun
 * Microsystems, Inc. ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Sun.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES
 * SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
 * THIS SOFTWARE OR ITS DERIVATIVES.
 * 
 * CopyrightVersion 1.1_beta
 * 
 */

/*
 * Implementation of the Java threads HPI on top of RHAPSODY cthreads
 */

#ifndef _RHAPSODY_THREADS_MD_H_
#define _RHAPSODY_THREADS_MD_H_

/*
 * Include the RHAPSODY cthreads definition of a thread structure
 */
#include <mach/cthreads.h>

/*
 * Thread C stack overflow check
#define sysThreadCheckStack(redzone, var)                               \
    ((var = sysThreadSelf()) &&                                         \
      (unsigned int)((char *)&(var) - (char *)(var)->stack_base)\
                < (redzone))
*/
/*
 * Forward definition of machine dependent monitor struct
 */
struct sys_mon;

/*
 * The following thread states are really hints since there is no
 * interface to get at a native thread's true state. The states
 * are maintained by updating the states where ever possible
 * such as a transition to CONDVAR_WAIT occurs in the definition of
 * the routine condvarWait().
 * This state maintenance should disappear once we have a threads interface
 * to obtain a thread's state.
 * So, the states RUNNABLE and MONITOR_SUSPENDED are never reached since
 * they cannot be correctly maintained above the OS threads interface.
 * A new state (beyond that of green threads) called RUNNING has been
 * added. If a thread is not in MONITOR_WAIT or CONDVAR_WAIT or SUSPENDED, then
 * it is in RUNNING state.
 */
typedef enum {
    RUNNABLE,
    RUNNING,
    SUSPENDED,
    MONITOR_WAIT,
    CONDVAR_WAIT,
    MONITOR_SUSPENDED,
    RUNNING_AT_BREAKPOINT,
    SUSPENDED_AT_BREAKPOINT,
    NUM_THREAD_STATES /* Obviously, this has to be the last entry! */
} native_thread_state_t;

/*
 * Machine dependent info in a sys_thread_t
 */
struct sys_thread {
    sys_thread_t *next;       /* next thread in active thread queue */
    void *cookie;             /* backpointer to the corresponding Java object */
    void *awt_cookie;         /* a cookie where AWT stores per thread info */
    cthread_t cthread;        /* native underlying cthread */
    stackp_t stack_base;      /* thread stack base, not logical stack base! */
    native_thread_state_t state;     /* current thread state */
    bool_t primordial_thread; /* current thread running from main */
    bool_t system_thread;     /* TRUE if a system thread */
    bool_t mpending_suspend;  /* TRUE if waiting for monitor and suspended */
    bool_t cpending_suspend;  /* TRUE if waiting for condition and suspended */
    bool_t interrupted;       /*  */
    bool_t run_static_initializers; /* TRUE if static initializers are allowed to be run in this thread */
    struct mutex mutex;       /* per thread lock to protect thread fields */
    /* ip, 03/19/97:
     * cthread_fork() is the only possibility to create cthreads. Since
     * sysThreadCreate expects the thread to be suspended on creation
     * we need to have a two stage start. 
     */
    struct mutex start_mutex;
    void *(*start_func)(void *);
    void *start_arg;
    /* ip, 03/21/97:
     * used for implementing "signals" to threads
     */
    bool_t jump_buf_valid;
    jmp_buf jump_buf;
    /* Monitor specific.
       Every monitor keeps track of the number of times it is
       entered.  When that count goes to 0, the monitor can be
       freed up.  But each thread has its own entry count on a
       particular monitor, because multiple threads can be using a
       single monitor (as one does a wait, another enters, etc.).
       Each thread can only be waiting in exactly one monitor.
       That monitor waited on is saved in mon_wait, and the value
       of the monitor's entry_count when the wait was performed is
       saved in monitor_entry_count.  That is restored into the
       monitor when this waiting thread is notified. */
    int monitor_entry_count;		/* For recursive monitor entry */
    sys_mon_t *mon_wait;		/* MONITOR_WAIT or CONDVAR_WAIT'ing */

    /*
     * Handling of thread local monitor caches.
     */
    unsigned int cacheKey; /* Do not collect the last used monitor from the global cache, even if it is not otherwise referenced. */
    void* monitorCache[SYS_TLS_MONCACHE]; /* Thread local monitor cache. */
} ;

bool_t threadsInitialized();

#define SYS_THREAD_NULL        	((sys_thread_t *) 0)

#endif /* !_RHAPSODY_THREADS_MD_H_ */
