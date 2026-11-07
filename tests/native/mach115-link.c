#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/dyld.h>
#include "sys_api.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static int mode, linked, added, found;
const char *sysJavaHome(void) { return "/jdk"; }
NSObjectFileImageReturnCode NSCreateObjectFileImageFromFile(const char *name, NSObjectFileImage *image)
{ *image = (void *)1; return (NSObjectFileImageReturnCode)mode; }
NSModule NSLinkModule(NSObjectFileImage image, const char *name, int bind)
{ CHECK(image == (void *)1 && bind == 1); ++linked; return found ? (void *)2 : NULL; }
int NSAddLibrary(const char *name) { ++added; return found; }
int NSIsSymbolNameDefined(const char *name) { CHECK(!strcmp(name, "_Java_test")); return found; }
NSSymbol NSLookupAndBindSymbol(const char *name) { CHECK(found); return (void *)3; }
void *NSAddressOfSymbol(NSSymbol sym) { CHECK(sym == (void *)3); return (void *)0x1234; }
int main(void)
{
    char name[80], *path;
    sysBuildLibName(name, sizeof(name), "/jdk/lib", "test");
    CHECK(!strcmp(name, "/jdk/lib/libtest.dylib"));
    sysBuildLibName(name, sizeof(name), "", "test"); CHECK(!strcmp(name, "libtest.dylib"));
    sysBuildLibName(name, 2, NULL, "test"); CHECK(name[0] == 0);
    path = sysInitializeLinker(); CHECK(path && strstr(path, "/jdk/lib") && strstr(path, "/usr/lib/java")); free(path);
    mode = NSObjectFileImageSuccess; found = 1; CHECK(sysAddDLSegment("x") && linked == 1 && added == 0);
    found = 0; CHECK(!sysAddDLSegment("x") && linked == 2);
    mode = NSObjectFileImageInappropriateFile; found = 1; CHECK(sysAddDLSegment("x") && added == 1);
    found = 0; CHECK(!sysAddDLSegment("x") && added == 2);
    mode = NSObjectFileImageFailure; CHECK(!sysAddDLSegment("x") && added == 2);
    CHECK(sysDynamicLink("Java_test") == 0);
    found = 1; CHECK(sysDynamicLink("Java_test") == 0x1234);
    puts("Mach loader contracts passed");
    return 0;
}
