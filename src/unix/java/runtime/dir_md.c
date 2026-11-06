/*
 * Unix directory listing (java.io.File.list) and OS identification.
 */
#include <sys/types.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys_api.h"

struct sys_dir {
    DIR *dir;
    sys_dirent_t entry;
};

sys_dir_t *sysOpenDir(const char *path)
{
    sys_dir_t *dir = (sys_dir_t *) calloc(1, sizeof *dir);

    if (dir == NULL)
	return NULL;
    if ((dir->dir = opendir(path)) == NULL) {
	free(dir);
	return NULL;
    }
    return dir;
}

sys_dirent_t *sysReadDir(sys_dir_t *dir)
{
    struct dirent *d = readdir(dir->dir);

    if (d == NULL)
	return NULL;
    dir->entry.d_name = d->d_name;
    return &dir->entry;
}

int sysCloseDir(sys_dir_t *dir)
{
    int result = 0;
    if (dir) {
	result = closedir(dir->dir);
	free(dir);
    }
    return result;
}

/* os.name and os.version, as reported since JDK 1.0: uname -s and -r. */
void sysOSInfo(char *name, int namelen, char *version, int versionlen)
{
    struct utsname u;

    if (uname(&u) != 0) {
	strcpy(u.sysname, "Unix");
	strcpy(u.release, "0.0");
    }
    strncpy(name, u.sysname, namelen);
    strncpy(version, u.release, versionlen);
    name[namelen - 1] = version[versionlen - 1] = 0;
}
