#include "sys_api.h"
#include "threads_md.h"
#include "java_lang_Thread.h"
#include <string.h>
#include <stdio.h>
extern long sun_tools_debug_Agent_getThreadStatus(void *, Hjava_lang_Thread *);
int main(void)
{
    Classjava_lang_Thread object;
    Hjava_lang_Thread handle;
    sys_thread_t system;
    int i;
    static const int expected[] = {1, 1, 5, 3, 4, 5, -1, 6};
    memset(&object, 0, sizeof(object));
    memset(&system, 0, sizeof(system));
    handle.obj = &object;
    if (sun_tools_debug_Agent_getThreadStatus(NULL, NULL) != -1 ||
        sun_tools_debug_Agent_getThreadStatus(NULL, &handle) != 0) return 1;
    object.PrivateInfo = (long)&system;
    for (i = 0; i < NUM_THREAD_STATES; i++) {
        system.state = (native_thread_state_t)i;
        if (sun_tools_debug_Agent_getThreadStatus(NULL, &handle) != expected[i]) return 2;
    }
    for (i = MONITOR_WAIT; i <= CONDVAR_WAIT; i++) {
        system.state = (native_thread_state_t)i;
        system.mon_wait = (sys_mon_t *)&system;
        if (sun_tools_debug_Agent_getThreadStatus(NULL, &handle) != 2) return 3;
        system.mpending_suspend = TRUE;
        if (sun_tools_debug_Agent_getThreadStatus(NULL, &handle) != 5) return 4;
        system.mpending_suspend = FALSE;
        system.cpending_suspend = TRUE;
        if (sun_tools_debug_Agent_getThreadStatus(NULL, &handle) != 5) return 5;
        system.cpending_suspend = FALSE;
        system.mon_wait = NULL;
    }
    puts("DR2 debugger thread states passed");
    return 0;
}
