/*
 * Win32 directory listing (java.io.File.list) and OS identification.
 */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sys_api.h"

struct sys_dir {
    HANDLE handle;
    WIN32_FIND_DATAA data;
    int pending;		/* data holds an entry not yet returned */
    sys_dirent_t entry;
};

sys_dir_t *sysOpenDir(const char *path)
{
    char pattern[MAX_PATH + 3];
    size_t len = strlen(path);
    sys_dir_t *dir;

    if (len == 0 || len + 3 > sizeof pattern)
	return NULL;
    strcpy(pattern, path);
    if (pattern[len - 1] != '\\' && pattern[len - 1] != '/')
	pattern[len++] = '\\';
    strcpy(pattern + len, "*");

    if ((dir = (sys_dir_t *) calloc(1, sizeof *dir)) == NULL)
	return NULL;
    dir->handle = FindFirstFileA(pattern, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
	/* An empty directory still has "." and ".."; failure means no directory. */
	free(dir);
	return NULL;
    }
    dir->pending = 1;
    return dir;
}

sys_dirent_t *sysReadDir(sys_dir_t *dir)
{
    if (!dir->pending && !FindNextFileA(dir->handle, &dir->data))
	return NULL;
    dir->pending = 0;
    dir->entry.d_name = dir->data.cFileName;
    return &dir->entry;
}

int sysCloseDir(sys_dir_t *dir)
{
    if (dir) {
	if (!FindClose(dir->handle)) { errno = EBADF; return -1; }
	free(dir);
    }
    return 0;
}

/* os.name and os.version, as reported since JDK 1.0. */
void sysOSInfo(char *name, int namelen, char *version, int versionlen)
{
    OSVERSIONINFOA info;

    info.dwOSVersionInfoSize = sizeof(info);
    if (!GetVersionExA(&info)) {
	info.dwPlatformId = VER_PLATFORM_WIN32_NT;
	info.dwMajorVersion = info.dwMinorVersion = 0;
    }
    _snprintf(name, namelen, "%s",
	      info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ? "Windows 95" : "Windows NT");
    _snprintf(version, versionlen, "%lu.%lu", info.dwMajorVersion, info.dwMinorVersion);
    name[namelen - 1] = version[versionlen - 1] = 0;
}
