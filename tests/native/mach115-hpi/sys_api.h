/* Only declarations required by the isolated Mach HPI contract tests. */
#ifndef TEST_MACH115_SYS_API_H
#define TEST_MACH115_SYS_API_H
#include "bool.h"
typedef struct sys_thread sys_thread_t;
typedef struct sys_mon sys_mon_t;
typedef void *stackp_t;
#define SYS_TLS_MONCACHE 8
sys_thread_t *sysThreadSelf(void);
#endif
