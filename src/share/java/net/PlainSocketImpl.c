/* Reconstructed from net.dll 0x10001220-0x10001be0. */
#include "net.h"
#include "java_net_SocketImpl.h"
static int tcp_level = IPPROTO_TCP;
void java_net_PlainSocketImpl_initProto(struct Hjava_net_PlainSocketImpl *self)
{
    struct protoent *entry = getprotobyname("tcp");
    tcp_level = entry ? entry->p_proto : IPPROTO_TCP;
}
void java_net_PlainSocketImpl_socketCreate(struct Hjava_net_PlainSocketImpl *self, long stream)
{
    int s;
    if (!unhand(self)->fd) { SignalError(0, "java/net/SocketException", "Socket closed"); return; }
    s = (int)socket(AF_INET, stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (s < 0) netSignalError(net_error(), "create");
    else unhand(unhand(self)->fd)->fd = s + 1;
}
void java_net_PlainSocketImpl_socketConnect(struct Hjava_net_PlainSocketImpl *self, struct Hjava_net_InetAddress *addr, long port)
{
    int s = netSocket(unhand(self)->fd);
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    if (s < 0) return;
    if (!addr) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    netAddress(&sa, addr, port);
    if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) { netSignalError(net_error(), "connect"); return; }
    unhand(self)->address = addr; unhand(self)->port = port;
    if (!unhand(self)->localport) {
        if (getsockname(s, (struct sockaddr *)&sa, &len) < 0) netSignalError(net_error(), "getsockname");
        else unhand(self)->localport = ntohs(sa.sin_port);
    }
}
void java_net_PlainSocketImpl_socketBind(struct Hjava_net_PlainSocketImpl *self, struct Hjava_net_InetAddress *addr, long port)
{
    int s = netSocket(unhand(self)->fd);
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    if (s < 0) return;
    if (!addr) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    netAddress(&sa, addr, port);
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) { netSignalError(net_error(), "bind"); return; }
    unhand(self)->address = addr;
    if (getsockname(s, (struct sockaddr *)&sa, &len) < 0) netSignalError(net_error(), "getsockname");
    else unhand(self)->localport = ntohs(sa.sin_port);
}
void java_net_PlainSocketImpl_socketListen(struct Hjava_net_PlainSocketImpl *self, long count)
{
    int s = netSocket(unhand(self)->fd);
    if (s >= 0 && listen(s, count) < 0) netSignalError(net_error(), "Listen failed");
}
void java_net_PlainSocketImpl_socketAccept(struct Hjava_net_PlainSocketImpl *self, struct Hjava_net_SocketImpl *peer)
{
    int s, accepted;
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    s = netSocket(unhand(self)->fd);
    if (s < 0) return;
    if (!peer || !unhand(peer)->address) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    if (!netWait(unhand(self)->fd, unhand(self)->timeout, "Accept timed out")) return;
    accepted = (int)accept(s, (struct sockaddr *)&sa, &len);
    if (accepted < 0) { netSignalError(net_error(), "Accept failed"); return; }
    unhand(unhand(peer)->fd)->fd = accepted + 1;
    unhand(peer)->port = ntohs(sa.sin_port);
    unhand(peer)->localport = unhand(self)->localport;
    unhand(unhand(peer)->address)->family = sa.sin_family;
    unhand(unhand(peer)->address)->address = ntohl(sa.sin_addr.s_addr);
}
long java_net_PlainSocketImpl_socketAvailable(struct Hjava_net_PlainSocketImpl *self)
{
    unsigned long bytes = 0;
    int s = netSocket(unhand(self)->fd);
    if (s < 0) return -1;
    if (net_ioctl(s, FIONREAD, &bytes) < 0) netSignalError(net_error(), "Socket ioctl failed");
    return bytes;
}
void java_net_PlainSocketImpl_socketClose(struct Hjava_net_PlainSocketImpl *self) { netClose(unhand(self)->fd); }
void java_net_PlainSocketImpl_socketSetOption(struct Hjava_net_PlainSocketImpl *self, long option, long on, struct Hjava_lang_Object *value)
{
    int s = netSocket(unhand(self)->fd), n = on, result;
    struct linger linger;
    if (s < 0) return;
    if (option == 1) result = setsockopt(s, tcp_level, TCP_NODELAY, (char *)&n, sizeof(n));
    else if (option == 128) {
        if (on && !value) { SignalError(0, "java/net/SocketException", "Invalid parameter"); return; }
        linger.l_onoff = (unsigned short)on;
        linger.l_linger = on ? (unsigned short)*(long *)unhand(value) : 0;
        result = setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
    } else { SignalError(0, "java/net/SocketException", "Socket option unsupported"); return; }
    if (result < 0) netSignalError(net_error(), "setsockopt failed");
}
long java_net_PlainSocketImpl_socketGetOption(struct Hjava_net_PlainSocketImpl *self, long option)
{
    int s = netSocket(unhand(self)->fd), n = 0, result;
    struct linger linger;
    struct sockaddr_in sa;
    net_socklen len;
    if (s < 0) return -1;
    if (option == 1) {
        len = sizeof(n); result = getsockopt(s, tcp_level, TCP_NODELAY, (char *)&n, &len);
        n = n ? 1 : -1;
    } else if (option == 128) {
        len = sizeof(linger); result = getsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&linger, &len);
        n = linger.l_onoff ? linger.l_linger : -1;
    } else if (option == 15) {
        len = sizeof(sa); result = getsockname(s, (struct sockaddr *)&sa, &len);
        n = ntohl(sa.sin_addr.s_addr);
        if (!n) n = 0x7f000001;
    } else { SignalError(0, "java/net/SocketException", "Invalid option"); return -1; }
    if (result < 0) { netSignalError(net_error(), "getsockopt failed"); return -1; }
    return n;
}
