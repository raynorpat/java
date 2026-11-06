/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
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
