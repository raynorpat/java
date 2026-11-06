#include "sys_api.h"

/* Compile against the active HPI: these JDK 1.1.5 calls must be accepted.
 * This object is not linked; runtime regressions are covered by the VM tests. */
int hpi_signatures(sys_thread_t **thread, void *cookie, sys_mon_t *monitor)
{
    int result = sysThreadBootstrap(thread, cookie);
    if (result != SYS_OK)
        return result;
    return sysMonitorInit(monitor);
}
