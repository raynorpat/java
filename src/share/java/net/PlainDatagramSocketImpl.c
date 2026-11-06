/* Reconstructed from net.dll 0x10002250-0x10002ef0. */
#include "net.h"
void java_net_PlainDatagramSocketImpl_datagramSocketCreate(struct Hjava_net_PlainDatagramSocketImpl *self)
{
    int s, on = 1;
    if (!unhand(self)->fd) { SignalError(0, "java/net/SocketException", "Socket closed"); return; }
    s = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { netSignalError(net_error(), "Socket creation failed"); return; }
    unhand(unhand(self)->fd)->fd = s + 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on));
}
void java_net_PlainDatagramSocketImpl_datagramSocketClose(struct Hjava_net_PlainDatagramSocketImpl *self) { netClose(unhand(self)->fd); }
void java_net_PlainDatagramSocketImpl_bind(struct Hjava_net_PlainDatagramSocketImpl *self, long port, struct Hjava_net_InetAddress *addr)
{
    int s = netSocket(unhand(self)->fd);
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    if (s < 0) return;
    if (!addr) { SignalError(0, "java/lang/NullPointerException", "Null address"); return; }
    netAddress(&sa, addr, port);
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) { netSignalError(net_error(), "Cannot bind"); return; }
    if (getsockname(s, (struct sockaddr *)&sa, &len) < 0) netSignalError(net_error(), "getsockname");
    else unhand(self)->localPort = ntohs(sa.sin_port);
}
void java_net_PlainDatagramSocketImpl_send(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_DatagramPacket *packet)
{
    int s = netSocket(unhand(self)->fd);
    struct sockaddr_in sa;
    Classjava_net_DatagramPacket *p;
    if (s < 0) return;
    if (!packet) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    p = unhand(packet);
    if (!p->buf || !p->address) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    netAddress(&sa, p->address, p->port);
    if (sendto(s, (char *)unhand(p->buf)->body, p->length, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        netSignalError(net_error(), "Send failed");
}
long java_net_PlainDatagramSocketImpl_peek(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_InetAddress *addr)
{
    struct Hjava_io_FileDescriptor *fd = unhand(self)->fd;
    int s = netSocket(fd), result;
    char byte;
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    if (s < 0) return -1;
    if (!addr) { SignalError(0, "java/lang/NullPointerException", 0); return -1; }
    if (!netWait(fd, unhand(self)->timeout, "Receive timed out")) return -1;
    result = recvfrom(s, &byte, 1, MSG_PEEK, (struct sockaddr *)&sa, &len);
    if (unhand(fd)->fd < 0) { SignalError(0, "java/net/SocketException", "Socket was closed"); return -1; }
    if (result < 0 && net_error() != NET_ERR(EMSGSIZE)) { netSignalError(net_error(), "Peek failed"); return -1; }
    unhand(addr)->address = ntohl(sa.sin_addr.s_addr); unhand(addr)->family = sa.sin_family;
    return ntohs(sa.sin_port);
}
void java_net_PlainDatagramSocketImpl_receive(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_DatagramPacket *packet)
{
    struct Hjava_io_FileDescriptor *fd = unhand(self)->fd;
    int s = netSocket(fd), result;
    struct sockaddr_in sa;
    net_socklen len = sizeof(sa);
    Classjava_net_DatagramPacket *p;
    if (s < 0) return;
    if (!packet || !unhand(packet)->buf) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    p = unhand(packet);
    p->address = (struct Hjava_net_InetAddress *)execute_java_constructor(EE(), "java/net/InetAddress", 0, "()");
    if (!p->address) return;
    if (!netWait(fd, unhand(self)->timeout, "Receive timed out")) return;
    result = recvfrom(s, (char *)unhand(p->buf)->body, p->length, 0, (struct sockaddr *)&sa, &len);
    if (unhand(fd)->fd < 0) { SignalError(0, "java/net/SocketException", "Socket was closed"); return; }
    if (result < 0 && net_error() != NET_ERR(EMSGSIZE)) { p->length = 0; netSignalError(net_error(), "Receive failed"); return; }
    if (result >= 0) p->length = result;
    p->port = ntohs(sa.sin_port);
    unhand(p->address)->family = sa.sin_family;
    unhand(p->address)->address = ntohl(sa.sin_addr.s_addr);
}
void java_net_PlainDatagramSocketImpl_socketSetOption(struct Hjava_net_PlainDatagramSocketImpl *self, long option, struct Hjava_lang_Object *value)
{
    int s = netSocket(unhand(self)->fd), result, n;
    unsigned long addr;
    if (s < 0) return;
    if (!value) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    if (option == 16) {
        addr = htonl(unhand((struct Hjava_net_InetAddress *)value)->address);
        result = setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, sizeof(addr));
    } else if (option == 4) {
        n = *(long *)unhand(value);
        result = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n));
    } else { SignalError(0, "java/net/SocketException", "Invalid datagram socket option"); return; }
    if (result < 0) netSignalError(net_error(), "setsockopt failed");
}
long java_net_PlainDatagramSocketImpl_socketGetOption(struct Hjava_net_PlainDatagramSocketImpl *self, long option)
{
    struct Hjava_io_FileDescriptor *fd = unhand(self)->fd;
    int s = netSocket(fd), result;
    unsigned long addr;
    struct sockaddr_in sa;
    net_socklen len;
    if (s < 0) return -1;
    if (option == 16) {
        len = sizeof(addr); result = getsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, &len);
    } else if (option == 15) {
        len = sizeof(sa); result = getsockname(s, (struct sockaddr *)&sa, &len); addr = sa.sin_addr.s_addr;
    } else { SignalError(0, "java/net/SocketException", "Invalid datagram socket option"); return -1; }
    if (result < 0) { netSignalError(net_error(), "getsockopt failed"); return -1; }
    return ntohl(addr);
}
static void membership(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_InetAddress *addr, int option)
{
    int s = netSocket(unhand(self)->fd);
    struct ip_mreq request;
    net_socklen len = sizeof(request.imr_interface);
    if (s < 0) return;
    if (!addr) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    request.imr_multiaddr.s_addr = htonl(unhand(addr)->address);
    if ((unhand(addr)->address & 0xf0000000UL) != 0xe0000000UL) { netSignalError(0, "Not a multicast address"); return; }
    request.imr_interface.s_addr = INADDR_ANY;
    getsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&request.imr_interface, &len);
    if (setsockopt(s, IPPROTO_IP, option, (char *)&request, sizeof(request)) < 0) netSignalError(net_error(), "Multicast membership failed");
}
void java_net_PlainDatagramSocketImpl_join(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_InetAddress *addr) { membership(self, addr, IP_ADD_MEMBERSHIP); }
void java_net_PlainDatagramSocketImpl_leave(struct Hjava_net_PlainDatagramSocketImpl *self, struct Hjava_net_InetAddress *addr) { membership(self, addr, IP_DROP_MEMBERSHIP); }
void java_net_PlainDatagramSocketImpl_setTTL(struct Hjava_net_PlainDatagramSocketImpl *self, char ttl)
{
    int s = netSocket(unhand(self)->fd);
    unsigned char value = (unsigned char)ttl;
    if (s >= 0 && setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&value, sizeof(value)) < 0) netSignalError(net_error(), "Set TTL failed");
}
char java_net_PlainDatagramSocketImpl_getTTL(struct Hjava_net_PlainDatagramSocketImpl *self)
{
    int s = netSocket(unhand(self)->fd);
    unsigned char value = 0;
    net_socklen len = sizeof(value);
    if (s >= 0 && getsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&value, &len) < 0) netSignalError(net_error(), "Get TTL failed");
    return (char)value;
}
