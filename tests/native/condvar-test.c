/* Native behavioral checks for the reconstructed Win32 condition variable. */
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads_md.h"
#include "condvar_md.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); exit(1); } } while (0)

typedef char condvar_size_check[sizeof(condvar_t) == 16 ? 1 : -1];
typedef char condvar_generation_check[offsetof(condvar_t, generation) == 4 ? 1 : -1];
typedef char condvar_waiters_check[offsetof(condvar_t, waiters) == 8 ? 1 : -1];
typedef char condvar_signals_check[offsetof(condvar_t, signals) == 12 ? 1 : -1];

static CRITICAL_SECTION mutex;
static condvar_t cv;
static sys_thread_t self;
static LONG completed;

static DWORD WINAPI waiter(void *arg)
{
    EnterCriticalSection(&mutex);
    condvarWait(&self, &cv, &mutex);
    ++completed;
    LeaveCriticalSection(&mutex);
    return 0;
}

static void await_waiters(unsigned int count)
{
    DWORD start;
    unsigned int observed;
    start = GetTickCount();
    do {
        EnterCriticalSection(&mutex);
        observed = cv.waiters;
        LeaveCriticalSection(&mutex);
        if (observed == count) return;
        Sleep(1);
    } while (GetTickCount() - start < 5000);
    CHECK(0);
}

int main(void)
{
    HANDLE threads[2];
    DWORD start;
    InitializeCriticalSection(&mutex);
    self.interrupt_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    CHECK(self.interrupt_event != NULL);
    CHECK(condvarInit(&cv));
    CHECK(WaitForSingleObject(cv.event, 0) == WAIT_TIMEOUT);
    CHECK(condvarSignal(&cv));
    CHECK(condvarBroadcast(&cv));
    CHECK(cv.generation == 0 && cv.signals == 0);
    EnterCriticalSection(&mutex);
    start = GetTickCount();
    condvarTimedWait(&self, &cv, &mutex, 40);
    CHECK(GetTickCount() - start >= 30);
    CHECK(cv.waiters == 0 && cv.signals == 0);
    LeaveCriticalSection(&mutex);
    threads[0] = CreateThread(NULL, 0, waiter, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, waiter, NULL, 0, NULL);
    CHECK(threads[0] != NULL && threads[1] != NULL);
    await_waiters(2);
    EnterCriticalSection(&mutex);
    CHECK(condvarSignal(&cv));
    LeaveCriticalSection(&mutex);
    await_waiters(1);
    EnterCriticalSection(&mutex);
    CHECK(completed == 1 && cv.signals == 0);
    CHECK(WaitForSingleObject(cv.event, 0) == WAIT_TIMEOUT);
    CHECK(condvarBroadcast(&cv));
    LeaveCriticalSection(&mutex);
    CHECK(WaitForMultipleObjects(2, threads, TRUE, 5000) == WAIT_OBJECT_0);
    CHECK(completed == 2 && cv.waiters == 0 && cv.signals == 0);
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    threads[0] = CreateThread(NULL, 0, waiter, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, waiter, NULL, 0, NULL);
    CHECK(threads[0] != NULL && threads[1] != NULL);
    await_waiters(2);
    EnterCriticalSection(&mutex);
    CHECK(condvarBroadcast(&cv));
    CHECK(cv.signals == 2);
    start = cv.generation;
    CHECK(condvarSignal(&cv));
    CHECK(cv.generation == start && cv.signals == 2);
    LeaveCriticalSection(&mutex);
    CHECK(WaitForMultipleObjects(2, threads, TRUE, 5000) == WAIT_OBJECT_0);
    CHECK(completed == 4 && cv.waiters == 0 && cv.signals == 0);
    CHECK(WaitForSingleObject(cv.event, 0) == WAIT_TIMEOUT);
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    SetEvent(self.interrupt_event);
    EnterCriticalSection(&mutex);
    condvarWait(&self, &cv, &mutex);
    condvarTimedWait(&self, &cv, &mutex, 5000);
    CHECK(cv.waiters == 0);
    CHECK(WaitForSingleObject(self.interrupt_event, 0) == WAIT_OBJECT_0);
    LeaveCriticalSection(&mutex);
    CHECK(condvarDestroy(&cv));
    CloseHandle(self.interrupt_event);
    DeleteCriticalSection(&mutex);
    puts("condvar-test: passed");
    return 0;
}
