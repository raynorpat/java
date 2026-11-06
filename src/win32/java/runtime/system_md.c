#include <string.h>
#include <stdarg.h>

#include "typedefs_md.h"
#include "jmath_md.h"
#include "sys_api.h"

void ll2str(int64_t a, char *s, char *limit)
{
    _snprintf(s, limit - s, "%I64d", a);
    limit[-1] = 0;
}

void (*sysFinalizersOnExit)(int);

EXPORTED void sysExit(int rv)
{
    if (sysFinalizersOnExit) sysFinalizersOnExit(rv);
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
