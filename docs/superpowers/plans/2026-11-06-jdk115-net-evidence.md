# JDK 1.1.5 net evidence

## Binary recovery

Primary: vendor/jdk1.1.5/bin/net.dll, checked against net_g.dll with IDA.
Both expose exactly the same 29 legacy Java_*_stub symbols (tools/net115-exports.txt).
The DLL entry point is not an exported API. Rhapsody DR2 libnet.A.dylib retains all
29 java_net_* implementation names and their generated stub wrappers.

| Windows address | Recovered implementation |
| --- | --- |
| 10001000 | DllMain, WSAStartup(1.1), WSACleanup |
| 10001080 | select-based millisecond read timeout |
| 100010f0 / 10001110 / 10001130 | receive / send / close socket descriptor helpers |
| 10001160 / 100011d0 / 10001210 | local hostname / wildcard address / AF_INET |
| 10001220 through 10001a50 | PlainSocketImpl create/connect/bind/listen/accept/available/close/options |
| 10001be0 | socket error to Java exception mapping |
| 10001db0 / 10001fb0 | SocketInputStream read / SocketOutputStream write |
| 100020a0 / 100021f0 | lookupAllHostAddr / getHostByAddr |
| 10002250 through 10002e60 | PlainDatagramSocketImpl create/close/bind/send/peek/receive/options/membership/TTL |
| 10002ef0 through 10003270 | 29 generated legacy stubs |

The generated headers and stubs were produced by the original 1.1.5 javah against
its unmodified classes.zip, not adapted from 1.0.2. PlainSocketImpl timeout is at
byte 16; PlainDatagramSocketImpl is localPort, fd, timeout. FileDescriptor.fd holds
socket+1. InetAddress.address uses host byte order. Constructor invocation uses
"()", confirmed at 10002800; "()V" is not the legacy constructor ABI.

## Adaptations

Source function names follow the Rhapsody symbols. File boundaries and local names
are reconstructed, not asserted original. net_util.c contains shared platform
branches: WinSock 1.1 on Windows and POSIX sockets on Linux. Linux links with
-Bsymbolic so obsolete VM java_net_* implementations cannot interpose the new
library. The parent removes the obsolete generated stub registrations from javai.

The close helper calls shutdown before close to wake blocked POSIX operations;
Sun instead optionally resolves NT CancelIo and closes the socket. Closed reads
and receives become SocketException rather than reporting EOF/empty packets.
POSIX writes use MSG_NOSIGNAL. Send loops avoid dropping partial writes.
Debug uses net.dll/libnet.so in its separate configuration directory because the
current VM library-name lookup does not append _g (the original export ABI is
identical). No original DLL is installed or linked at runtime by these targets.

## Tests and oracle

Net115.java and Net115Close.java were compiled by Sun 1.1.5 javac (45.3). Their
expected output is checked by tools/net115.cmake. Before implementation Net115
failed on our VM with UnsatisfiedLinkError: no net in shared library path.
Sun originals passed both tests. The reconstructed net.dll was then loaded by
Sun's original javai.dll in an isolated bin directory and passed both tests.

Coverage: numeric and localhost resolution; loopback TCP bind/connect/accept,
accept/read timeouts, nodelay, linger on/off, local/remote ports and bound address,
byte transfer and EOF, connection refusal; UDP timeout, oversized packet
truncation, sender address/port; multicast TTL/interface; and close waking a
blocked accept, stream read, and datagram receive. Tests do not require public
network access or multicast packet routing.

Net115RMI creates a real registry, exports an implementation, resolves it through
a loopback Registry stub, invokes echo and list, then exits to stop RMI threads.
The CMake test first runs our rmic to create fresh stub/skeleton class files in
the build directory. The same scenario passes Sun's VM. Sun disallows registry
mutation via a loopback alias when its local hostname maps to a different address;
cleanup therefore uses the local Registry object. The advertised object endpoint
is explicitly 127.0.0.1.

Windows Release and Debug: vm-net115, net-exports, tools-rmi-functional115 pass.
The export check verifies exactly all 29 Sun exports. The parent owns full-suite
and installed-image gates. Linux results are recorded below after execution.

Linux Release: compiled all net sources without net warnings in C89/pedantic mode;
vm-net115 passed 15 consecutive repetitions and tools-rmi-functional115 passed.
Initial Linux Net115Close revealed a UDP close race: GDB SIGSEGV stack began
java_net_PlainDatagramSocketImpl_receive -> receive_stub ->
invokeSynchronizedNativeMethod -> ExecuteJava -> ThreadRT0. Java close clears
PlainDatagramSocketImpl.fd after native close; the receive method had incorrectly
reloaded that field after waking. Receive/peek now retain the original descriptor
handle before blocking, matching Sun's cached descriptor. The repeated run above
is after this correction. Windows Release and Debug were rebuilt afterward.

Remaining integration gates are the parent's Linux Debug and complete suites,
installed-image tests, and fresh review. Multicast group delivery and DNS failures
requiring a resolver are deliberately not environment-dependent acceptance gates;
the native membership, peek, and reverse-lookup entry points exist but are not
individually driven by the present Java tests.

## Review follow-up: peek, large descriptors, error messages

The review found that MSG_PEEK can wake on POSIX shutdown with no sender address.
Net115PeekClose installs a permissive Java 1.1 SecurityManager to force the real
DatagramSocket.receive -> impl.peek path. Before the fix Linux called
checkConnect with a nonexistent sender; Sun's original Windows DLL passed.
The native peek now checks its retained descriptor after recvfrom and throws
SocketException before reading sockaddr, just as receive does.

Net115HighFD retains 1100 loopback datagram sockets and verifies that a timed
receive still produces InterruptedIOException. The original implementation
aborted on Linux with "bit out of range 0 - FD_SETSIZE on fd_set". POSIX netWait
now uses poll for millisecond timeouts, avoiding the fd_set representation limit;
Windows retains Sun's select implementation. The stress test runs only on Unix
and requires a descriptor limit above 1100.

IDA at 0x10001be0 recovered the missing error groups: 10041 "Invalid protocol
type"; 10042-10047 "Option unsupported by protocol"; 10058 "Connection shutdown";
10064 "Host down". All are SocketException. net-errors115 directly exercises the
native translator and checks every restored mapping. Its red result was 10041
mapping to generic "operation". There is no public-network dependency.

Review follow-up verification: Windows Release and Debug pass all four focused
tests (network, exports, RMI, error mappings). Linux Release passes the expanded
network suite five consecutive times, including both new regressions; functional
RMI passes; the native error-mapping test passes when compiled as 32-bit C89.
