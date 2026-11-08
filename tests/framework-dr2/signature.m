#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DONOR
static char *(*convert)(const char *);
#else
extern char *NSJavaSignatureToObjc(const char *);
#define convert NSJavaSignatureToObjc
#endif

int main(void)
{
    const char *signatures[] = {
        "()V", "(I)I", "(ZBC)C", "(S)S", "(J)J", "(F)F", "(D)D",
        "(Ljava/lang/String;)Ljava/lang/Object;", "([I)V", "([[I)V",
        "([Ljava/lang/String;)V", "([[Ljava/lang/Object;I)J", "(I", "invalid", "()Q"
    };
    unsigned i;
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    convert = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaSignatureToObjc"));
#endif
    for (i = 0; i < sizeof(signatures)/sizeof(signatures[0]); ++i) {
        char *converted = convert(signatures[i]);
        printf("%s=%s\n", signatures[i], converted ? converted : "<nil>");
        free(converted);
    }
    [pool release];
    return 0;
}
