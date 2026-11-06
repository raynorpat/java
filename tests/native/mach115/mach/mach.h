/* Minimal test-only Mach thread API. */
#ifndef TEST_MACH115_MACH_H
#define TEST_MACH115_MACH_H
typedef unsigned int thread_t;
typedef int kern_return_t;
kern_return_t thread_suspend(thread_t);
#endif
