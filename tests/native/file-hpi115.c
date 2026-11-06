/* Primary Win32 HPI path normalization and file wrapper probe. */
#include "sys_api.h"
#include "path_md.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <windows.h>
#ifdef SUN_ORACLE
static char *(__cdecl *native_path)(char *);
static int (__cdecl *file_open)(const char *, int, int);
static int (__cdecl *file_access)(const char *, int);
static int (__cdecl *file_unlink)(const char *);
static int *(__cdecl *file_errno)(void);
#else
#define native_path sysNativePath
#define file_open sysOpen
#define file_access sysAccess
#define file_unlink sysUnlink
#define file_errno _errno
#endif
int main(int argc, char **argv)
{
    static const char *paths[] = {"", "/", "//", "/C:/a//b/", "C:\\", "a///b\\\\c/", "//server/share/", "C:relative/", "///abc"};
    char text[300];
    int i, fd;
#ifdef SUN_ORACLE
    HMODULE dll = LoadLibraryA(argv[1]);
    if (!dll) return 2;
    native_path = (void *)((char *)dll + 0x392d7);
    file_open = (void *)((char *)dll + 0x304e0);
    file_access = (void *)((char *)dll + 0x30562);
    file_unlink = (void *)((char *)dll + 0x30654);
    file_errno = (void *)GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_errno");
#endif
    for (i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        strcpy(text, paths[i]);
        printf("path=%s\n", native_path(text));
    }
    memset(text, 'x', 260); text[260] = 0;
    *file_errno() = 0;
    fd = file_open(text, O_RDONLY, 0);
    printf("long-open=%d,%d\n", fd, *file_errno());
    *file_errno() = 0;
    i = file_access(text, 0);
    printf("long-access=%d,%d\n", i, *file_errno());
    fd = file_open("file-hpi115.tmp", O_CREAT|O_TRUNC|O_RDWR, 0600);
    if (fd < 0) return 3;
    /* Sun and current process use different CRTs: close in their owner. */
#ifdef SUN_ORACLE
    ((int (__cdecl *)(int))GetProcAddress(GetModuleHandleA("msvcrt.dll"), "_close"))(fd);
#else
    sysClose(fd);
#endif
    SetFileAttributesA("file-hpi115.tmp", FILE_ATTRIBUTE_READONLY);
    printf("readonly-delete=%d\n", file_unlink("file-hpi115.tmp"));
    SetFileAttributesA("file-hpi115.tmp", FILE_ATTRIBUTE_NORMAL);
    DeleteFileA("file-hpi115.tmp");
    return 0;
}
