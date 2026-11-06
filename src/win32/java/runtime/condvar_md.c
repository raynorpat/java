/* Reconstructed from JDK 1.1.5 javai_g.dll, VA 10014da0-100150af.
 * This is reconstructed source, not recovered original Sun source.
 * See tests/native/condvar-evidence.md for binary evidence and constraints.
 */
#include <windows.h>
#include <mmsystem.h>
#include "threads_md.h"
#include "condvar_md.h"

int
condvarInit(condvar_t *cv)
{
    HANDLE event;
    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (event == NULL) return 0;
    cv->event = event;
    return 1;
}

int
condvarDestroy(condvar_t *cv)
{
    return CloseHandle(cv->event);
}

void
condvarWait(struct sys_thread *self, condvar_t *cv, mutex_t *mutex)
{
    HANDLE handles[2];
    unsigned int generation;
    int retry;
    DWORD result;
    generation = cv->generation;
    retry = 0;
    handles[0] = cv->event;
    handles[1] = self->interrupt_event;
    ++cv->waiters;
    do {
        LeaveCriticalSection(mutex);
        if (retry) Sleep(1);
        else retry = 1;
        result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        EnterCriticalSection(mutex);
        if (result == WAIT_OBJECT_0 + 1) {
            --cv->waiters;
            return;
        }
    } while (cv->signals == 0 || cv->generation == generation);
    --cv->waiters;
    --cv->signals;
    if (cv->signals == 0) ResetEvent(cv->event);
}

void
condvarTimedWait(struct sys_thread *self, condvar_t *cv, mutex_t *mutex,
                 DWORD milliseconds)
{
    HANDLE handles[2];
    unsigned int generation;
    int retry;
    DWORD result, previous, now, elapsed;
    generation = cv->generation;
    retry = 0;
    handles[0] = cv->event;
    handles[1] = self->interrupt_event;
    ++cv->waiters;
    previous = timeGetTime();
    for (;;) {
        LeaveCriticalSection(mutex);
        if (retry) Sleep(1);
        else retry = 1;
        result = WaitForMultipleObjects(2, handles, FALSE, milliseconds);
        EnterCriticalSection(mutex);
        if (result == WAIT_OBJECT_0 + 1) {
            --cv->waiters;
            return;
        }
        if (cv->signals != 0 && cv->generation != generation) break;
        now = timeGetTime();
        /* Preserve the binary's clock-wrap arithmetic, including its quirk. */
        if (now < previous) elapsed = 0xffffffffUL - now + previous;
        else elapsed = now - previous;
        if (elapsed >= milliseconds) {
            --cv->waiters;
            return;
        }
        milliseconds -= elapsed;
        previous = now;
    }
    --cv->waiters;
    --cv->signals;
    if (cv->signals == 0) ResetEvent(cv->event);
}

int
condvarSignal(condvar_t *cv)
{
    if (cv->waiters > cv->signals) {
        if (!SetEvent(cv->event)) return 0;
        ++cv->signals;
        ++cv->generation;
    }
    return 1;
}

int
condvarBroadcast(condvar_t *cv)
{
    if (cv->waiters != 0) {
        if (!SetEvent(cv->event)) return 0;
        cv->signals = cv->waiters;
        ++cv->generation;
    }
    return 1;
}
