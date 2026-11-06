/* Layout-only test substitute: no setjmp/longjmp implementation. */
#ifndef TEST_MACH115_SETJMP_H
#define TEST_MACH115_SETJMP_H
#ifdef MACH115_SDK_SETJMP
#include <bsd/i386/setjmp.h>
#else
typedef int jmp_buf[18]; /* Original i386 sigcontext, verified against SDK. */
#endif
#endif
