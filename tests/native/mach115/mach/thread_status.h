/* Test-only i386 declarations exposed by Mach's guarded thread_status.h. */
#ifndef TEST_MACH115_THREAD_STATUS_H
#define TEST_MACH115_THREAD_STATUS_H
#define i386_THREAD_STATE -1
typedef struct {
    unsigned int eax, ebx, ecx, edx, edi, esi, ebp, esp;
    unsigned int ss, eflags, eip, cs, ds, es, fs, gs;
} i386_thread_state_t;
#define i386_THREAD_STATE_COUNT 16
#endif
