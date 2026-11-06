/* Sun 1.1.5 runtime ABI helpers; see exports115-evidence.md. */
#include "tree.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdarg.h>
#include "jni.h"
#ifdef XP_PC
#include <windows.h>
#endif
#undef printf
#undef fprintf
#undef vfprintf

/* Sun1004E3DE allows the embedding VM to redirect formatted output. */
int (JNICALL *jio_vfprintf_hook)(FILE *, const char *, va_list);

int jio_vfprintf(FILE *stream, const char *format, va_list args)
{
    return jio_vfprintf_hook ? jio_vfprintf_hook(stream, format, args)
                            : vfprintf(stream, format, args);
}
int jio_fprintf(FILE *stream, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = jio_vfprintf(stream, format, args);
    va_end(args);
    return result;
}
int jio_printf(const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = jio_vfprintf(stdout, format, args);
    va_end(args);
    return result;
}

/* Transitional mapping: use this tree's ClassClass pointer and Sticky flag. */
void MakeClassSticky(ClassClass *cb)
{ cbFlags(cb) |= CCF_IsSticky; }

ClassClass *FindStickySystemClass(ExecEnv *ee, char *name, bool_t resolve)
{
    ClassClass *cb = FindClassFromClass(ee, name, resolve, NULL);
    if (cb) MakeClassSticky(cb);
    return cb;
}

#ifdef XP_PC
int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}
#endif
