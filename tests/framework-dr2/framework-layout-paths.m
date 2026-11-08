#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
    char root[PATH_MAX], library[PATH_MAX], home[PATH_MAX], expected[PATH_MAX];
    const char *(*javaHome)(void);
    char *(*linkerPath)(void);
    char *actual;
    if (argc != 2 || !realpath(argv[1], root)) return 1;
    sprintf(library, "%s/Libraries/libjavai.dylib", root);
    if (!NSAddLibrary(library)) return 2;
    javaHome = NSAddressOfSymbol(NSLookupAndBindSymbol("_sysJavaHome"));
    linkerPath = NSAddressOfSymbol(NSLookupAndBindSymbol("_sysInitializeLinker"));
    sprintf(home, "%s/Home", root);
    sprintf(expected, "%s/lib", home);
    actual = linkerPath();
    printf("home=%s\nlinker=%s\n", javaHome(), actual);
    if (strcmp(javaHome(), home) || strcmp(actual, expected)) return 3;
    free(actual);
    return 0;
}
