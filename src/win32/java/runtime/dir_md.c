/* Win32 directory ABI recovered from javai_g.dll 100155b0..10015908. */
#include <windows.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sys_api.h"

DIR *opendir(const char *path)
{
    DIR *dir = (DIR *)malloc(sizeof(DIR));
    char root[4] = {0, 0, 0, 0};
    char *p;
    if (dir == NULL) { errno = ENOMEM; return NULL; }
    /* Preserve the primary's root-path test, including leading '/'. */
    if ((path[1] == 0 && path[0] == '\\') || path[0] == '/') {
        root[0] = (char)(_getdrive() + 'A' - 1);
        strcpy(root + 1, ":\\");
        path = root;
    }
    dir->path = (char *)malloc(strlen(path) + 5);
    if (dir->path == NULL) { free(dir); errno = ENOMEM; return NULL; }
    strcpy(dir->path, path);
    for (p = dir->path; *p; ++p) if (*p == '/') *p = '\\';
    if (GetFileAttributesA(dir->path) == INVALID_FILE_ATTRIBUTES) {
        free(dir->path); free(dir); errno = ENOENT; return NULL;
    }
    strcat(dir->path, root[1] ? "*.*" : "\\*.*");
    dir->handle = FindFirstFileA(dir->path, &dir->find_data);
    if (dir->handle == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) {
        free(dir->path); free(dir); errno = EACCES; return NULL;
    }
    return dir;
}

struct dirent *readdir(DIR *dir)
{
    if (dir->handle == INVALID_HANDLE_VALUE) return NULL;
    strcpy(dir->dirent.d_name, dir->find_data.cFileName);
    if (!FindNextFileA(dir->handle, &dir->find_data)) {
        if (GetLastError() == ERROR_INVALID_HANDLE) { errno = EBADF; return NULL; }
        FindClose(dir->handle);
        dir->handle = INVALID_HANDLE_VALUE;
    }
    return &dir->dirent;
}

int closedir(DIR *dir)
{
    if (dir->handle != INVALID_HANDLE_VALUE) {
        if (!FindClose(dir->handle)) { errno = EBADF; return -1; }
        dir->handle = INVALID_HANDLE_VALUE;
    }
    free(dir->path);
    free(dir);
    return 0;
}

/* Header-contract implementation: no linked rewinddir body in primary. */
void rewinddir(DIR *dir)
{
    if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
    dir->handle = FindFirstFileA(dir->path, &dir->find_data);
}

DIR *sysOpenDir(const char *path) { return opendir(path); }
struct dirent *sysReadDir(DIR *dir) { return readdir(dir); }
int sysCloseDir(DIR *dir) { return closedir(dir); }

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
