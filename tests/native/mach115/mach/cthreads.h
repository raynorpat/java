/* Test-only Mach ABI surface; no OS implementation or SDK is supplied here. */
#ifndef TEST_MACH_CTHREADS_H
#define TEST_MACH_CTHREADS_H
#include <setjmp.h>
typedef struct cthread *cthread_t;
typedef struct ur_cthread { struct ur_cthread *next; cthread_t incarnation; } *ur_cthread_t;
struct cthread_queue_item { struct cthread_queue_item *next; };
struct cthread_queue { struct cthread_queue_item *head, *tail; };
typedef struct mutex { int lock; char *name; } *mutex_t;
typedef struct condition { int lock; struct cthread_queue queue; char *name; } *condition_t;
#define cthread_self() (ur_cthread_self()->incarnation)
cthread_t cthread_fork(void *(*)(void *), void *);
void cthread_detach(cthread_t);
void *cthread_join(cthread_t);
void cthread_yield(void);
void cthread_exit(void *);
int cthread_priority(cthread_t, int, unsigned int);
ur_cthread_t ur_cthread_self(void);
void spin_lock(int *);
void cond_broadcast(condition_t);
int mutex_try_lock(mutex_t);
void mutex_wait_lock(mutex_t);
int condition_wait_timeout(condition_t, mutex_t, unsigned int);
void condition_signal_timeout(condition_t);
void condition_broadcast_timeout(condition_t);
#endif
