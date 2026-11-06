/* Only declarations required by the isolated Mach HPI contract tests. */
#ifndef TEST_MACH115_SYS_API_H
#define TEST_MACH115_SYS_API_H
#include "bool.h"
typedef struct sys_thread sys_thread_t;
typedef struct sys_mon sys_mon_t;
typedef void *stackp_t;
#define SYS_TLS_MONCACHE 8
sys_thread_t *sysThreadSelf(void);
int sysThreadAlloc(sys_thread_t **, stackp_t, void *);
void sysThreadInit(sys_thread_t *, stackp_t);
stackp_t sysThreadStackBase(sys_thread_t *);
void sysThreadSetStackBase(sys_thread_t *, stackp_t);
void *sysThreadGetBackPtr(sys_thread_t *);
int sysThreadEnumerateOver(int (*)(sys_thread_t *, void *), void *);
int sysMonitorEnter(sys_mon_t *);
int sysMonitorExit(sys_mon_t *);
#define SYS_OK 0
#define SYS_NOMEM -5
#endif
