#include <windows.h>
#include <time.h>
#include "sys_api.h"

char *vbase = (char*) 0x40000000;

/* Milliseconds since 1970-01-01 UTC. */
static int64_t now_millis(void)
{
    FILETIME ft;
    int64_t t;

    GetSystemTimeAsFileTime(&ft);	/* 100ns units since 1601 */
    t = ((int64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return t / 10000 - 11644473600000;
}

long sysGetMilliTicks(void)
{
    return (long) now_millis();
}

long sysTime(long *p)
{
    long t = (long) time(NULL);
    if (p)
	*p = t;
    return t;
}

int64_t sysTimeMillis(void)
{
    return now_millis();
}

struct tm * sysGmtime(time_t * timer, struct tm *result)
{
    return gmtime_s(result, timer) == 0 ? result : NULL;
}

struct tm * sysLocaltime( time_t *timer, struct tm * result )
{
    return localtime_s(result, timer) == 0 ? result : NULL;
}

void sysStrftime(char *buf, int buflen, char *fmt, struct tm *t)
{
    if (strftime(buf, buflen, fmt, t) == 0 && buflen > 0)
	buf[0] = 0;
}

time_t sysMktime(struct tm *t)
{
    return mktime(t);
}


#if defined(XP_PC) && !(defined(_MSC_VER) && _MSC_VER >= 1800)
/* rjp -- revisit */
double rint( double a0 )
{
    int a;

    a = (int) a0;
    return a*1.0;
}
#endif


#if (defined(XP_PC) || defined(XP_MAC))

/*
** More missing RTL functions that are required by the runtime...
*/


void srand48( long seed )
{
    srand( (unsigned int) seed );
}

double drand48( void )
{
    return rand() / 32787.0;
}

#endif
/*
** Reentrant versions of the runtime functions gmtime() and localtime()
*/

#ifdef XP_PC
#include <socket_md.h>

/*
   rjp -- this assumes that the multi-threaded RTL uses tls for the
          tm structure
*/
struct tm * gmtime_r( const time_t * timer, struct tm *result )
{
    *result = *gmtime( timer );
    return result;
}

struct tm * localtime_r( const time_t *timer, struct tm * result )
{
    *result = *localtime( timer );
    return result;
}

#endif
