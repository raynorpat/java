/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
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

DIR *sysOpenDir(const char *path) { return opendir(path); }
struct dirent *sysReadDir(DIR *dir) { return readdir(dir); }
int sysCloseDir(DIR *dir) { return dir ? closedir(dir) : 0; }

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
