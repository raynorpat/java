/* Test-only OS/VM doubles; includes the original Rhapsody record headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/monitor_md.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static sys_thread_t thread;
static sys_thread_t *current;
static struct condition cv;
static struct mutex mutex;
static int calls, broadcast, locked, result;
static native_thread_state_t expected_state;
static unsigned int expected_timeout;

sys_thread_t *sysThreadSelf(void) { return current; }
int condition_wait_timeout(condition_t c, mutex_t m, unsigned int timeout)
{
    CHECK(c==&cv && m==&mutex && timeout==expected_timeout);
    if (current) CHECK(current->state==expected_state);
    ++calls; return result;
}
void condition_signal_timeout(condition_t c) { CHECK(c==&cv); ++calls; }
void condition_broadcast_timeout(condition_t c) { CHECK(c==&cv); ++calls; }
void cond_broadcast(condition_t c)
{
    CHECK(c==&cv && !c->lock && !locked); ++broadcast;
    c->queue.head=c->queue.tail=NULL;
}
void spin_lock(int *lock) { CHECK(lock==&cv.lock && !*lock); ++locked; *lock=1; }

int main(void)
{
    char *name="preserved";
    int statuses[3], i;
    CHECK(sizeof(void *)==4 && sizeof(jmp_buf)==72);
    CHECK(sizeof(sys_thread_t)==192 && sizeof(sys_mon_t)==68);
    CHECK(offsetof(sys_thread_t,cthread)==12 && offsetof(sys_thread_t,state)==20);
    CHECK(offsetof(sys_thread_t,interrupted)==40 && offsetof(sys_thread_t,start_mutex)==56);
    CHECK(offsetof(sys_thread_t,jump_buf)==76 && offsetof(sys_thread_t,monitor_entry_count)==148);
    CHECK(offsetof(sys_thread_t,mon_wait)==152 && offsetof(sys_thread_t,cacheKey)==156);
    CHECK(offsetof(sys_thread_t,monitorCache)==160);
    CHECK(offsetof(sys_mon_t,cv_waiters)==12 && offsetof(sys_mon_t,cv_monitor)==28);
    CHECK(offsetof(sys_mon_t,waiters_queue)==44 && offsetof(sys_mon_t,mwait_queue)==52);
    CHECK(offsetof(sys_mon_t,monitor_owner)==60 && offsetof(sys_mon_t,oldPriority)==64);
    memset(&cv,0x55,sizeof(cv)); cv.name=name;
    CHECK(condvarInit(&cv)==0 && !cv.lock && !cv.queue.head && !cv.queue.tail && cv.name==name);
    CHECK(condvarDestroy(&cv)==0 && locked==1 && !broadcast && cv.lock==1);
    condvarInit(&cv); locked=0;
    cv.queue.head=(struct cthread_queue_item *)&thread;
    CHECK(condvarDestroy(&cv)==0 && broadcast==1 && locked==1 && cv.lock==1);
    condvarInit(&cv); current=&thread;
    statuses[0]=0; statuses[1]=-3; statuses[2]=-1;
    for (i=0;i<3;i++) {
        result=statuses[i]; expected_state=CONDVAR_WAIT; expected_timeout=123;
        thread.state=SUSPENDED;
        CHECK(condvarTimedWait(&cv,&mutex,123,CONDVAR_WAIT)==result);
        CHECK(thread.state==RUNNING);
        expected_state=MONITOR_WAIT; expected_timeout=(unsigned int)-1;
        thread.state=SUSPENDED;
        CHECK(condvarWait(&cv,&mutex,MONITOR_WAIT)==result && thread.state==RUNNING);
    }
    current=NULL; expected_timeout=0; result=-3;
    CHECK(condvarTimedWait(&cv,&mutex,0,CONDVAR_WAIT)==-3);
    expected_timeout=(unsigned int)-1;
    CHECK(condvarWait(&cv,&mutex,MONITOR_WAIT)==-3);
    CHECK(calls==8);
    CHECK(condvarSignal(&cv)==0 && calls==9);
    CHECK(condvarBroadcast(&cv)==0 && calls==10);
    puts("Mach condvar wrappers and original layouts passed"); return 0;
}
