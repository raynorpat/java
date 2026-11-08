/* Isolated NSRuntime boundary test: initializer and bridge entry probes. */
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/dyld.h>
extern const char *bridgeLoadedLibraryPath(void);
void ObjCJavaInitialization(void)
{
    unsigned i;
    const char *ownPath = bridgeLoadedLibraryPath();
    if (!ownPath || !strstr(ownPath, "/bootstrap/lib/libObjCJava.dylib")) abort();
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "/usr/lib/java/") || strstr(path, "/System/Library/Frameworks/JavaVM.framework/")) abort();
    }
    puts("private ObjCJava initialized");
    fflush(stdout);
}
int _NSJavaStubEnter(void) { return 0; }
void _NSJavaStubExit(int state) {}
void *_NSRaiseExceptionInJava(void *env, id exception) { abort(); return 0; }
