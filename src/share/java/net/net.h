/* Reconstructed from Sun JDK 1.1.5 net.dll; see net115-evidence.md. */
#ifndef NET115_H
#define NET115_H
#ifdef WIN32
#include <winsock.h>
typedef int net_socklen;
#define net_error() WSAGetLastError()
#define NET_ERR(x) WSA##x
#define net_close closesocket
#define net_ioctl ioctlsocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
typedef socklen_t net_socklen;
#define net_error() errno
#define NET_ERR(x) x
#define net_close close
#define net_ioctl ioctl
#endif
#include <string.h>
#include "native.h"
#include "java_io_FileDescriptor.h"
#include "java_net_InetAddress.h"
#include "java_net_InetAddressImpl.h"
#include "java_net_PlainSocketImpl.h"
#include "java_net_PlainDatagramSocketImpl.h"
#include "java_net_DatagramPacket.h"
#include "java_net_SocketInputStream.h"
#include "java_net_SocketOutputStream.h"
void netSignalError(int error, char *operation);
int netSocket(struct Hjava_io_FileDescriptor *fd);
int netWait(struct Hjava_io_FileDescriptor *fd, long timeout, char *message);
void netClose(struct Hjava_io_FileDescriptor *fd);
void netAddress(struct sockaddr_in *sa, struct Hjava_net_InetAddress *addr, long port);
#endif
