#import <JavaVM/JavaVM.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    unsigned i, found = 0;
    char expected[PATH_MAX];
    if (sizeof(JAVAHandle) != sizeof(void *) || sizeof(JAVAClass) != sizeof(void *)) return 5;
    if (argc != 2 || !realpath(argv[1], expected)) return 1;
    printf("%s\n", [NSStringFromClass([NSJavaVirtualMachine class]) cString]);
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "JavaVM.framework/")) {
            char actual[PATH_MAX];
            if (!realpath(path, actual) || strncmp(actual, expected, strlen(expected)) || actual[strlen(expected)] != '/') return 2;
            ++found;
        }
    }
    [pool release];
    return found != 1;
}
