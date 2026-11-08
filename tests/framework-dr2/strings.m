#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static void *(*toJava)(id);
static id (*toObjc)(void *);
#else
extern void *JAVALoadVirtualMachine(NSString *, id);
extern void *_NSStringToJavaString(id);
extern id _JavaStringToNSString(void *);
#define loadVM JAVALoadVirtualMachine
#define toJava _NSStringToJavaString
#define toObjc _JavaStringToNSString
#endif

static int roundtrip(NSString *text, const char *name)
{
    void *handle = toJava(text);
    NSString *result = toObjc(handle);
    unsigned i;
    if (!handle || ![text isEqual:result]) return 1;
    printf("%s=%u:", name, [result length]);
    for (i = 0; i < [result length]; ++i)
        printf("%04x", [result characterAtIndex:i]);
    putchar('\n');
    return 0;
}

int main(int argc, char **argv)
{
    unichar characters[] = {0x0041, 0, 0x00e9, 0x4e2d, 0xd83d, 0xde00, 0xffff};
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSString *unicodeText;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
    toJava = NSAddressOfSymbol(NSLookupAndBindSymbol("__NSStringToJavaString"));
    toObjc = NSAddressOfSymbol(NSLookupAndBindSymbol("__JavaStringToNSString"));
#endif
    printf("nil=%d,%d\n", toJava(nil) == NULL, toObjc(NULL) == nil);
    if (!loadVM([NSString stringWithCString:argv[1]], nil)) return 1;
    unicodeText = [[[NSString alloc] initWithCharacters:characters length:7] autorelease];
    if (roundtrip(@"", "empty") || roundtrip(@"hello", "ascii") ||
        roundtrip(unicodeText, "unicode")) return 1;
    [pool release];
    return 0;
}
