/*
 * Reentrant host lookups for socket_md.c: the resolver's static result is
 * copied into the caller's buffer while a lock is held.
 */
#include <pthread.h>
#include <string.h>
#include "socket_md.h"
#include "iomgr.h"

static pthread_mutex_t resolver_lock = PTHREAD_MUTEX_INITIALIZER;

/* Copy *hp into result/buf; NULL if buf is too small. */
static struct hostent *copy_hostent(struct hostent *hp, struct hostent *result,
				    char *buf, int buflen)
{
    char **list;
    char *p;
    int n, i, need;

    for (n = 0; hp->h_addr_list[n] != NULL; n++)
	;
    need = (n + 2) * sizeof(char *) + n * hp->h_length + strlen(hp->h_name) + 1;
    if (need > buflen)
	return NULL;

    list = (char **) buf;		/* addresses, NULL, empty alias list */
    p = buf + (n + 2) * sizeof(char *);
    for (i = 0; i < n; i++) {
	list[i] = p;
	memcpy(p, hp->h_addr_list[i], hp->h_length);
	p += hp->h_length;
    }
    list[n] = NULL;
    list[n + 1] = NULL;
    strcpy(p, hp->h_name);

    result->h_name = p;
    result->h_aliases = &list[n + 1];
    result->h_addrtype = hp->h_addrtype;
    result->h_length = hp->h_length;
    result->h_addr_list = list;
    return result;
}

struct hostent *sysGethostbyname(const char *name, struct hostent *result,
				 char *buf, int buflen, int *h_errnop)
{
    struct hostent *hp;

    pthread_mutex_lock(&resolver_lock);
    hp = gethostbyname(name);
    *h_errnop = hp ? 0 : h_errno;
    if (hp)
	hp = copy_hostent(hp, result, buf, buflen);
    pthread_mutex_unlock(&resolver_lock);
    return hp;
}

struct hostent *sysGethostbyaddr(const char *addr, int len, int type,
				 struct hostent *result, char *buf,
				 int buflen, int *h_errnop)
{
    struct hostent *hp;

    pthread_mutex_lock(&resolver_lock);
    hp = gethostbyaddr(addr, len, type);
    *h_errnop = hp ? 0 : h_errno;
    if (hp)
	hp = copy_hostent(hp, result, buf, buflen);
    pthread_mutex_unlock(&resolver_lock);
    return hp;
}
