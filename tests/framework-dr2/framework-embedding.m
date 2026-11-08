#import <JavaVM/JavaVM.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
@interface NSObject (FrameworkEmbeddingFixture)
- (int)add:(int)value;
- (NSString *)greeting:(NSString *)value;
- (id)identity:(id)value;
- (void)fail;
- (void)takeValue:(id)value forKey:(NSString *)key;
+ (int)answer;
@end
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSJavaVirtualMachine *vm;
    Class clazz;
    id object, custom;
    unsigned i, core = 0, bridge = 0;
    int caught = 0;
    if (argc != 3) return 2;
    vm = [NSJavaVirtualMachine defaultVirtualMachine];
    if (!vm || vm != [NSJavaVirtualMachine defaultVirtualMachine]) return 3;
    if (![vm defineClass:[NSData dataWithContentsOfFile:[NSString stringWithCString:argv[2]]]
        withName:@"FrameworkDefined"] || [[vm findClass:@"FrameworkDefined"] answer] != 115) return 11;
    clazz = [vm findClass:@"ProxyFixture"];
    if (!clazz) return 4;
    object = [[clazz alloc] init];
    custom = [clazz newWithSignature:@"(I)", 30];
    if ([object add:5] != 12 || [custom add:5] != 35
        || ![[object greeting:@"framework"] isEqual:@"Hello framework"]
        || [object identity:custom] != custom) return 5;
    [object takeValue:@"42" forKey:@"value"];
    if ([[object valueForKey:@"value"] intValue] != 42) return 6;
    NS_DURING
        [object fail];
    NS_HANDLER
        caught = [[localException name] isEqual:@"java/lang/IllegalStateException"]
            && [[localException reason] isEqual:@"proxy failure"];
    NS_ENDHANDLER
    if (!caught || [object add:1] != 43) return 7;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        char resolved[PATH_MAX];
        if (realpath(path, resolved)) path = resolved;
        if (strstr(path, "/System/Library/Frameworks/JavaVM.framework")
            || strstr(path, "/usr/lib/java/")) {
            fprintf(stderr, "donor image: %s\n", path); return 8;
        }
        if (strstr(path, "libjavai.dylib") || strstr(path, "libObjCJava.dylib")) {
            if (strncmp(path, argv[1], strlen(argv[1]))) return 9;
            if (strstr(path, "libjavai.dylib")) ++core;
            else ++bridge;
        }
    }
    if (core != 1 || bridge != 1) return 10;
    [custom release];
    [object release];
    [pool release];
    puts("PASS private framework embedding");
    return 0;
}
