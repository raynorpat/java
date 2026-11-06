/* Transitional routing of legacy diagnostic stdio calls through the Sun
 * invocation hooks. Public JNI headers remain byte-identical to the vendor. */
#ifndef JNI_IO_H
#define JNI_IO_H
#include <stdio.h>
#include <stdarg.h>
int jio_vfprintf(FILE *, const char *, va_list);
int jio_fprintf(FILE *, const char *, ...);
int jio_printf(const char *, ...);
#define fprintf jio_fprintf
#define vfprintf jio_vfprintf
#define printf jio_printf
#endif
