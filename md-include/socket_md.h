#ifndef sun_java_socket_md_h___
#define sun_java_socket_md_h___

#include <winsock.h>
#include <errno.h>

/*
** The namespace terrorists up north are busy..busy..busy...
*/
#ifdef h_errno
#undef h_errno
#endif

#define MAXHOSTNAMELEN          64

/* Socket calls report Winsock error codes. */
#undef EISCONN
#define EISCONN                 WSAEISCONN

#endif /* sun_java_socket_md_h___ */
