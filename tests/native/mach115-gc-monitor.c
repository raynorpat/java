/* Real thread/monitor composition with a peer stopped inside the native lock. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include <mach/mach.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static sys_mon_t queue;
sys_mon_t *_queue_lock = &queue;
int ActiveThreadCount, UserThreadCount;
static unsigned int native[2][32];
static struct ur_cthread current;
static int stopped, visited;
ur_cthread_t ur_cthread_self(void) { return &current; }
int mutex_try_lock(mutex_t m) { CHECK(!m->lock); m->lock = 1; return 1; }
void mutex_wait_lock(mutex_t m) { CHECK(0); }
int condvarInit(condition_t c) { memset(c, 0, sizeof(*c)); return 0; }
int condvarWait(condition_t c, mutex_t m, int state) { CHECK(0); return SYS_ERR; }
int condvarSignal(condition_t c) { return 0; }
void sysAbort(void) { abort(); }
kern_return_t thread_suspend(thread_t t)
{
    CHECK(t == 2 && !stopped && queue.monitor_owner == sysThreadSelf());
    /* Simulate the peer frozen after native lock acquisition, before wait. */
    CHECK(!queue.mutex.lock); queue.mutex.lock = 1; stopped = 1; return 0;
}
kern_return_t thread_resume(thread_t t)
{ CHECK(t == 2 && stopped && queue.mutex.lock); queue.mutex.lock = 0; stopped = 0; return 0; }
kern_return_t thread_get_state(thread_t t, int flavor, thread_state_t state, unsigned int *count)
{ CHECK(t == 2 && stopped && *count == 16); memset(state, 0, 64); state[7] = 0x1234; return 0; }
static int visit(sys_thread_t *t, void *arg)
{ CHECK(stopped && queue.mutex.lock); ++visited; return 0; }
int main(void)
{
    sys_thread_t *self, *peer;
    native[0][1] = 1; native[1][1] = 2;
    CHECK(sysMonitorInit(&queue) == SYS_OK);
    current.incarnation = (cthread_t)native[0];
    CHECK(sysThreadAlloc(&self, (stackp_t)0x2000, NULL) == SYS_OK);
    current.incarnation = (cthread_t)native[1];
    CHECK(sysThreadAlloc(&peer, (stackp_t)0x2000, NULL) == SYS_OK);
    current.incarnation = (cthread_t)native[0];
    CHECK(sysMonitorEnter(&queue) == SYS_OK);
    CHECK(sysThreadSingle() == SYS_OK && stopped && queue.entry_count == 1);
    CHECK(sysThreadEnumerateOver(visit, NULL) == SYS_OK && visited == 2);
    CHECK(sysThreadStackPointer(peer) == (void *)0x1234);
    sysThreadMulti(); CHECK(!stopped && queue.entry_count == 1);
    CHECK(sysMonitorExit(&queue) == SYS_OK);
    puts("Mach GC/monitor suspension contracts passed");
    return 0;
}
