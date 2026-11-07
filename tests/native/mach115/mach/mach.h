/* Minimal test-only Mach thread API. */
#ifndef TEST_MACH115_MACH_H
#define TEST_MACH115_MACH_H
typedef unsigned int thread_t;
typedef int kern_return_t;
kern_return_t thread_suspend(thread_t);
kern_return_t thread_resume(thread_t);
typedef int *thread_info_t;
typedef unsigned int *thread_state_t;
kern_return_t thread_get_state(thread_t, int, thread_state_t, unsigned int *);
kern_return_t thread_info(thread_t, int, thread_info_t, unsigned int *);
#endif
