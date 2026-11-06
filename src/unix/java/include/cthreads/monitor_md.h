/*
 * @(#)monitor_md.h	1.22 97/02/17
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
 * Monitor interface	10/25/94
 *
 * Private data structures and interfaces used in the monitor code.
 * This file is used to share declarations and such between the different
 * files implementing monitors.  It does not contain exported API.
 */

#ifndef	_RHAPSODY_MONITOR_MD_H_
#define	_RHAPSODY_MONITOR_MD_H_

#include "condvar_md.h"
#include "threads_md.h"

/*
 * Type definitions.
 */

typedef struct monitor_waiter monitor_waiter_t;
typedef struct monitor_wait_queue monitor_wait_queue_t;

/* Element of the MonitorWaitQ - representing thread doing a monitor wait */
/*
 * The only reason we do the queueing is for sysMonitorDumpInfo.
 * The counting, though, is used to avoid extraneous calls to
 * condvarBroadcast and condvarSignal, for instance.
 */
struct monitor_waiter {
    monitor_waiter_t   *next;
    monitor_waiter_t  **prev;
    sys_thread_t       *waiting_thread;
};

struct monitor_wait_queue {
    monitor_waiter_t   *head;		/* linked list of waiting threads */
    short		count;		/* number of waiters on the list */
};

/* C89 spelling of the original disabled macro:
#define ANY_WAITING(mwq) ((mwq).count > 0)
*/
#define INIT_MONITOR_WAIT_QUEUE(mwq) { (mwq).head = NULL; (mwq).count = 0; }

/* The system-level monitor data structure */
struct sys_mon {
    struct mutex     	mutex;          /* The monitor's mutex */
    long        	entry_count;    /* Recursion depth */
    struct condition   	cv_waiters;     /* Awaken those waiting to enter the
					   monitor */
    struct condition	cv_monitor;	/* Notify those doing monitorWait on
					   the monitor */
    /*
     * Threads waiting on either of the above condvars put themselves
     * on one of these lists.
     */
    monitor_wait_queue_t waiters_queue;	/* Head of MonitorEntryQ */
    monitor_wait_queue_t mwait_queue;	/* Head of MonitorWaitQ */

    /* Thread currently executing in this monitor */
    sys_thread_t 	*monitor_owner;

    int                 oldPriority;    /* For priority inversion */
};

typedef enum {
	ASYNC_REGISTER,
	ASYNC_UNREGISTER
} async_action_t;

/*
 * Macros
 */

#define SYS_MID_NULL ((sys_mon_t *) 0)

/*
 * Symbolic constant to be used when waiting indefinitly on a condition
 * variable
 */
#define SYS_TIMEOUT_INFINITY -1

typedef enum {
	SYS_ASYNC_MON_ALARM = 1,
 	SYS_ASYNC_MON_IO,
 	SYS_ASYNC_MON_EVENT,
	SYS_ASYNC_MON_CHILD,
 	SYS_ASYNC_MON_MAX
} async_mon_key_t;

#define SYS_ASYNC_MON_INPUT SYS_ASYNC_MON_IO
#define SYS_ASYNC_MON_OUTPUT SYS_ASYNC_MON_IO

sys_mon_t *asyncMon(async_mon_key_t);

#endif	/* _RHAPSODY_MONITOR_MD_H_ */

