/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
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

/* Primary 1004BFFA and Rhapsody 5C135ED4: private LIFO exit callbacks. */
struct exit_callback115 {
    void (*function)(void);
    struct exit_callback115 *next;
};
static struct exit_callback115 *exit_callbacks;

EXPORTED void sysExit(int rv)
{
    struct exit_callback115 *callback;
    if (sysFinalizersOnExit) sysFinalizersOnExit(rv);
    while ((callback = exit_callbacks) != NULL) {
        /* deviation: unlink before calling so a returning JNI exit hook or
         * reentrant exit cannot revisit freed callback nodes. */
        exit_callbacks = callback->next;
        callback->function();
        free(callback);
    }
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
    struct exit_callback115 *callback = malloc(sizeof(*callback));
    if (!callback) return 1;
    callback->function = func;
    callback->next = exit_callbacks;
    exit_callbacks = callback;
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
