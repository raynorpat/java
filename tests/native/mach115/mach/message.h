/* Legacy Mach IPC declarations used by the deterministic test double. */
#ifndef TEST_MACH_MESSAGE_H
#define TEST_MACH_MESSAGE_H
typedef unsigned int port_t;
typedef struct {
    unsigned int msg_unused:24, msg_simple:8;
    unsigned int msg_size;
    int msg_type;
    port_t msg_local_port, msg_remote_port;
    int msg_id;
} msg_header_t;
#define RCV_TIMEOUT 256
#define RCV_TIMED_OUT (-203)
#define SEND_TIMEOUT 1
#define SEND_TIMED_OUT (-103)
#define MSG_TYPE_NORMAL 0
#define PORT_NULL 0
int msg_receive(msg_header_t *, int, unsigned int);
int msg_send(msg_header_t *, int, unsigned int);
#endif
