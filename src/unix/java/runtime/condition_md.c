/* Reconstructed from Rhapsody DR2 libjava.A.dylib, 5C1366EC-5C136A94.
 * Mach IPC primitives beneath the VM condvar wrappers. See Phase 3 evidence.
 * Field/local names below are descriptive reconstructions. No Libc source
 * implementation is included; notably Sun's return codes differ from Libc.
 */
#include <stdio.h>
#include <mach/cthreads.h>
#include <mach/message.h>

extern int jio_fprintf(FILE *, const char *, ...);

/* Only the accessed prefix of the private cproc ABI is needed here.
 * Confirmed against cthread_internals.h and the original instruction offsets.
 */
struct condition_cproc {
    struct condition_cproc *next;
    void *incarnation;
    volatile int state;
    port_t reply_port;
    port_t wait_port;
};

static void enqueue(condition_t c, struct condition_cproc *p)
{
    p->next = NULL;
    if (c->queue.tail) c->queue.tail->next = (struct cthread_queue_item *)p;
    else c->queue.head = (struct cthread_queue_item *)p;
    c->queue.tail = (struct cthread_queue_item *)p;
}

int condition_wait_timeout(condition_t c, mutex_t m, unsigned int millisecs)
{
    struct condition_cproc *p = (struct condition_cproc *)ur_cthread_self();
    struct condition_cproc *item, *next;
    msg_header_t msg;
    int received, result;

    spin_lock(&c->lock);
    enqueue(c, p);
    p->state = 2;
    c->lock = 0;
    m->lock = 0;
    for (;;) {
        msg.msg_size = sizeof(msg);
        msg.msg_local_port = p->wait_port;
        received = millisecs == (unsigned int)-1
            ? msg_receive(&msg, 0, 0)
            : msg_receive(&msg, RCV_TIMEOUT, millisecs);
        /* Original special retry value; not Mach's RCV_INTERRUPTED (-207). */
        if (received == -21) continue;
        if (received != RCV_TIMED_OUT) {
            if (received != 0) {
                jio_fprintf(stderr, "condition_wait_timeout(): msg_receive failure with exit code %d", received);
                result = -1;
            } else {
                p->state = 0;
                result = 0;
            }
            break;
        }
        /* A notifier already dequeued us; consume its message before return. */
        if (p->state == 3) continue;
        spin_lock(&c->lock);
        if (p->state == 3) {
            c->lock = 0;
            continue;
        }
        item = (struct condition_cproc *)c->queue.head;
        c->queue.head = c->queue.tail = NULL;
        while (item) {
            next = item->next;
            if (item != p) enqueue(c, item);
            item = next;
        }
        result = -3;
        p->state = 0;
        c->lock = 0;
        break;
    }
    if (!mutex_try_lock(m)) mutex_wait_lock(m);
    return result;
}

static void send_wakeup(struct condition_cproc *p)
{
    msg_header_t msg;
    int result;
    /* Initialize reserved bits too; the original only sets msg_simple. */
    msg.msg_unused = 0;
    msg.msg_simple = 1;
    msg.msg_size = sizeof(msg);
    msg.msg_type = MSG_TYPE_NORMAL;
    msg.msg_local_port = PORT_NULL;
    msg.msg_remote_port = p->wait_port;
    msg.msg_id = 0;
    result = msg_send(&msg, SEND_TIMEOUT, 0);
    if (result != 0 && result != SEND_TIMED_OUT)
        jio_fprintf(stderr, "condition wakeup: msg_send() failure %d\n", result);
}

static struct condition_cproc *dequeue(condition_t c)
{
    struct condition_cproc *p = (struct condition_cproc *)c->queue.head;
    if (p) {
        c->queue.head = (struct cthread_queue_item *)p->next;
        if (!c->queue.head) c->queue.tail = NULL;
        p->state = 3;
    }
    return p;
}

void condition_signal_timeout(condition_t c)
{
    struct condition_cproc *p;
    if (!c->queue.head) return;
    spin_lock(&c->lock);
    p = dequeue(c);
    c->lock = 0;
    if (p) send_wakeup(p);
}

/* Preserve the original recursion: mark every waiter before unlocking, then
 * send in reverse order. No waiter node may be reused while still traversed.
 */
static void dequeue_unlock_send(condition_t c)
{
    struct condition_cproc *p = dequeue(c);
    if (p) {
        dequeue_unlock_send(c);
        send_wakeup(p);
    } else c->lock = 0;
}

void condition_broadcast_timeout(condition_t c)
{
    if (!c->queue.head) return;
    spin_lock(&c->lock);
    dequeue_unlock_send(c);
}
