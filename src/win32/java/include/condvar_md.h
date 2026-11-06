/* Reconstructed from JDK 1.1.5 javai_g.dll; not an original Sun header.
 * Field names are descriptive reconstructions. See condvar-evidence.md.
 */
#ifndef _WIN32_CONDVAR_MD_H_
#define _WIN32_CONDVAR_MD_H_

#include <windows.h>
#include "mutex_md.h"

struct sys_thread;

typedef struct {
    HANDLE event;
    unsigned int generation;
    unsigned int waiters;
    unsigned int signals;
} condvar_t;

/* Storage must be zeroed before initialization. All operations except
 * init/destroy require the associated mutex held exactly once.
 */
extern int condvarInit(condvar_t *);
extern int condvarDestroy(condvar_t *);
extern void condvarWait(struct sys_thread *, condvar_t *, mutex_t *);
extern void condvarTimedWait(struct sys_thread *, condvar_t *, mutex_t *, DWORD);
extern int condvarSignal(condvar_t *);
extern int condvarBroadcast(condvar_t *);

#endif /* _WIN32_CONDVAR_MD_H_ */
