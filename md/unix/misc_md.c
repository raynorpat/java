#include <sys/time.h>
#include <time.h>
#include "sys_api.h"

/* Milliseconds since 1970-01-01 UTC. */
static int64_t now_millis(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000 + tv.tv_usec / 1000;
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
    return gmtime_r(timer, result);
}

struct tm * sysLocaltime( time_t *timer, struct tm * result )
{
    return localtime_r(timer, result);
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
