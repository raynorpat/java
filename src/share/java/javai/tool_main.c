/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Reconstructed JDK 1.1.5 Java tool launchers; see launcher115-evidence.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "sys_api.h"
#include "path_md.h"

extern JAVA_PUBLIC_API(int) java_main(int argc, char **argv);

#ifndef TOOL_HEAP
#define TOOL_HEAP "-ms8m"
#endif

int main(int argc, char **argv)
{
    const char *home = sysJavaHome();
    const char *oldpath = getenv("CLASSPATH");
    char *path = NULL;
    char *home_property = NULL;
    char **args;
    /* java_main normalizes the class name in place, as Sun does. */
    char tool_class[] = TOOL_CLASS;
    int i, count = 0, result;
#ifdef TOOL_APPLET
    int debug = 0, arguments = 0;
    char debugger_class[] = "sun.tools.ttydebug.TTY";
    const char *configured_home = getenv("JAVA_HOME");
    struct stat info;
    for (i = 1; i < argc; ++i)
        if (strncmp(argv[i], "-J", 2)) ++arguments;
    if (arguments == 0) {
        fprintf(stderr, "usage: appletviewer [-debug] [-J<runtime flag>] url|file ...\n");
        return 1;
    }
    if (configured_home != NULL) {
        if (*configured_home == 0 || stat(configured_home, &info) != 0 ||
            !(info.st_mode & S_IFDIR)) {
            fprintf(stderr, "Invalid JAVA_HOME: %s\n", configured_home);
            return 1;
        }
    }
#endif

    args = calloc(argc + 5, sizeof(char *));
    if (args == NULL) return 1;
#ifndef TOOL_SIMPLE
#ifdef TOOL_APPLET
    if (!oldpath) oldpath = "";
#else
    if (!oldpath) oldpath = ".";
#endif
    path = malloc(strlen(oldpath) + 2 * strlen(home) + 64);
    if (!path) {
        free(args);
        return 1;
    }
#ifdef XP_PC
    sprintf(path, "CLASSPATH=%s%s%s\\classes;%s\\lib\\classes.zip",
            oldpath, *oldpath ? ";" : "", home, home);
#else
    /* deviation: platform separators for the pthreads port. */
    sprintf(path, "CLASSPATH=%s%s%s/classes:%s/lib/classes.zip",
            oldpath, *oldpath ? ":" : "", home, home);
#endif
    putenv(path);
#endif
    args[count++] = "java";
    if (*TOOL_HEAP) args[count++] = TOOL_HEAP;
#ifdef TOOL_APPLET
    home_property = malloc(strlen(configured_home ? configured_home : home) + 16);
    if (home_property == NULL) { free(args); return 1; }
    sprintf(home_property, "-Djava.home=%s", configured_home ? configured_home : home);
    args[count++] = home_property;
#endif
#ifndef TOOL_SIMPLE
    for (i = 1; i < argc; i++)
        if (!strncmp(argv[i], "-J", 2)) args[count++] = argv[i] + 2;
#endif
#ifdef TOOL_APPLET
    for (i = 1; i < argc; ++i)
        if (!strcmp(argv[i], "-debug")) debug = 1;
    if (debug) args[count++] = debugger_class;
#endif
    args[count++] = tool_class;
    for (i = 1; i < argc; i++) {
#ifdef TOOL_APPLET
        if (!strcmp(argv[i], "-debug")) continue;
#endif
#ifndef TOOL_SIMPLE
        if (strncmp(argv[i], "-J", 2)) args[count++] = argv[i];
#else
        args[count++] = argv[i];
#endif
    }
    result = java_main(count, args);
    free(args);
    free(home_property);
    /* putenv retains path for the lifetime of this process. */
    return result;
}
