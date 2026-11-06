#include "sys_api.h"
#include <stddef.h>
#include "java_lang_Thread.h"
#include "java_lang_ThreadGroup.h"

typedef char thread_group_offset[offsetof(Classjava_lang_Thread, group) == 36 ? 1 : -1];
typedef char group_threads_offset[offsetof(Classjava_lang_ThreadGroup, nthreads) == 24 ? 1 : -1];

/* Compile against the active HPI: these JDK 1.1.5 calls must be accepted.
 * This object is not linked; runtime regressions are covered by the VM tests. */
int hpi_signatures(sys_thread_t **thread, void *cookie, sys_mon_t *monitor)
{
    int result = sysThreadBootstrap(thread, cookie);
    if (result != SYS_OK)
        return result;
    return sysMonitorInit(monitor);
}
