/* File.canonPath: javai_g.dll 10015FE0, Win32 helper 100394BA. */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "sys_api.h"
#ifdef XP_PC
#include <windows.h>
#define CANON_SIZE 260
#else
#include <limits.h>
#include <unistd.h>
#define CANON_SIZE PATH_MAX
#endif

static int canonical_path(char *path, char *result)
{
#ifdef XP_PC
    char full[CANON_SIZE], *part, *end;
    WIN32_FIND_DATAA data;
    HANDLE find;
    size_t length, count;
    int missing = 0;
    if (!_fullpath(full, path, sizeof(full))) return -1;
    /* Preserve drive or UNC prefix; recover spelling of existing components. */
    part = full;
    if (full[0] == '\\' && full[1] == '\\') {
        part = strchr(full + 2, '\\');
        if (part) part = strchr(part + 1, '\\');
        if (!part) { strcpy(result, full); return 0; }
    } else {
        part = strchr(full, '\\');
        if (!part) return -1;
    }
    length = part - full + 1;
    memcpy(result, full, length); result[length] = 0;
    part++;
    while (*part) {
        end = strchr(part, '\\');
        if (end) *end = 0;
        find = missing ? INVALID_HANDLE_VALUE : FindFirstFileA(full, &data);
        if (find != INVALID_HANDLE_VALUE) {
            FindClose(find);
            count = strlen(data.cFileName);
            if (length + count >= CANON_SIZE) return -1;
            memcpy(result + length, data.cFileName, count + 1);
        } else {
            /* Sun calls FindClose even on INVALID_HANDLE_VALUE before asking
             * GetLastError; retain that observable missing-prefix behavior. */
            if (!missing) FindClose(find);
            if (!missing && end && GetLastError() != ERROR_FILE_NOT_FOUND) return -1;
            missing = 1;
            count = strlen(part);
            if (length + count >= CANON_SIZE) return -1;
            memcpy(result + length, part, count + 1);
        }
        length += count;
        if (!end) break;
        *end = '\\'; part = end + 1;
        if (length + 1 >= CANON_SIZE) return -1;
        result[length++] = '\\'; result[length] = 0;
    }
    return 0;
#else
    /* Platform mapping: resolve existing prefixes with realpath, then retain
     * nonexistent components while normalizing dot and dot-dot. */
    char input[CANON_SIZE], resolved[CANON_SIZE], *part, *end, *slash;
    size_t length, count;
    if (strlen(path) >= sizeof(input)) return -1;
    strcpy(input, path);
    if (input[0] != '/') return -1;
    strcpy(result, "/");
    for (part = input + 1; *part; part = end) {
        end = strchr(part, '/');
        if (end) *end++ = 0; else end = part + strlen(part);
        if (!*part || !strcmp(part, ".")) continue;
        if (!strcmp(part, "..")) {
            slash = strrchr(result, '/');
            if (slash == result) result[1] = 0; else *slash = 0;
            continue;
        }
        length = strlen(result); count = strlen(part);
        if (length + count + 2 > CANON_SIZE) return -1;
        if (length > 1) result[length++] = '/';
        memcpy(result + length, part, count + 1);
        if (realpath(result, resolved)) strcpy(result, resolved);
        else if (errno != ENOENT && errno != ENOTDIR) return -1;
    }
    return 0;
#endif
}

HString *java_io_File_canonPath(HObject *self, HString *name)
{
    char *path, result[CANON_SIZE];
    int status;
    path = allocCString(name);
    if (!path) return NULL;
    status = canonical_path(sysNativePath(path), result);
    sysFree(path);
    if (status < 0) { SignalError(0, "java/io/IOException", "bad path error"); return NULL; }
    return makeJavaString(result, strlen(result));
}
