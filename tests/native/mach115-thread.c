/* Allocation and enumeration contracts with test-only OS/VM doubles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <signal.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "cthreads/runtime.h"
#include "java_lang_Thread.h"
#include "monitor_cache.h"
#include <mach/mach.h>
#include <mach/thread_info.h>
#include <mach/thread_status.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static struct sys_mon queue;
sys_mon_t *_queue_lock = &queue;
int ActiveThreadCount, UserThreadCount;
stackp_t mainstktop;
static struct native_thread {
    unsigned int next, real_thread;
    unsigned char prefix[108];
    void *data;
    int reserved, objc;
} native[3], child;
static struct ur_cthread current;
static int locked, enters, fail_alloc, visits, stop_after;
static int expected_lock = 1;
static sys_thread_t *expected[3];
static struct { unsigned int key; monitor_t *next; sys_mon_t mid; } join_monitor;
static sys_thread_t *dying;
static int monitor_present, monitor_in_use, join_locked, deleted, notified, freed;
static int queue_notified;
static ExecEnv *saved_ee;
static int creating, fail_fork, fail_suspend, forked, joined, detached, yielded;
static int started, suspended, force_wait, waited;
static int exited;
static int controlling, control_error, suspend_calls, resume_calls, priority_calls;
static sys_thread_t control_thread;
static native_thread_state_t control_state;
static int gc_mode, gc_oom, gc_fail_suspend, gc_fail_state, gc_short_state;
static int gc_fail_resume, gc_resume_calls, gc_free_calls, gc_abort_seen;
static int gc_depth[3], gc_expected_resumes;
static void *gc_allocation;
static int finalizers, dumps, region_error, port_freed;
static void *mapped_stack_top;
int task_self_ = 1;
void cthread_init(void) { }
void InitializeFinalizerThread(void) { ++finalizers; }
void DumpThreads(void) { ++dumps; }
void DumpMonitors(void) { ++dumps; }
int sysMonitorWait(sys_mon_t *m, int timeout, bool_t clear)
{ (void)m; (void)timeout; (void)clear; CHECK(0); return 0; }
kern_return_t vm_region(unsigned int task, vm_address_t *addr, vm_size_t *size,
                       vm_prot_t *prot, vm_prot_t *maxprot, vm_inherit_t *inherit,
                       boolean_t *shared, memory_object_name_t *object, vm_offset_t *offset)
{
    CHECK(task == 1);
    if (region_error) return 5;
    *addr &= ~4095U; *size = 8192; *prot = *maxprot = 3; *inherit = 0;
    *shared = 0; *object = 7; *offset = 0;
    mapped_stack_top = (void *)(*addr + *size); return 0;
}
kern_return_t port_deallocate(unsigned int task, int port)
{ CHECK(task == 1 && port == 7); ++port_freed; return 0; }
static sys_thread_t *pending;
static void *(*child_entry)(void *);
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
    if (gc_mode) { CHECK(m == &queue); ++locked; ++enters; return 0; }
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
    if (gc_mode) { --locked; return 0; }
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
    if (p == gc_allocation) {
        CHECK(gc_mode && gc_depth[1] == 1 && gc_depth[2] == 0);
        ++gc_free_calls; gc_allocation = NULL; free(p); return;
    }
    if (creating) {
        CHECK(p == pending && ActiveThreadCount == 0 && UserThreadCount == 0);
        CHECK(!child.data && !detached && (!fail_suspend || joined));
        CHECK(sysThreadEnumerateOver_unlocked(check_removed, &count) == 0 && !count);
        ++freed; free(p); return;
    }
    CHECK(p == dying && !locked && !join_locked && !dying->cacheKey);
    CHECK(dying->next == NULL && sysThreadSelf() == NULL);
    CHECK(sysThreadEnumerateOver_unlocked(check_removed, &count) == 0);
    CHECK(count == ActiveThreadCount);
    ++freed;
    free(p);
}
int mutex_try_lock(mutex_t m)
{
    if (force_wait) { force_wait = 0; return 0; }
    if (m->lock) return 0;
    m->lock = 1; return 1;
}
void mutex_wait_lock(mutex_t m)
{
    CHECK(!m->lock); ++waited; m->lock = 1;
}
cthread_t cthread_fork(void *(*entry)(void *), void *arg)
{
    pending = (sys_thread_t *)arg;
    CHECK(pending->start_mutex.lock && pending->state == SUSPENDED);
    CHECK(pending->run_static_initializers && !pending->jump_buf_valid);
    CHECK(!pending->next && !pending->stack_base && !pending->mutex.lock);
    CHECK(!ActiveThreadCount && !UserThreadCount);
    ++forked; child_entry = entry;
    return fail_fork ? NULL : (cthread_t)&child;
}
void cthread_yield(void)
{
    CHECK(pending->start_mutex.lock && !child.data && !child.real_thread);
    ++yielded; child.real_thread = 73;
}
kern_return_t thread_suspend(thread_t thread)
{
    if (gc_mode) {
        CHECK(locked >= 1 && thread >= 101 && thread <= 102);
        if ((int)thread == gc_fail_suspend) return 5;
        ++gc_depth[thread-100]; return 0;
    }
    if (controlling) {
        CHECK(thread == 97 && !control_thread.mutex.lock);
        CHECK(control_thread.state == control_state);
        ++suspend_calls; return control_error;
    }
    CHECK(thread == 73 && pending->start_mutex.lock && !child.data);
    CHECK(!ActiveThreadCount && !UserThreadCount && !detached);
    if (fail_suspend) return 5;
    suspended = 1; return 0;
}
kern_return_t thread_resume(thread_t thread)
{
    if (gc_mode) {
        CHECK(locked >= 1 && thread >= 101 && thread <= 102);
        ++gc_resume_calls;
        if ((int)thread == gc_fail_resume) return 5;
        --gc_depth[thread-100]; return 0;
    }
    CHECK(controlling && thread == 97 && !control_thread.mutex.lock);
    CHECK(control_thread.state == control_state);
    ++resume_calls; return control_error;
}
kern_return_t thread_get_state(thread_t thread, int flavor, thread_state_t state,
                               unsigned int *count)
{
    unsigned int i;
    CHECK(gc_mode && locked >= 1 && flavor == -1 && *count == 16);
    CHECK(thread >= 101 && thread <= 102 && gc_depth[thread-100] > 0);
    if ((int)thread == gc_fail_state) return 5;
    for (i = 0; i < 16; ++i) state[i] = thread*1000+i;
    if (gc_short_state) *count = 15;
    return 0;
}
static void *mach_test_calloc(size_t count, size_t size)
{
    CHECK(gc_mode && locked >= 1 && count == 2 && size >= 16*sizeof(void *));
    CHECK(gc_depth[1] == 1 && gc_depth[2] == 0 && !gc_allocation);
    if (gc_oom) return NULL;
    gc_allocation = calloc(count, size);
    CHECK(gc_allocation != NULL); return gc_allocation;
}
void sysAbort(void)
{
    CHECK(gc_mode && gc_fail_resume && gc_resume_calls == gc_expected_resumes);
    CHECK(gc_allocation != NULL); ++gc_abort_seen;
    /* JNI abort hooks are allowed to return: the GC path must still stop. */
}
static void mach_test_abort(void)
{
    CHECK(gc_abort_seen == 1);
    puts("GC resume failure stopped after attempting all owned suspensions");
    exit(0);
}
kern_return_t thread_info(thread_t thread, int flavor, thread_info_t info,
                          unsigned int *count)
{
    thread_basic_info_data_t *basic = (thread_basic_info_data_t *)info;
    CHECK(thread == 97 && flavor == THREAD_BASIC_INFO && *count == 11);
    ++priority_calls;
    if (control_error) return control_error;
    memset(basic, 0, sizeof(*basic)); basic->base_priority = 11;
    basic->cur_priority = 17;
    return 0;
}
int cthread_priority(cthread_t t, int priority, unsigned int set_max)
{
    CHECK(t == control_thread.cthread && priority == 8 && set_max == 0);
    ++priority_calls; return control_error;
}
void cthread_detach(cthread_t t)
{
    CHECK(t == (cthread_t)&child && suspended && ActiveThreadCount == 1);
    CHECK(child.data == pending && !child.objc);
    ++detached;
}
void *cthread_join(cthread_t t)
{
    CHECK(t == (cthread_t)&child && !suspended && !detached);
    CHECK(!pending->start_mutex.lock && pending->start_func == NULL);
    CHECK(child_entry(pending) == NULL && !started);
    ++joined; return NULL;
}
static void *start_java(void *arg)
{
    CHECK(arg == pending->cookie && arg == pending->start_arg);
    CHECK(!pending->start_mutex.lock && child.data == pending);
    CHECK(ActiveThreadCount == 1 && detached && sysThreadSelf() == pending);
    ++started; return &child;
}
void cthread_exit(void *result)
{
    CHECK(result == NULL && sysThreadSelf() == NULL);
    CHECK(!ActiveThreadCount && !UserThreadCount && !locked && !join_locked);
    ++exited;
}
static void test_control(void)
{
    int i, calls, priority;
    native_thread_state_t after_suspend[8] = {
        SUSPENDED, SUSPENDED, SUSPENDED, SUSPENDED,
        SUSPENDED, MONITOR_SUSPENDED, SUSPENDED_AT_BREAKPOINT,
        SUSPENDED_AT_BREAKPOINT
    };
    native_thread_state_t after_resume[8] = {
        RUNNABLE, RUNNING, RUNNING, MONITOR_WAIT, CONDVAR_WAIT,
        MONITOR_WAIT, RUNNING_AT_BREAKPOINT, RUNNING
    };
    controlling = 1;
    current.incarnation = (cthread_t)&native[0];
    child.real_thread = 97;
    for (i = 0; i < 8; ++i) {
        memset(&control_thread, 0, sizeof(control_thread));
        control_thread.cthread = (cthread_t)&child;
        control_thread.state = (native_thread_state_t)i;
        control_state = after_suspend[i]; calls = suspend_calls;
        if (i == 2 || i == 5 || i == 7) {
            CHECK(sysThreadSuspend(&control_thread) == SYS_ERR);
            CHECK(suspend_calls == calls);
        } else {
            CHECK(sysThreadSuspend(&control_thread) == SYS_OK);
            CHECK(suspend_calls == calls+1);
        }
        CHECK(control_thread.state == control_state && !control_thread.mutex.lock);
        CHECK(control_thread.mpending_suspend == (i == 3));
        CHECK(control_thread.cpending_suspend == (i == 4));
        control_state = i == 3 ? MONITOR_WAIT : i == 4 ? CONDVAR_WAIT :
                        after_resume[after_suspend[i]];
        CHECK(sysThreadResume(&control_thread) == SYS_OK);
        CHECK(!control_thread.mpending_suspend && !control_thread.cpending_suspend);
        /* Also exercise resume directly from each state. */
        control_thread.state = (native_thread_state_t)i; control_state = after_resume[i];
        CHECK(sysThreadResume(&control_thread) == SYS_OK);
    }
    /* Mach failure leaves the original state hints updated, but unlocks and
     * returns SYS_ERR. Self-suspension must not hold the thread's mutex. */
    control_error = 5; control_thread.state = RUNNING; control_state = SUSPENDED;
    CHECK(sysThreadSuspend(&control_thread) == SYS_ERR);
    control_state = RUNNING;
    CHECK(sysThreadResume(&control_thread) == SYS_ERR);
    control_error = 0;
    native[0].data = &control_thread; control_state = SUSPENDED;
    calls = waited; force_wait = 1;
    CHECK(sysThreadSuspend(&control_thread) == SYS_OK && waited == calls);
    CHECK(force_wait == 1); force_wait = 0; native[0].data = NULL;
    control_thread.state = RUNNING; control_state = SUSPENDED; force_wait = 1;
    CHECK(sysThreadSuspend(&control_thread) == SYS_OK && waited == calls+1);
    CHECK(sizeof(thread_basic_info_data_t) == 44);
    CHECK(offsetof(thread_basic_info_data_t, cur_priority) == 24);
    priority = -99;
    CHECK(sysThreadGetPriority(&control_thread, &priority) == SYS_OK && priority == 17);
    CHECK(sysThreadSetPriority(&control_thread, 8) == SYS_OK);
    control_error = 5; priority = -99;
    CHECK(sysThreadGetPriority(&control_thread, &priority) == SYS_ERR && priority == -99);
    CHECK(sysThreadSetPriority(&control_thread, 8) == SYS_ERR && priority_calls == 4);
    CHECK(!control_thread.mutex.lock && resume_calls == 17);
}
static int visit(sys_thread_t *t, void *arg)
{
    CHECK(locked == expected_lock && arg == &visits);
    CHECK(t == expected[visits]);
    ++visits;
    return visits == stop_after ? 37 : 0;
}
static void test_attachment(void)
{
    sys_thread_t *t, *out;
    Classjava_lang_Thread object;
    Hjava_lang_Thread handle;
    ExecEnv ee;
    int i;
    native_thread_state_t breakpoint[8] = { RUNNING_AT_BREAKPOINT, RUNNING_AT_BREAKPOINT,
        SUSPENDED_AT_BREAKPOINT, MONITOR_WAIT, CONDVAR_WAIT, MONITOR_SUSPENDED,
        RUNNING_AT_BREAKPOINT, SUSPENDED_AT_BREAKPOINT };
    memset(&object, 0, sizeof(object)); memset(&handle, 0, sizeof(handle));
    memset(&ee, 0, sizeof(ee)); handle.obj = &object;
    current.incarnation = (cthread_t)&native[0];
    intrInit(); t = sysThreadSelf();
    CHECK(t && ActiveThreadCount == 0 && UserThreadCount == 0);
    intrInit(); CHECK(sysThreadSelf() == t);
#ifdef SIGPIPE
    raise(SIGPIPE); /* network write errors must not terminate the VM */
#endif
    sysThreadSetBackPtr(t, &handle); object.PrivateInfo = (long)t;
    CHECK(sysThreadBootstrap(&out, &handle) == SYS_OK && out == t);
    CHECK(object.PrivateInfo == (long)t && ActiveThreadCount == 1 && t->primordial_thread);
    sysThreadInitializeSystemThreads(); CHECK(finalizers == 1);
#ifdef SIGQUIT
    raise(SIGQUIT); CHECK(dumps == 2);
#endif
    sysThreadInterrupt(t);
    CHECK(sysThreadIsInterrupted(t, FALSE) == 1 && sysThreadIsInterrupted(t, TRUE) == 1);
    CHECK(sysThreadIsInterrupted(t, FALSE) == 0);
    object.eetop = (long)&ee;
    ee.thread = (JHandle *)&handle;
    CHECK(get_awt_cookie(EE2JNIEnv(&ee)) == NULL);
    set_awt_cookie(EE2JNIEnv(&ee), &ee);
    current.incarnation = (cthread_t)&native[1];
    CHECK(t->awt_cookie == &ee && get_awt_cookie(EE2JNIEnv(&ee)) == &ee);
    set_awt_cookie(EE2JNIEnv(&ee), NULL);
    current.incarnation = (cthread_t)&native[0];
    CHECK(!sysThreadHasPendingException(t));
    sysThreadPostException(t, &handle);
    CHECK(sysThreadHasPendingException(t) && ee.exception.exc == (JHandle *)&handle);
    object.eetop = 0;
    for (i = 0; i < 8; ++i) {
        t->state = (native_thread_state_t)i; sysThreadSetAtBreakpoint(t);
        CHECK(t->state == breakpoint[i] && !t->mutex.lock);
    }
    dying = t; saved_ee = NULL; deleted = 0;
    CHECK(sysThreadFree(t) == SYS_OK); dying = NULL;
    region_error = 1; out = &control_thread;
    CHECK(jni_AttachHostThread(&out) == SYS_ERR && out == &control_thread);
    CHECK(!sysThreadSelf() && !ActiveThreadCount);
    region_error = 0;
    CHECK(jni_AttachHostThread(&out) == SYS_OK && out == sysThreadSelf());
    CHECK(out->stack_base == mapped_stack_top && port_freed == 1 && ActiveThreadCount == 1);
    CHECK(jni_AttachHostThread(&t) == SYS_OK && t == out && port_freed == 1);
    dying = t; jni_DetachHostThread(); dying = NULL;
    CHECK(!sysThreadSelf() && !ActiveThreadCount);
    current.incarnation = NULL;
    CHECK(jni_AttachHostThread(&out) == SYS_NORESOURCE && !sysThreadSelf());
}

static void test_gc(int fatal_mode)
{
    sys_thread_t *t[3];
    void **regs, **other;
    int i, n, prior;
    CHECK(sizeof(i386_thread_state_t) == 64);
    CHECK(offsetof(i386_thread_state_t, esp) == 28);
    controlling = 0; fail_alloc = 0;
    for (i = 0; i < 3; ++i) {
        native[i].real_thread = 100+i;
        current.incarnation = (cthread_t)&native[i];
        CHECK(sysThreadAlloc(&t[i], (void *)0x700000, NULL) == SYS_OK);
        if (i == 0) {
            gc_mode = 1; locked = 1;
            CHECK(sysThreadSingle() == SYS_OK && locked == 1);
            sysThreadMulti();
            CHECK(!gc_allocation && !gc_resume_calls && locked == 1);
            locked = 0; gc_mode = 0;
        }
    }
    current.incarnation = (cthread_t)&native[0];
    t[1]->state = SUSPENDED; t[1]->cpending_suspend = TRUE;
    t[2]->state = MONITOR_WAIT;
    gc_mode = 1; gc_depth[1] = 1; locked = 1; /* Caller holds queue lock. */
    CHECK(sysThreadStackPointer(t[1]) == NULL);
    n = -1; CHECK(sysThreadRegisters(t[1], &n) == NULL && n == 0);
    gc_oom = 1;
    CHECK(sysThreadSingle() == SYS_NOMEM && locked == 1);
    sysThreadMulti(); CHECK(!gc_resume_calls && !gc_allocation);
    gc_oom = 0; gc_fail_suspend = 102;
    CHECK(sysThreadSingle() == SYS_ERR && !gc_resume_calls && locked == 1);
    gc_fail_suspend = 101;
    if (fatal_mode == 2) {
        gc_fail_resume = 102; gc_expected_resumes = 1;
        sysThreadSingle(); CHECK(0);
    }
    CHECK(sysThreadSingle() == SYS_ERR && gc_resume_calls == 1 && locked == 1);
    prior = gc_resume_calls; sysThreadMulti(); CHECK(gc_resume_calls == prior);
    gc_fail_suspend = 0; gc_fail_state = 101;
    CHECK(sysThreadSingle() == SYS_ERR && gc_resume_calls == prior+2);
    gc_fail_state = 0; gc_short_state = 1; prior = gc_resume_calls;
    CHECK(sysThreadSingle() == SYS_ERR && gc_resume_calls == prior+1);
    gc_short_state = 0;
    CHECK(sysThreadSingle() == SYS_OK && locked == 1);
    CHECK(gc_depth[1] == 2 && gc_depth[2] == 1);
    CHECK(sysThreadStackPointer(t[0]) == (void *)1);
    CHECK(sysThreadStackPointer(t[1]) == (void *)101007);
    CHECK(sysThreadStackPointer(t[2]) == (void *)102007);
    regs = sysThreadRegisters(t[1], &n); CHECK(n == 16 && regs != NULL);
    other = sysThreadRegisters(t[2], &n); CHECK(n == 16 && other != regs);
    for (i = 0; i < 16; ++i) {
        CHECK(regs[i] == (void *)(101000+i));
        CHECK(other[i] == (void *)(102000+i));
    }
    CHECK(sysThreadRegisters(t[0], &n) == NULL && n == 0);
    gc_resume_calls = 0;
    if (fatal_mode == 1) { gc_fail_resume = 102; gc_expected_resumes = 2; }
    sysThreadMulti(); CHECK(!fatal_mode);
    CHECK(gc_resume_calls == 2 && !gc_allocation && locked == 1);
    CHECK(gc_depth[1] == 1 && gc_depth[2] == 0 && gc_free_calls == 5);
    CHECK(t[1]->state == SUSPENDED && t[1]->cpending_suspend);
    CHECK(t[2]->state == MONITOR_WAIT);
    CHECK(sysThreadStackPointer(t[1]) == NULL);
    CHECK(sysThreadRegisters(t[2], &n) == NULL && n == 0);
    sysThreadMulti(); CHECK(gc_resume_calls == 2);
    locked = 0; gc_mode = 0;
    for (i = 0; i < 3; ++i) {
        current.incarnation = (cthread_t)&native[i];
        dying = t[i]; saved_ee = NULL;
        CHECK(sysThreadFree(t[i]) == SYS_OK);
        dying = NULL;
    }
}

int main(int argc, char **argv)
{
    sys_thread_t *t[3], *out;
    struct sys_thread initial;
    int i, stacks[3], order[3];
    Classjava_lang_Thread objects[3];
    Hjava_lang_Thread cookie[3];
    memset(objects, 0, sizeof(objects));
    memset(cookie, 0, sizeof(cookie));
    for (i = 0; i < 3; ++i) cookie[i].obj = &objects[i];
    CHECK(sizeof(void *) == 4 && offsetof(struct native_thread, data) == 116);
    CHECK(offsetof(struct native_thread, objc) == 124);
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
    /* Creation is not published until the child is suspended. Failed startup
     * must neither call Java nor leave an OS thread referring to freed storage. */
    creating = 1; fail_alloc = 1; out = &initial;
    CHECK(sysThreadCreate(8192, 1, start_java, &out, NULL) == SYS_NOMEM);
    CHECK(out == &initial && !forked && freed == 5);
    fail_alloc = 0; fail_fork = 1;
    CHECK(sysThreadCreate(8192, 1, start_java, &out, NULL) == SYS_NORESOURCE);
    CHECK(out == &initial && forked == 1 && freed == 6 && !joined && !started);
    fail_fork = 0; fail_suspend = 1;
    CHECK(sysThreadCreate(8192, 1, start_java, &out, NULL) == SYS_ERR);
    CHECK(out == &initial && forked == 2 && freed == 7 && joined == 1 && !started);
    CHECK(yielded == 1 && !ActiveThreadCount && !UserThreadCount);
    fail_suspend = 0;
    for (i = 0; i < 2; ++i) {
        memset(&child, 0, sizeof(child)); child.objc = 123;
        if (i) child.real_thread = 73;
        joined = detached = started = suspended = 0;
        force_wait = 1;
        CHECK(sysThreadCreate(8192, i, start_java, &out, i ? &cookie[i] : NULL) == SYS_OK);
        CHECK(out == pending && out->state == SUSPENDED && !out->start_mutex.lock);
        CHECK(out->system_thread == (i == 0) && UserThreadCount == i);
        CHECK(!started && detached == 1 && !joined && waited == i+1);
        current.incarnation = (cthread_t)&child;
        suspended = 0; /* Test-only OS resume; sysThreadResume has its own later probe. */
        CHECK(child_entry(out) == &child && started == 1);
        creating = 0; dying = out; saved_ee = NULL; deleted = 0;
        CHECK(sysThreadFree(out) == SYS_OK);
        dying = NULL; creating = 1;
        CHECK(!child.data && !ActiveThreadCount && !UserThreadCount);
    }
    creating = 0; fail_alloc = 1; out = &initial;
    current.incarnation = (cthread_t)&native[0]; mainstktop = &stacks[0];
    CHECK(sysThreadBootstrap(&out, &cookie[0]) == SYS_NOMEM);
    CHECK(out == &initial && !sysThreadSelf() && !ActiveThreadCount && !UserThreadCount);
    fail_alloc = 0;
    CHECK(sysThreadBootstrap(&out, &cookie[0]) == SYS_OK);
    CHECK(out == sysThreadSelf() && out->primordial_thread && !out->system_thread);
    CHECK(out->state == RUNNABLE && out->stack_base == mainstktop);
    CHECK(out->cookie == &cookie[0] && out->run_static_initializers);
    dying = out; saved_ee = NULL; deleted = 0;
    sysThreadExit();
    dying = NULL;
    CHECK(exited == 1 && !sysThreadSelf() && !ActiveThreadCount && !UserThreadCount);
    test_control();
    test_gc(argc == 1 ? 0 : strcmp(argv[1], "--gc-resume-error") == 0 ? 1 : 2);
    test_attachment();
    puts("Mach thread lifecycle/control contracts passed");
    return 0;
}

/* Inject allocation failure after CRT declarations, without changing their
 * linkage on Windows or adding an allocator hook to production code. */
#define malloc mach_test_malloc
#define free mach_test_free
#define calloc mach_test_calloc
#define abort mach_test_abort
#include "../../src/unix/java/runtime/thread_cthreads.c"
