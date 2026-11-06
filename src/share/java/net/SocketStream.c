/* Reconstructed from net.dll 0x10001db0 and 0x10001fb0. */
#include "net.h"
static int valid_buffer(HArrayOfByte *bytes, long off, long len)
{
    if (!bytes) { SignalError(0, "java/lang/NullPointerException", 0); return 0; }
    if (off < 0 || len < 0 || off > (long)obj_length(bytes) - len) {
        SignalError(0, "java/lang/ArrayIndexOutOfBoundsException", 0); return 0;
    }
    return 1;
}
long java_net_SocketInputStream_socketRead(struct Hjava_net_SocketInputStream *self, HArrayOfByte *bytes, long off, long len)
{
    int s = netSocket(unhand(self)->fd), result;
    struct Hjava_net_PlainSocketImpl *impl;
    if (s < 0 || !valid_buffer(bytes, off, len)) return -1;
    impl = (struct Hjava_net_PlainSocketImpl *)unhand(self)->impl;
    if (!impl) { SignalError(0, "java/lang/NullPointerException", "Null socketImpl"); return -1; }
    if (!netWait(unhand(self)->fd, unhand(impl)->timeout, "Read timed out")) return -1;
    result = recv(s, (char *)unhand(bytes)->body + off, len, 0);
    if (unhand(unhand(self)->fd)->fd < 0) { SignalError(0, "java/net/SocketException", "Socket was closed"); return -1; }
    if (result < 0) netSignalError(net_error(), "Socket read failed");
    return result;
}
void java_net_SocketOutputStream_socketWrite(struct Hjava_net_SocketOutputStream *self, HArrayOfByte *bytes, long off, long len)
{
    int s = netSocket(unhand(self)->fd), result;
    if (s < 0 || !valid_buffer(bytes, off, len)) return;
    while (len > 0) {
#ifdef MSG_NOSIGNAL
        result = send(s, (char *)unhand(bytes)->body + off, len, MSG_NOSIGNAL);
#else
        result = send(s, (char *)unhand(bytes)->body + off, len, 0);
#endif
        if (result <= 0) { netSignalError(net_error(), "Socket write failed"); return; }
        off += result; len -= result;
    }
}
