/* Allocation and enumeration contracts with test-only OS/VM doubles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static struct sys_mon queue;
sys_mon_t *_queue_lock = &queue;
int ActiveThreadCount, UserThreadCount;
static struct { unsigned char prefix[116]; void *data; } native[3];
static struct ur_cthread current;
static int locked, enters, fail_alloc, visits, stop_after;
static int expected_lock = 1;
static sys_thread_t *expected[3];
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
    CHECK(m == &queue && !locked);
    locked = 1; ++enters;
    return 0;
}
int sysMonitorExit(sys_mon_t *m)
{
    CHECK(m == &queue && locked);
    locked = 0;
    return 0;
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
    int i, cookie[3], stacks[3];
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
    for (i = 0; i < 3; ++i) {
        current.incarnation = (cthread_t)&native[i];
        CHECK(sysThreadSelf() == t[i]);
        native[i].data = NULL;
        free(t[i]);
    }
    puts("Mach thread allocation/enumeration contracts passed");
    return 0;
}

/* Inject allocation failure after CRT declarations, without changing their
 * linkage on Windows or adding an allocator hook to production code. */
#define malloc mach_test_malloc
#include "../../src/unix/java/runtime/thread_cthreads.c"
