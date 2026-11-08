#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern char *sysInitializeLinker(void);
const char *sysJavaHome(void) { return "/private/jdk115"; }
int main(void)
{
    char *path;
    putenv("DYLD_LIBRARY_PATH=");
    path = sysInitializeLinker();
    if (!path || strncmp(path, "/private/jdk115/lib:", 19) ||
        strstr(path, "JavaVM.framework")) return 1;
    free(path);
    puts("DR2 native path excludes donor framework");
    return 0;
}
