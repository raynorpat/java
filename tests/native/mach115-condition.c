/* Deterministic protocol tests, not a Mach scheduler or DR2 execution test. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <mach/cthreads.h>
#include <mach/message.h>
#ifdef TEST_VM_CONDVARS
#include "sys_api.h"
#include "cthreads/threads_md.h"
#include "cthreads/condvar_md.h"
static sys_thread_t vm_thread;
sys_thread_t *sysThreadSelf(void) { return &vm_thread; }
void cond_broadcast(condition_t c) { condition_broadcast_timeout(c); }
#endif

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
struct proc { struct proc *next; void *incarnation; int state; port_t reply, wait; };
static struct proc self, first, last;
static struct condition cv;
static struct mutex mutex;
static int mode, receives, sends, errors, block_lock, blocked, race_lock;
static unsigned int timeout;
static int sent[4], send_result;
static void enqueue(struct proc *p)
{
    p->next = NULL;
    if (cv.queue.tail) cv.queue.tail->next = (struct cthread_queue_item *)p;
    else cv.queue.head = (struct cthread_queue_item *)p;
    cv.queue.tail = (struct cthread_queue_item *)p;
}
static void reset(int scenario)
{
    memset(&cv, 0, sizeof(cv)); memset(&mutex, 0, sizeof(mutex));
    memset(&self, 0, sizeof(self)); memset(&first, 0, sizeof(first));
    memset(&last, 0, sizeof(last));
    self.wait=11; first.wait=22; last.wait=33; mutex.lock=1;
    mode=scenario; receives=sends=errors=blocked=race_lock=0;
    block_lock=send_result=0; timeout=37;
}
ur_cthread_t ur_cthread_self(void) { return (ur_cthread_t)&self; }
void spin_lock(int *lock)
{
    if (race_lock) {
        race_lock=0;
        condition_signal_timeout(&cv);
    }
    CHECK(*lock==0); *lock=1;
}
int mutex_try_lock(mutex_t m)
{
    CHECK(m==&mutex && m->lock==0);
    if (block_lock) return 0;
    m->lock=1; return 1;
}
void mutex_wait_lock(mutex_t m) { CHECK(m==&mutex && !m->lock); m->lock=1; ++blocked; }
int jio_fprintf(FILE *f, const char *format, ...)
{
    CHECK(f==stderr && strstr(format,"failure")); ++errors; return 0;
}
int msg_send(msg_header_t *m, int options, unsigned int ms)
{
    CHECK(cv.lock==0 && options==SEND_TIMEOUT && ms==0);
    CHECK(m->msg_simple==1 && m->msg_size==24 && m->msg_type==MSG_TYPE_NORMAL);
    CHECK(m->msg_local_port==PORT_NULL && m->msg_id==0);
    CHECK(sends<4); sent[sends++]=(int)m->msg_remote_port;
    if (mode==7) {
        CHECK(!cv.queue.head && !cv.queue.tail);
        CHECK(first.state==3 && self.state==3 && last.state==3);
    }
    return send_result;
}
int msg_receive(msg_header_t *m, int options, unsigned int ms)
{
#ifdef TEST_VM_CONDVARS
    CHECK(vm_thread.state==CONDVAR_WAIT);
#endif
    CHECK(!mutex.lock && !cv.lock);
    CHECK(m->msg_size==24 && m->msg_local_port==self.wait);
    CHECK(options==(timeout==(unsigned int)-1 ? 0 : RCV_TIMEOUT));
    CHECK(ms==(timeout==(unsigned int)-1 ? 0 : timeout));
    ++receives; CHECK(receives<4);
    if (mode==1) { enqueue(&last); return RCV_TIMED_OUT; }
    if (mode==2 && receives==1) return -21;
    if (mode==3 && receives==1) { condition_signal_timeout(&cv); return RCV_TIMED_OUT; }
    if (mode==4 && receives==1) { race_lock=1; return RCV_TIMED_OUT; }
    if (mode==5) return -202;
    if (self.state!=3) condition_signal_timeout(&cv);
    CHECK(self.state==3); return 0;
}
static int wait_for_condition(condition_t c, mutex_t m, unsigned int ms)
{
#ifdef TEST_VM_CONDVARS
    int status;
    vm_thread.state=RUNNING;
    status=ms==(unsigned int)-1 ? condvarWait(c,m,CONDVAR_WAIT)
        : condvarTimedWait(c,m,ms,CONDVAR_WAIT);
    CHECK(vm_thread.state==RUNNING);
    return status;
#else
    return condition_wait_timeout(c,m,ms);
#endif
}
int main(void)
{
    CHECK(sizeof(void *)==4 && sizeof(msg_header_t)==24);
    CHECK(sizeof(struct condition)==16 && sizeof(struct mutex)==8);
    CHECK(offsetof(struct proc,state)==8 && offsetof(struct proc,wait)==16);
    reset(0); CHECK(wait_for_condition(&cv,&mutex,timeout)==0);
    CHECK(receives==1 && sends==1 && sent[0]==11 && mutex.lock && self.state==0);
    reset(1); enqueue(&first); block_lock=1;
    CHECK(wait_for_condition(&cv,&mutex,timeout)==-3);
    CHECK(cv.queue.head==(struct cthread_queue_item *)&first && first.next==&last);
    CHECK(cv.queue.tail==(struct cthread_queue_item *)&last && !last.next);
    CHECK(self.state==0 && mutex.lock && blocked==1 && !errors);
    reset(2); timeout=(unsigned int)-1;
    CHECK(wait_for_condition(&cv,&mutex,timeout)==0 && receives==2);
    reset(3); CHECK(wait_for_condition(&cv,&mutex,timeout)==0 && receives==2);
    CHECK(sends==1 && !cv.queue.head && mutex.lock);
    reset(4); CHECK(wait_for_condition(&cv,&mutex,timeout)==0 && receives==2);
    CHECK(sends==1 && !cv.queue.head && mutex.lock);
    reset(5); CHECK(wait_for_condition(&cv,&mutex,timeout)==-1);
    CHECK(errors==1 && mutex.lock && !cv.queue.head && !cv.queue.tail && self.state==0);
    reset(6); condition_signal_timeout(&cv); CHECK(!sends && !cv.lock);
    enqueue(&first); enqueue(&last); condition_signal_timeout(&cv);
    CHECK(sends==1 && sent[0]==22 && first.state==3 && last.state==0);
    CHECK(cv.queue.head==(struct cthread_queue_item *)&last && cv.queue.tail==cv.queue.head);
    reset(7); enqueue(&first); enqueue(&self); enqueue(&last);
    condition_broadcast_timeout(&cv);
    CHECK(sends==3 && sent[0]==33 && sent[1]==11 && sent[2]==22);
    reset(8); enqueue(&self); send_result=SEND_TIMED_OUT;
    condition_signal_timeout(&cv); CHECK(!errors && sends==1);
    reset(9); enqueue(&self); send_result=-100;
    condition_signal_timeout(&cv); CHECK(errors==1 && sends==1);
    puts("Mach condition protocol passed (mock IPC)"); return 0;
}
