/* Allocation and enumeration contracts with test-only OS/VM doubles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "java_lang_Thread.h"
#include "monitor_cache.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static struct sys_mon queue;
sys_mon_t *_queue_lock = &queue;
int ActiveThreadCount, UserThreadCount;
static struct { unsigned char prefix[116]; void *data; } native[3];
static struct ur_cthread current;
static int locked, enters, fail_alloc, visits, stop_after;
static int expected_lock = 1;
static sys_thread_t *expected[3];
static struct { unsigned int key; monitor_t *next; sys_mon_t mid; } join_monitor;
static sys_thread_t *dying;
static int monitor_present, monitor_in_use, join_locked, deleted, notified, freed;
static int queue_notified;
static ExecEnv *saved_ee;
extern int sysThreadEnumerateOver_unlocked(int (*)(sys_thread_t *, void *), void *);

void *mach_test_malloc(size_t size)
{
    CHECK(size == sizeof(sys_thread_t));
    if (fail_alloc) return NULL;
    return calloc(1, size);
}
ur_cthread_t ur_cthread_self(void) { return &current; }
int sysMonitorEnter(sys_mon_t *m)
{
    if (m == &join_monitor.mid) {
        CHECK(!locked && !join_locked && deleted == (saved_ee != NULL));
        join_locked = 1; return 0;
    }
    CHECK(m == &queue && !locked);
    if (dying && dying->cookie) {
        Hjava_lang_Thread *h = (Hjava_lang_Thread *)dying->cookie;
        CHECK(h->obj->PrivateInfo == 0 && h->obj->eetop == 0);
    }
    locked = 1; ++enters;
    return 0;
}
int sysMonitorExit(sys_mon_t *m)
{
    if (m == &join_monitor.mid) {
        CHECK(join_locked); join_locked = 0; return 0;
    }
    CHECK(m == &queue && locked);
    locked = 0;
    return 0;
}
int sysMonitorNotify(sys_mon_t *m)
{
    CHECK(m == &queue && locked && dying && !dying->system_thread);
    ++queue_notified; return 0;
}
int sysMonitorNotifyAll(sys_mon_t *m)
{
    CHECK(m == &join_monitor.mid && join_locked && m->mwait_queue.count > 0);
    ++notified; return 0;
}
bool_t sysMonitorInUse(sys_mon_t *m)
{
    CHECK(m == &join_monitor.mid && !locked);
    return monitor_in_use ? TRUE : FALSE;
}
monitor_t *lookupMonitor(unsigned int key)
{
    CHECK(dying && key == (unsigned int)dying->cookie && !locked);
    CHECK(deleted == (saved_ee != NULL));
    return monitor_present ? (monitor_t *)&join_monitor : NULL;
}
void DeleteExecEnv(ExecEnv *ee)
{
    CHECK(ee == saved_ee && !locked && !deleted && dying->next == NULL);
    ++deleted;
}
static int check_removed(sys_thread_t *t, void *arg)
{
    CHECK(t != dying); ++*(int *)arg; return 0;
}
static void mach_test_free(void *p)
{
    int count = 0;
    CHECK(p == dying && !locked && !join_locked && !dying->cacheKey);
    CHECK(dying->next == NULL && sysThreadSelf() == NULL);
    CHECK(sysThreadEnumerateOver_unlocked(check_removed, &count) == 0);
    CHECK(count == ActiveThreadCount);
    ++freed;
    free(p);
}
static int visit(sys_thread_t *t, void *arg)
{
    CHECK(locked == expected_lock && arg == &visits);
    CHECK(t == expected[visits]);
    ++visits;
    return visits == stop_after ? 37 : 0;
}
int main(void)
{
    sys_thread_t *t[3], *out;
    struct sys_thread initial;
    int i, stacks[3], order[3];
    Classjava_lang_Thread objects[3];
    Hjava_lang_Thread cookie[3];
    memset(objects, 0, sizeof(objects));
    memset(cookie, 0, sizeof(cookie));
    for (i = 0; i < 3; ++i) cookie[i].obj = &objects[i];
    CHECK(sizeof(void *) == 4 && sizeof(native[0]) == 120);
    current.incarnation = (cthread_t)&native[0];
    CHECK(sysThreadSelf() == NULL);
    CHECK(sysThreadEnumerateOver(visit, &visits) == 0 && visits == 0);
    fail_alloc = 1; out = &initial;
    CHECK(sysThreadAlloc(&out, stacks, cookie) == SYS_NOMEM);
    CHECK(out == &initial && !sysThreadSelf());
    CHECK(ActiveThreadCount == 0 && UserThreadCount == 0 && enters == 1);
    fail_alloc = 0;
    for (i = 0; i < 3; ++i) {
        current.incarnation = (cthread_t)&native[i];
        CHECK(sysThreadAlloc(&t[i], &stacks[i], &cookie[i]) == SYS_OK);
        CHECK(sysThreadSelf() == t[i]);
        CHECK(ActiveThreadCount == i+1 && UserThreadCount == i+1 && !locked);
        memset(&initial, 0, sizeof(initial));
        initial.next = i ? t[i-1] : NULL;
        initial.cookie = &cookie[i]; initial.cthread = (cthread_t)&native[i];
        initial.stack_base = &stacks[i]; initial.state = RUNNABLE;
        initial.run_static_initializers = TRUE;
        CHECK(memcmp(t[i], &initial, sizeof(initial)) == 0);
        CHECK(sysThreadGetBackPtr(t[i]) == &cookie[i]);
        CHECK(sysThreadStackBase(t[i]) == &stacks[i]);
        sysThreadInit(t[i], &stacks[0]);
        CHECK(sysThreadStackBase(t[i]) == &stacks[0]);
        sysThreadSetStackBase(t[i], &stacks[i]);
        CHECK(sysThreadStackBase(t[i]) == &stacks[i]);
    }
    for (i = 0; i < 3; ++i) expected[i] = t[2-i];
    CHECK(sysThreadEnumerateOver(visit, &visits) == 0 && visits == 3 && !locked);
    visits = 0; stop_after = 2;
    CHECK(sysThreadEnumerateOver(visit, &visits) == 37 && visits == 2 && !locked);
    visits = 0; stop_after = 0; expected_lock = 0; i = enters;
    CHECK(sysThreadEnumerateOver_unlocked(visit, &visits) == 0 && visits == 3);
    CHECK(enters == i && !locked);
    fail_alloc = 1; out = &initial;
    CHECK(sysThreadAlloc(&out, NULL, NULL) == SYS_NOMEM && out == &initial);
    CHECK(sysThreadSelf() == t[2] && ActiveThreadCount == 3 && UserThreadCount == 3);
    CHECK(enters == i);
    /* Remove middle, tail, then head. Cover no monitor, idle monitor,
     * and a live monitor with joiners, plus an attached thread without Java state. */
    order[0] = 1; order[1] = 0; order[2] = 2;
    for (i = 0; i < 3; ++i) {
        int index = order[i];
        current.incarnation = (cthread_t)&native[index];
        dying = t[index];
        saved_ee = i == 0 ? NULL : (ExecEnv *)&stacks[index];
        objects[index].PrivateInfo = (long)dying;
        objects[index].eetop = (long)saved_ee;
        dying->cacheKey = 123;
        deleted = notified = 0;
        monitor_present = i != 0; monitor_in_use = i == 2;
        join_monitor.mid.mwait_queue.count = 1;
        CHECK(sysThreadFree(dying) == SYS_OK);
        dying = NULL;
        CHECK(!native[index].data && freed == i+1);
        CHECK(ActiveThreadCount == 2-i && UserThreadCount == 2-i);
        CHECK(queue_notified == i+1 && notified == (i == 2));
    }
    fail_alloc = 0;
    current.incarnation = (cthread_t)&native[0];
    CHECK(sysThreadAlloc(&out, NULL, &cookie[0]) == SYS_OK);
    dying = out; saved_ee = NULL; deleted = notified = 0;
    monitor_present = monitor_in_use = 1;
    join_monitor.mid.mwait_queue.count = 0;
    CHECK(sysThreadFree(dying) == SYS_OK);
    dying = NULL;
    CHECK(!notified && freed == 4 && queue_notified == 4);
    CHECK(sysThreadAlloc(&out, NULL, NULL) == SYS_OK);
    dying = out; saved_ee = NULL; deleted = 0;
    /* The queue's daemon accounting normally changes at thread creation. */
    dying->system_thread = TRUE; --UserThreadCount;
    CHECK(sysThreadFree(dying) == SYS_OK);
    dying = NULL;
    CHECK(ActiveThreadCount == 0 && UserThreadCount == 0 && queue_notified == 4);
    CHECK(sysThreadSelf() == NULL && freed == 5);
    puts("Mach thread allocation/enumeration/teardown contracts passed");
    return 0;
}

/* Inject allocation failure after CRT declarations, without changing their
 * linkage on Windows or adding an allocator hook to production code. */
#define malloc mach_test_malloc
#define free mach_test_free
#include "../../src/unix/java/runtime/thread_cthreads.c"
