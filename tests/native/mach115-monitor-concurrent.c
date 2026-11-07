/* Linux-only concurrent adapter test. Real reconstructed Mach monitor/thread
 * logic runs over pthread wait doubles; this is not execution on Mach. */
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"
#include "java_lang_Thread.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); abort(); } } while (0)
struct host_thread {
    sys_thread_t vm;
    unsigned char native[128];
    struct ur_cthread proc;
    Classjava_lang_Thread object;
    Hjava_lang_Thread handle;
    ExecEnv ee;
};
static __thread struct host_thread *host;
static struct { condition_t key; pthread_mutex_t lock; pthread_cond_t cv; } waits[8];
static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;
static sys_mon_t mon;
static int value, ready, go, mode, result;
static struct host_thread main_thread, workers[4];
static int find_condition(condition_t c)
{
    int i;
    pthread_mutex_lock(&table_lock);
    for (i = 0; i < 8; ++i) {
        if (waits[i].key == c) break;
        if (!waits[i].key) {
            waits[i].key = c; pthread_mutex_init(&waits[i].lock, NULL);
            pthread_cond_init(&waits[i].cv, NULL); break;
        }
    }
    pthread_mutex_unlock(&table_lock); CHECK(i < 8); return i;
}
static void attach(struct host_thread *h)
{
    sys_thread_t *t = &h->vm;
    memset(h, 0, sizeof(*h)); host = h;
    h->proc.incarnation = (cthread_t)h->native;
    memcpy(h->native+116, &t, sizeof(t)); t->cthread = h->proc.incarnation;
    h->handle.obj = &h->object; h->object.eetop = (long)&h->ee;
    t->cookie = &h->handle;
}
ur_cthread_t ur_cthread_self(void) { return &host->proc; }
int mutex_try_lock(mutex_t m) { return !__sync_lock_test_and_set(&m->lock, 1); }
void mutex_wait_lock(mutex_t m) { while (!mutex_try_lock(m)) sched_yield(); }
void spin_lock(int *p) { while (__sync_lock_test_and_set(p, 1)) sched_yield(); }
int64_t sysTimeMillis(void)
{
    struct timeval tv; gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000;
}
int condition_wait_timeout(condition_t c, mutex_t m, unsigned int ms)
{
    int i = find_condition(c), err;
    struct timespec end;
    struct timeval tv;
    pthread_mutex_lock(&waits[i].lock);
    __sync_lock_release(&m->lock);
    if (ms == (unsigned int)-1) err = pthread_cond_wait(&waits[i].cv, &waits[i].lock);
    else {
        gettimeofday(&tv, NULL);
        end.tv_sec = tv.tv_sec + ms/1000;
        end.tv_nsec = tv.tv_usec*1000L + (ms%1000)*1000000L;
        if (end.tv_nsec >= 1000000000L) { ++end.tv_sec; end.tv_nsec -= 1000000000L; }
        err = pthread_cond_timedwait(&waits[i].cv, &waits[i].lock, &end);
    }
    pthread_mutex_unlock(&waits[i].lock);
    mutex_wait_lock(m);
    return err == ETIMEDOUT ? SYS_TIMEOUT : err ? SYS_ERR : SYS_OK;
}
void condition_signal_timeout(condition_t c)
{
    int i = find_condition(c); pthread_mutex_lock(&waits[i].lock);
    pthread_cond_signal(&waits[i].cv); pthread_mutex_unlock(&waits[i].lock);
}
void condition_broadcast_timeout(condition_t c)
{
    int i = find_condition(c); pthread_mutex_lock(&waits[i].lock);
    pthread_cond_broadcast(&waits[i].cv); pthread_mutex_unlock(&waits[i].lock);
}
void cond_broadcast(condition_t c) { condition_broadcast_timeout(c); }
void sysAbort(void) { abort(); }
static void *run(void *arg)
{
    int i;
    attach((struct host_thread *)arg);
    if (mode == 0) {
        for (i = 0; i < 2000; ++i) {
            CHECK(sysMonitorEnter(&mon) == 0 && sysMonitorEnter(&mon) == 0);
            ++value;
            CHECK(sysMonitorExit(&mon) == 0 && sysMonitorExit(&mon) == 0);
        }
    } else {
        CHECK(sysMonitorEnter(&mon) == 0 && sysMonitorEnter(&mon) == 0);
        ++ready; sysMonitorNotifyAll(&mon);
        while (!go) {
            result = sysMonitorWait(&mon, -1, TRUE);
            if (mode != 1) break;
            CHECK(result == 0);
        }
        CHECK(mon.monitor_owner == &host->vm && mon.entry_count == 2);
        if (mode == 2) CHECK(result == SYS_INTRPT && !host->vm.interrupted);
        if (mode == 3) CHECK(result == SYS_OK && host->ee.exceptionKind != EXCKIND_NONE);
        CHECK(sysMonitorExit(&mon) == 0 && sysMonitorExit(&mon) == 0);
    }
    return NULL;
}
int main(void)
{
    pthread_t ids[4];
    int i, n;
    attach(&main_thread); CHECK(sysMonitorInit(&mon) == 0);
    for (mode = 0; mode < 4; ++mode) {
        ready = go = 0; n = mode < 2 ? 4 : 1;
        for (i = 0; i < n; ++i) CHECK(!pthread_create(&ids[i], NULL, run, &workers[i]));
        if (mode) {
            CHECK(sysMonitorEnter(&mon) == 0);
            while (ready != n) CHECK(sysMonitorWait(&mon, 2000, TRUE) == 0);
            if (mode == 1) { go = 1; CHECK(sysMonitorNotifyAll(&mon) == 0); }
            if (mode == 2) sysThreadInterrupt(&workers[0].vm);
            if (mode == 3) sysThreadPostException(&workers[0].vm, &workers[0].handle);
            CHECK(sysMonitorExit(&mon) == 0);
        }
        for (i = 0; i < n; ++i) CHECK(!pthread_join(ids[i], NULL));
        CHECK(!sysMonitorInUse(&mon));
    }
    CHECK(value == 8000);
    CHECK(sysMonitorEnter(&mon) == 0);
    CHECK(sysMonitorWait(&mon, 25, TRUE) == SYS_TIMEOUT);
    CHECK(sysMonitorExit(&mon) == 0 && sysMonitorDestroy(&mon) == 0);
    puts("Mach monitor logic concurrent contracts passed (pthread OS doubles)");
    return 0;
}
