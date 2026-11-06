/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Win32 file HPI: javai_g.dll 100304e0..10030763, path 100392d7. */
#include "sys_api.h"
#include "path_md.h"
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <string.h>

char *sysNativePath(char *path)
{
    char *read = path, *write = path;
    int length = 0;
    if (*read == '/' || *read == '\\') {
        while (*read == '/' || *read == '\\') read++;
        if (!isalpha((unsigned char)*read) || read[1] != ':') {
            *write++ = '\\';
            length++;
        }
    }
    while (*read) {
        if (*read == '/' || *read == '\\') {
            if (!length || write[-1] != '\\') {
                *write++ = '\\';
                length++;
            }
            read++;
        } else {
            *write++ = *read++;
            length++;
        }
    }
    if (length > 1 && write[-1] == '\\' &&
        !(length == 3 && isalpha((unsigned char)write[-3]) && write[-2] == ':'))
        write--;
    *write = 0;
    return path;
}

static int nativeFilePath(const char *path, char *copy)
{
    if (strlen(path) >= MAX_PATH) { errno = ENAMETOOLONG; return -1; }
    strcpy(copy, path);
    sysNativePath(copy);
    return 0;
}

int sysOpen(const char *path, int flags, int mode)
{
    char copy[MAX_PATH];
    if (nativeFilePath(path, copy)) return -1;
    return _open(copy, flags | _O_BINARY, mode);
}
int sysAccess(const char *path, int mode)
{
    char copy[MAX_PATH];
    if (nativeFilePath(path, copy)) return -1;
    return _access(copy, mode);
}
int sysStat(const char *path, struct stat *st)
{
    char copy[MAX_PATH];
    if (nativeFilePath(path, copy)) return -1;
    return stat(copy, st);
}
int sysFStat(int fd, struct stat *st)
{
    return fstat(fd, st);
}
int sysUnlink(const char *path)
{
    char copy[MAX_PATH];
    if (nativeFilePath(path, copy)) return -1;
    SetFileAttributesA(copy, 0);
    return DeleteFileA(copy) ? 0 : -1;
}
int sysMkdir(const char *path, int mode)
{
    char copy[MAX_PATH];
    if (nativeFilePath(path, copy)) return -1;
    return _mkdir(copy);
}
