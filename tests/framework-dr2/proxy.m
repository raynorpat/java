#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef DONOR
#include <mach-o/dyld.h>
#import <JavaVM/JavaVM.h>
static void *(*lookupHandle)(void *);
#else
#import "objects.h"
#import "vm.h"
#import "bridge-exceptions.h"
#import "retention.h"
#import "object-map.h"
#import "selectors.h"
extern void *_NSStringToJavaString(id);
extern id _JavaStringToNSString(void *);
#define lookupHandle NSJavaLookupHandle
#endif
@interface NSObject (ProxyFixtureMethods)
+ (id)newWithSignature:(NSString *)signature, ...;
#ifdef DONOR
- (void *)javaHandle;
#else
- (HObject *)javaHandle;
#endif
- (int)add:(int)value;
+ (int)twice:(int)value;
- (long long)wide:(long long)value;
- (float)single:(float)value;
- (double)fraction:(double)value;
- (NSString *)greeting:(NSString *)value;
- (id)identity:(id)value;
- (int)inherited;
- (int)arrayArgument:(id)value;
- (void)fail;
@end
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    Class clazz;
    id object, custom;
    void *handle;
#ifdef DONOR
    NSJavaVirtualMachine *vm;
#endif
    if (argc != 2) return 2;
#ifdef DONOR
    vm = [[[NSJavaVirtualMachine alloc]
        initWithClassPath:[NSString stringWithCString:argv[1]]] autorelease];
    clazz = [vm findClass:@"ProxyFixture"];
    lookupHandle = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaLookupHandle"));
#else
    if (!JAVALoadVirtualMachine([NSString stringWithCString:argv[1]], nil)) return 1;
    JavaGCInitialization(); ExceptionInitialization();
    MappingInitialization(); bridgeInitializeSelectorMaps();
    NSSetJavaClassMappings("java/lang/String", "NSString", _JavaStringToNSString, _NSStringToJavaString);
    NSSetJavaClassMappings("java/lang/Throwable", "NSException", _JavaExceptionToNSException, _NSExceptionToJavaException);
    clazz = NSJavaClassToObjcClass(NSJavaLookupClassName("ProxyFixture"), YES);
#endif
    if (!clazz) return 1;
    object = [[clazz alloc] init];
    printf("initial-retains=%u\n", [object retainCount]);
    custom = [clazz newWithSignature:@"(I)", 30];
    printf("default=%d\n", [object add:5]);
    printf("constructor=%d\n", [custom add:5]);
    printf("static=%d\n", [clazz twice:9]);
    printf("inherited=%d\n", [object inherited]);
    printf("wide=%016llx\n", [object wide:0x0102030405060708LL]);
    printf("wide-zero=%016llx\n", [object wide:0LL]);
    printf("wide-negative=%016llx\n", [object wide:-1LL]);
    printf("float=%.2f\n", [object single:1.25f]);
    printf("double=%.2f\n", [object fraction:-1.25]);
    printf("string=%s\n", [[object greeting:@"world"] cString]);
    printf("identity=%d\n", [object identity:custom] == custom);
    printf("description=%s\n", [[object description] cString]);
    printf("responds=%d,%d\n", [object respondsToSelector:@selector(add:)], [object respondsToSelector:@selector(noSuchMethod)]);
    NS_DURING
        [object arrayArgument:nil];
        puts("array=not-rejected");
    NS_HANDLER
        printf("array=%s|%s\n", [[localException name] cString], [[localException reason] cString]);
    NS_ENDHANDLER
    NS_DURING
        [object fail];
        puts("exception=not-raised");
    NS_HANDLER
        printf("exception=%s|%s\n", [[localException name] cString], [[localException reason] cString]);
    NS_ENDHANDLER
    handle = [object javaHandle];
    printf("mapped=%d\n", lookupHandle(handle) == object);
    printf("retains=%u\n", [object retainCount]);
    [object release];
    printf("unmapped=%d\n", lookupHandle(handle) == nil);
    [custom release];
    [pool release];
    return 0;
}
