/* DR2 5C1355B0-5C135788: native Mach-O loading and global symbol lookup. */
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "path_md.h"

void sysSaveLDPath(char *path) { putenv(path); }

void sysBuildLibName(char *out, int size, char *path, char *name)
{
    size_t prefix = path && *path ? strlen(path) + 1 : 0;
    if (size <= 0) return;
    if (prefix + strlen(name) + sizeof("lib.dylib") > (size_t)size) {
        *out = 0; return;
    }
    if (prefix) sprintf(out, "%s/lib%s.dylib", path, name);
    else sprintf(out, "lib%s.dylib", name);
}

char *sysInitializeLinker(void)
{
    const char *path = getenv("DYLD_LIBRARY_PATH");
    const char *home = sysJavaHome();
    /* A missing reconstructed library must not silently load a donor binary. */
    const char *defaults = ".:/usr/lib:/usr/lib/java:/lib";
    char *result;
    if (!path) path = "";
    /* Retain this reconstruction's JDK image location and caller ownership. */
    result = malloc(strlen(home) + strlen(path) + strlen(defaults) + 8);
    if (result) sprintf(result, "%s/lib:%s:%s", home, path, defaults);
    return result;
}

int sysAddDLSegment(char *path)
{
    NSObjectFileImage image;
    NSObjectFileImageReturnCode result = NSCreateObjectFileImageFromFile(path, &image);
    if (result == NSObjectFileImageSuccess)
        return NSLinkModule(image, path, 1) != NULL;
    /* DR2 SDK marks NSDestroyObjectFileImage unimplemented. */
    if (result == NSObjectFileImageInappropriateFile) return NSAddLibrary(path) != 0;
    return 0;
}

long sysDynamicLink(char *name)
{
    char *native = malloc(strlen(name) + 2);
    NSSymbol symbol;
    long address = 0;
    if (!native) return 0;
    /* Shared VM/JNI callers supply C names; Mach-O prefixes an underscore. */
    native[0] = '_'; strcpy(native + 1, name);
    if (NSIsSymbolNameDefined(native)) {
        symbol = NSLookupAndBindSymbol(native);
        if (symbol) address = (long)NSAddressOfSymbol(symbol);
    }
    free(native);
    return address;
}
