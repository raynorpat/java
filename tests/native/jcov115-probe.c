/* Writer oracle: SUN_ORACLE calls private Sun Debug writer 1003A02F with
 * original-layout synthetic class metadata. No JVM execution is mocked. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tree.h"
#include "interpreter.h"
#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifndef SUN_ORACLE
ClassClass **binclasses;
long nbinclasses;
/* Single-threaded probe: check balanced registry locking without a VM. */
struct sys_mon *_binclass_lock;
static int registry_locked;
int sysMonitorEnter(struct sys_mon *monitor) {
    (void)monitor;
    if (registry_locked) abort();
    registry_locked = 1;
    return 0;
}
int sysMonitorExit(struct sys_mon *monitor) {
    (void)monitor;
    if (!registry_locked) abort();
    registry_locked = 0;
    return 0;
}
int sysOpen(const char *p, int flags, int mode) { return open(p, flags, mode); }
int sysClose(int fd) { return close(fd); }
int sysUnlink(const char *p) { return unlink(p); }
int sysAccess(const char *p, int mode) { return access(p, mode); }
int sysStat(const char *p, struct stat *st) { return stat(p, st); }
int sysMkdir(const char *p, int mode) {
#ifdef WIN32
    (void)mode; return _mkdir(p);
#else
    return mkdir(p, mode);
#endif
}
void ll2str(int64_t value, char *p, char *limit) {
#ifdef WIN32
    _snprintf(p, limit - p, "%I64d", value);
#else
    snprintf(p, limit - p, "%lld", (long long)value);
#endif
}
int jio_fprintf(FILE *stream, const char *format, ...) {
    int result;
    va_list args;
    va_start(args, format); result = vfprintf(stream, format, args); va_end(args);
    return result;
}
#endif

int main(int argc, char **argv)
{
    Classjava_lang_Class body;
    ClassClass handle, *classes[1];
    struct methodblock method;
    struct covtable entries[3];
    char **test_name;
    int (*writer)(char *);
    int result;
#ifdef SUN_ORACLE
    HMODULE module;
    if (argc != 3) return 2;
    module = LoadLibraryA(argv[2]);
    if (!module) return 3;
    writer = (int (*)(char *))((char *)module + 0x3a02f);
    test_name = (char **)((char *)module + 0x71870);
    *(ClassClass ***)((char *)module + 0x83550) = classes;
    *(long *)((char *)module + 0x8351c) = 1;
#else
    if (argc != 2) return 2;
    writer = jcov_write_data;
    test_name = &testname;
    binclasses = classes;
    nbinclasses = 1;
#endif
    memset(&body, 0, sizeof(body)); memset(&handle, 0, sizeof(handle));
    memset(&method, 0, sizeof(method)); memset(entries, 0, sizeof(entries));
    handle.obj = &body; classes[0] = &handle;
    cbName(&handle) = "JCovSynthetic115";
    cbAbsoluteSourceName(&handle) = "JCovSynthetic115.java";
    cbTimestamp(&handle) = int2ll(1);
    cbMethods(&handle) = &method; cbMethodsCount(&handle) = 1;
    method.coverage_table = entries; method.coverage_table_length = 3;
    entries[0].type = CT_METHOD; entries[0].where = (1UL << 18) + 4; entries[0].count = 2;
    entries[1].type = CT_CASE; /* omitted placeholder */
    entries[2].type = CT_BLOCK; entries[2].where = (2UL << 18) + 8;
    *test_name = "one";
    result = writer(argv[1]); if (result) return 10 + result;
    entries[0].count = 0; entries[2].count = 3;
    *test_name = "two";
    result = writer(argv[1]); if (result) return 20 + result;
    entries[0].count = 1; entries[2].count = 1;
    *test_name = NULL;
    result = writer(argv[1]); if (result) return 30 + result;
#ifndef SUN_ORACLE
    if (registry_locked) return 40;
#endif
    return 0;
}
