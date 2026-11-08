#ifndef sun_java_iomgr_h___
#define sun_java_iomgr_h___

#include <sys/socket.h>
#include <netdb.h>

#define IO_DONTBLOCK 0
#define IO_BLOCK 1

/* Sockets block; each Java thread is a native thread. */
#define sysConnect(a,b,c)		connect(a,b,c)
#ifdef JDK115_CTHREADS
#define sysAccept(a,b,c)         accept(a,b,(int *)(c))
#else
#define sysAccept(a,b,c)		accept(a,b,(socklen_t *)(c))
#endif
#define sysRecv(a,b,c,d)		recv(a,b,c,d)
#define sysSend(a,b,c,d)		send(a,b,c,d)
#ifdef JDK115_CTHREADS
#define sysRecvFrom(a,b,c,d,e,f)  recvfrom(a,b,c,d,e,(int *)(f))
#else
#define sysRecvFrom(a,b,c,d,e,f)	recvfrom(a,b,c,d,e,(socklen_t *)(f))
#endif
#define sysSendTo(a,b,c,d,e,f)		sendto(a,b,c,d,e,f)

#define sysBind(a,b,c)			bind(a,b,c)
#define sysListen(a,b)			listen(a,b)
#define sysSocket(a,b,c)		socket(a,b,c)

extern long sysSocketAvailable(int, long *);

#define sysGethostname(a,b)		gethostname(a,b)
/* Host lookups are serialized in socket_md.c; see sysGethostbyname there. */
extern struct hostent *sysGethostbyname(const char *name, struct hostent *result,
					char *buf, int buflen, int *h_errnop);
extern struct hostent *sysGethostbyaddr(const char *addr, int len, int type,
					struct hostent *result, char *buf,
					int buflen, int *h_errnop);

#endif /* sun_java_iomgr_h___ */
