/* Monitor integration with real Win32 synchronization and stub thread identity. */
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sys_api.h"
#include "monitor_md.h"

#ifdef DEBUG
/* This isolated HPI probe has no VM thread registry to dump. */
void DumpThreads(void) {}
void panic(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}
#endif

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); exit(1); } } while (0)

typedef char monitor_size_check[sizeof(sys_mon_t) == 48 ? 1 : -1];
typedef char monitor_cv_check[offsetof(sys_mon_t, condvar) == 24 ? 1 : -1];
typedef char monitor_owner_check[offsetof(sys_mon_t, monitor_owner) == 40 ? 1 : -1];
typedef char monitor_count_check[offsetof(sys_mon_t, entry_count) == 44 ? 1 : -1];

bool_t ThreadsInitialized;
static __declspec(thread) sys_thread_t *current;
static sys_mon_t mon;
static sys_thread_t main_thread;

sys_thread_t *sysThreadSelf(void) { return current; }

int sysThreadIsInterrupted(sys_thread_t *thread, long clear)
{
    int interrupted;
    interrupted = thread->interrupted;
    if (clear == 1) {
        thread->interrupted = FALSE;
        ResetEvent(thread->interrupt_event);
    }
    return interrupted;
}

static DWORD WINAPI notifier(void *arg)
{
    sys_thread_t thread;
    memset(&thread, 0, sizeof(thread));
    current = &thread;
    CHECK(sysMonitorEnter(&mon) == SYS_OK);
    CHECK(mon.condvar.waiters == 1);
    if (arg != NULL) {
        main_thread.interrupted = TRUE;
        SetEvent(main_thread.interrupt_event);
    } else {
        CHECK(sysMonitorNotify(&mon) == SYS_OK);
    }
    CHECK(sysMonitorExit(&mon) == SYS_OK);
    return 0;
}

int main(void)
{
    HANDLE worker;
    CHECK(sysMonitorSizeof() == 48);
    CHECK(sysMonitorInit(&mon) == SYS_OK);
    CHECK(sysMonitorEntered(&mon));
    CHECK(sysMonitorEnter(&mon) == SYS_OK);
    CHECK(sysMonitorExit(&mon) == SYS_OK);
    CHECK(!sysMonitorInUse(&mon));
    ThreadsInitialized = TRUE;
    CHECK(sysMonitorEnter(&mon) == SYS_ERR);
    CHECK(sysMonitorExit(&mon) == SYS_ERR);
    CHECK(!sysMonitorEntered(&mon));
    current = &main_thread;
    main_thread.interrupt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    CHECK(main_thread.interrupt_event != NULL);
    CHECK(sysMonitorNotify(&mon) == SYS_ERR);
    CHECK(sysMonitorWait(&mon, 0, TRUE) == SYS_ERR);
    CHECK(sysMonitorEnter(&mon) == SYS_OK);
    CHECK(sysMonitorEnter(&mon) == SYS_OK);
    CHECK(mon.entry_count == 2 && sysMonitorInUse(&mon));
    CHECK(sysMonitorWait(&mon, 5, TRUE) == SYS_OK);
    CHECK(mon.entry_count == 2 && mon.monitor_owner == current);
    worker = CreateThread(NULL, 0, notifier, NULL, 0, NULL);
    CHECK(worker != NULL);
    CHECK(sysMonitorWait(&mon, SYS_TIMEOUT_INFINITY, TRUE) == SYS_OK);
    CHECK(mon.entry_count == 2 && main_thread.state == RUNNABLE);
    CHECK(WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0);
    CloseHandle(worker);
    main_thread.interrupted = TRUE;
    SetEvent(main_thread.interrupt_event);
    CHECK(sysMonitorWait(&mon, 0, FALSE) == SYS_INTRPT);
    CHECK(main_thread.interrupted);
    CHECK(sysMonitorWait(&mon, 0, TRUE) == SYS_INTRPT);
    CHECK(!main_thread.interrupted && mon.entry_count == 2);
    worker = CreateThread(NULL, 0, notifier, &mon, 0, NULL);
    CHECK(worker != NULL);
    CHECK(sysMonitorWait(&mon, SYS_TIMEOUT_INFINITY, TRUE) == SYS_INTRPT);
    CHECK(!main_thread.interrupted && mon.entry_count == 2);
    CHECK(WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0);
    CloseHandle(worker);
    CHECK(sysMonitorExit(&mon) == SYS_OK);
    CHECK(sysMonitorEntered(&mon));
    CHECK(sysMonitorExit(&mon) == SYS_OK);
    CHECK(!sysMonitorEntered(&mon) && !sysMonitorInUse(&mon));
    CHECK(sysMonitorExit(&mon) == SYS_ERR);
    mon.condvar.waiters = 1;
    CHECK(sysMonitorInUse(&mon));
    mon.condvar.waiters = 0;
    mon.condvar.signals = 1;
    CHECK(sysMonitorInUse(&mon));
    mon.condvar.signals = 0;
    CHECK(sysMonitorDestroy(&mon) == SYS_OK);
    CloseHandle(main_thread.interrupt_event);
    puts("monitor-test: passed");
    return 0;
}
