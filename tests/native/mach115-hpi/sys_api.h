/* Only declarations required by the isolated Mach HPI contract tests. */
#ifndef TEST_MACH115_SYS_API_H
#define TEST_MACH115_SYS_API_H
#define _SYS_API_H_ /* Also suppress nested includes of the full platform HPI. */
#include "bool.h"
#include <stdint.h>
typedef struct sys_thread sys_thread_t;
typedef struct sys_mon sys_mon_t;
typedef void *stackp_t;
#define SYS_TLS_MONCACHE 8
sys_thread_t *sysThreadSelf(void);
int sysThreadAlloc(sys_thread_t **, stackp_t, void *);
int sysThreadFree(sys_thread_t *);
int sysThreadCreate(long, unsigned int, void *(*)(void *), sys_thread_t **, void *);
int sysThreadBootstrap(sys_thread_t **, void *);
void sysThreadExit(void);
void intrInit(void);
void sysThreadInitializeSystemThreads(void);
void sysThreadSetBackPtr(sys_thread_t *, void *);
void sysThreadInterrupt(sys_thread_t *);
void sysThreadPostException(sys_thread_t *, void *);
int sysThreadHasPendingException(sys_thread_t *);
int jni_AttachHostThread(sys_thread_t **);
void jni_DetachHostThread(void);
void sysThreadSetAtBreakpoint(sys_thread_t *);
int sysThreadSuspend(sys_thread_t *);
int sysThreadResume(sys_thread_t *);
int sysThreadGetPriority(sys_thread_t *, int *);
int sysThreadSetPriority(sys_thread_t *, int);
int sysThreadSingle(void);
void sysThreadMulti(void);
void *sysThreadStackPointer(sys_thread_t *);
void **sysThreadRegisters(sys_thread_t *, int *);
void sysAbort(void);
void sysThreadInit(sys_thread_t *, stackp_t);
stackp_t sysThreadStackBase(sys_thread_t *);
void sysThreadSetStackBase(sys_thread_t *, stackp_t);
void *sysThreadGetBackPtr(sys_thread_t *);
int sysThreadEnumerateOver(int (*)(sys_thread_t *, void *), void *);
int sysMonitorEnter(sys_mon_t *);
int sysMonitorExit(sys_mon_t *);
int sysMonitorNotify(sys_mon_t *);
int sysMonitorNotifyAll(sys_mon_t *);
bool_t sysMonitorInUse(sys_mon_t *);
int sysMonitorSizeof(void);
int sysMonitorInit(sys_mon_t *);
int sysMonitorDestroy(sys_mon_t *);
int sysMonitorWait(sys_mon_t *, int, bool_t);
bool_t sysMonitorEntered(sys_mon_t *);
int sysThreadIsInterrupted(sys_thread_t *, long);
int64_t sysTimeMillis(void);
#define SYS_OK 0
#define SYS_NOMEM -5
#define SYS_ERR -1
#define SYS_NORESOURCE -6
#define SYS_INTRPT -2
#define SYS_TIMEOUT -3
#endif
