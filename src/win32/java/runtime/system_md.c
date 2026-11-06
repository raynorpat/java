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
#ifdef JNI115_ENABLED
    {
        extern int jni_InvokeExitHook(int);
        if (jni_InvokeExitHook(rv)) return;
    }
#endif
    exit(rv);
}

EXPORTED int sysAtexit(void (*func)(void))
{
    return 0;
}

EXPORTED void sysAbort(void)
{
#ifdef JNI115_ENABLED
    extern int jni_InvokeAbortHook(void);
    if (jni_InvokeAbortHook()) return;
#endif
    abort();
}
