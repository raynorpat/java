/* Test-only Mach ABI surface; no OS implementation or SDK is supplied here. */
#ifndef TEST_MACH_CTHREADS_H
#define TEST_MACH_CTHREADS_H
typedef struct ur_cthread { struct ur_cthread *next; void *incarnation; } *ur_cthread_t;
struct cthread_queue_item { struct cthread_queue_item *next; };
struct cthread_queue { struct cthread_queue_item *head, *tail; };
typedef struct mutex { int lock; char *name; } *mutex_t;
typedef struct condition { int lock; struct cthread_queue queue; char *name; } *condition_t;
ur_cthread_t ur_cthread_self(void);
void spin_lock(int *);
int mutex_try_lock(mutex_t);
void mutex_wait_lock(mutex_t);
int condition_wait_timeout(condition_t, mutex_t, unsigned int);
void condition_signal_timeout(condition_t);
void condition_broadcast_timeout(condition_t);
#endif
