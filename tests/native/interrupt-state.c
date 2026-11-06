#include <stdio.h>
#include <string.h>
#include "sys_api.h"
#include "threads_md.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

int main(void)
{
    sys_thread_t thread;
    memset(&thread, 0, sizeof(thread));
#ifdef WIN32
    thread.interrupt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    CHECK(thread.interrupt_event != NULL);
#endif
    CHECK(sysThreadIsInterrupted(&thread, 0) == 0);
    sysThreadInterrupt(&thread);
    CHECK(sysThreadIsInterrupted(&thread, 0) == 1);
    CHECK(sysThreadIsInterrupted(&thread, 2) == 1);
    CHECK(sysThreadIsInterrupted(&thread, 0) == 1);
#ifdef WIN32
    CHECK(WaitForSingleObject(thread.interrupt_event, 0) == WAIT_OBJECT_0);
#endif
    CHECK(sysThreadIsInterrupted(&thread, 1) == 1);
    CHECK(sysThreadIsInterrupted(&thread, 0) == 0);
#ifdef WIN32
    CHECK(WaitForSingleObject(thread.interrupt_event, 0) == WAIT_TIMEOUT);
    CloseHandle(thread.interrupt_event);
#endif
    return 0;
}
