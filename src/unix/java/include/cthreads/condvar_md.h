/* Recovered from Rhapsody libjava.A.dylib 5C13660C-5C1366EC.
 * The framework's monitor_md.h references this unshipped private header.
 */
#ifndef RHAPSODY_CONDVAR_MD_H
#define RHAPSODY_CONDVAR_MD_H
#include <mach/cthreads.h>
int condvarInit(condition_t);
int condvarDestroy(condition_t);
int condvarWait(condition_t, mutex_t, int);
int condvarTimedWait(condition_t, mutex_t, unsigned int, int);
int condvarSignal(condition_t);
int condvarBroadcast(condition_t);
#endif
