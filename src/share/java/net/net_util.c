/* Reconstructed common socket support; OS mappings are portability adaptations. */
#include "net.h"
#if !defined(WIN32) && !defined(JDK115_CTHREADS)
#include <poll.h>
#endif
#ifdef WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    WSADATA data;
    if (reason == DLL_PROCESS_ATTACH) {
        if (WSAStartup(0x101, &data))
            SignalError(0, "java/net/SocketException", "Unsupported version of Windows Sockets API");
    } else if (reason == DLL_PROCESS_DETACH) WSACleanup();
    return TRUE;
}
#endif
void netSignalError(int error, char *operation)
{
    char *type = "java/net/SocketException", *text = operation;
    switch (error) {
    case NET_ERR(EADDRINUSE): type = "java/net/BindException"; text = "Address in use"; break;
    case NET_ERR(EADDRNOTAVAIL): type = "java/net/BindException"; text = "Cannot assign requested address"; break;
    case NET_ERR(ECONNREFUSED): type = "java/net/ConnectException"; text = "Connection refused"; break;
    case NET_ERR(ETIMEDOUT): type = "java/net/NoRouteToHostException"; text = "Operation timed out"; break;
    case NET_ERR(EHOSTUNREACH): type = "java/net/NoRouteToHostException"; text = "Host unreachable"; break;
    case NET_ERR(ECONNRESET): text = "Connection reset by peer"; break;
    case NET_ERR(ENOTSOCK): text = "Descriptor not a socket"; break;
    case NET_ERR(EPROTOTYPE): text = "Invalid protocol type"; break;
    case NET_ERR(ENOPROTOOPT):
    case NET_ERR(EPROTONOSUPPORT):
    case NET_ERR(ESOCKTNOSUPPORT):
    case NET_ERR(EOPNOTSUPP):
    case NET_ERR(EPFNOSUPPORT):
    case NET_ERR(EAFNOSUPPORT): text = "Option unsupported by protocol"; break;
    case NET_ERR(ESHUTDOWN): text = "Connection shutdown"; break;
    case NET_ERR(EHOSTDOWN): text = "Host down"; break;
    case NET_ERR(EISCONN): text = "Already connected"; break;
    case NET_ERR(ENOTCONN): text = "Not connected"; break;
    }
    SignalError(0, type, text ? text : "Socket error occurred");
}
int netSocket(struct Hjava_io_FileDescriptor *fd)
{
    if (fd == 0 || unhand(fd)->fd <= 0) {
        SignalError(0, "java/net/SocketException", "Socket closed");
        return -1;
    }
    return (int)unhand(fd)->fd - 1;
}
int netWait(struct Hjava_io_FileDescriptor *fd, long timeout, char *message)
{
    int socket, result;
#if defined(WIN32) || defined(JDK115_CTHREADS)
    fd_set reads;
    struct timeval tv;
#else
    struct pollfd reads;
#endif
    socket = netSocket(fd);
    if (socket < 0) return 0;
    if (!timeout) return 1;
#if defined(WIN32) || defined(JDK115_CTHREADS)
#ifdef JDK115_CTHREADS
    /* DR2's kernel rejects select nfds above FD_SETSIZE, even if a caller
     * raises RLIMIT_NOFILE. Reject before FD_SET can overrun its mask. */
    if (socket >= FD_SETSIZE) {
        netSignalError(EINVAL, "Descriptor exceeds DR2 select capacity");
        return 0;
    }
#endif
    FD_ZERO(&reads); FD_SET(socket, &reads);
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    result = select(socket + 1, &reads, 0, 0, &tv);
#else
    /* POSIX select cannot represent descriptors at or above FD_SETSIZE. */
    reads.fd = socket;
    reads.events = POLLIN;
    reads.revents = 0;
    result = poll(&reads, 1, timeout);
#endif
    if (!result) SignalError(0, "java/io/InterruptedIOException", message);
    else if (result < 0) netSignalError(net_error(), "Socket operation failed");
    return result > 0;
}
void netClose(struct Hjava_io_FileDescriptor *fd)
{
    int socket;
    if (!fd) { SignalError(0, "java/net/SocketException", "Socket closed"); return; }
    socket = (int)unhand(fd)->fd - 1;
    if (socket >= 0) {
        unhand(fd)->fd = -1;
        /* Portability: wake POSIX blocked reads/accepts before closing the descriptor. */
        shutdown(socket, 2);
        net_close(socket);
    }
}
void netAddress(struct sockaddr_in *sa, struct Hjava_net_InetAddress *addr, long port)
{
    memset(sa, 0, sizeof(*sa));
    sa->sin_family = (short)unhand(addr)->family;
    sa->sin_addr.s_addr = htonl(unhand(addr)->address);
    sa->sin_port = htons((unsigned short)port);
}
