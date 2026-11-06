#include <stdlib.h>
#include <string.h>
#include "typedefs_md.h"
#include "sys_api.h"

/* Decimal, without printf's "%lld", which C89 lacks. */
void ll2str(int64_t a, char *s, char *limit)
{
    char digits[24];
    int n = 0;
    uint64_t u = a < 0 ? (uint64_t) 0 - (uint64_t) a : (uint64_t) a;

    do {
	digits[n++] = (char) ('0' + (int) (u % 10));
	u /= 10;
    } while (u != 0);
    if (a < 0)
	digits[n++] = '-';
    while (n > 0 && s < limit - 1)
	*s++ = digits[--n];
    *s = 0;
}

EXPORTED void sysExit(int rv)
{
    exit(rv);
}

EXPORTED int sysAtexit(void (*func)(void))
{
    return 0;
}

EXPORTED void sysAbort(void)
{
    abort();
}
