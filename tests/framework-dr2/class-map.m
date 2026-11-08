#import <Foundation/Foundation.h>
#include <stdio.h>
#ifdef DONOR
#include <mach-o/dyld.h>
#import <JavaVM/JavaVM.h>
static void *(*registerClass)(const char *, Class);
static void *(*infoForObjc)(Class, int);
static void *(*infoForName)(const char *);
static void *(*infoForJava)(void *, int);
static void *(*lookupJava)(const char *);
static void *(*objcToJava)(Class);
static void (*registerConstructors)(Class, int, void *);
static const char *(*constructorSignature)(Class, SEL);
static SEL (*constructorSelector)(Class, const char *);
#else
#import "class-map.h"
#import "vm.h"
#define registerClass NSSetJavaClassForObjcClass
#define infoForObjc _BRIDGEJavaInfoForObjcClass
#define infoForName _BRIDGEJavaInfoForJavaClassName
#define infoForJava _BRIDGEJavaInfoForJavaClass
#define lookupJava NSJavaLookupClassName
#define objcToJava NSObjcClassToJavaClass
#define registerConstructors NSRegisterJavaConstructors
#define constructorSignature NSGetJavaConstructorSignature
#define constructorSelector NSSelectorForJavaConstructor
#endif

@interface ClassMapParent : NSObject @end
@implementation ClassMapParent @end
@interface ClassMapChild : ClassMapParent @end
@implementation ClassMapChild @end
@interface ClassMapOther : NSObject @end
@implementation ClassMapOther @end

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    void *record, *base, *child;
    struct { SEL selector; const char *signature; } constructors[1];
    if (argc != 2) return 2;
#ifdef DONOR
    [[[NSJavaVirtualMachine alloc] initWithClassPath:[NSString stringWithCString:argv[1]]] autorelease];
    registerClass = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetJavaClassForObjcClass"));
    infoForObjc = NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEJavaInfoForObjcClass"));
    infoForName = NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEJavaInfoForJavaClassName"));
    infoForJava = NSAddressOfSymbol(NSLookupAndBindSymbol("__BRIDGEJavaInfoForJavaClass"));
    lookupJava = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaLookupClassName"));
    objcToJava = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSObjcClassToJavaClass"));
    registerConstructors = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSRegisterJavaConstructors"));
    constructorSignature = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSGetJavaConstructorSignature"));
    constructorSelector = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSelectorForJavaConstructor"));
#else
    if (!JAVALoadVirtualMachine([NSString stringWithCString:argv[1]], nil)) return 1;
    bridgeInitializeClassMap();
#endif
    record = registerClass("ClassMapBase", [ClassMapParent class]);
    if (!record) return 1;
    printf("registered=%d\n", infoForName("ClassMapBase") == record);
    printf("exact=%d\n", infoForObjc([ClassMapParent class], 1) == record);
    printf("inherited=%d\n", infoForObjc([ClassMapChild class], 0) == record);
    printf("strict-child=%d\n", infoForObjc([ClassMapChild class], 1) == NULL);
    base = lookupJava("ClassMapBase");
    child = lookupJava("ClassMapFixture");
    if (!base || !child) return 1;
    printf("class-cache=%d\n", objcToJava([ClassMapParent class]) == base);
    printf("child-cache=%d\n", objcToJava([ClassMapChild class]) == base);
    printf("java-exact=%d\n", infoForJava(base, 1) == record);
    printf("java-inherited=%d\n", infoForJava(child, 0) == record);
    printf("java-strict=%d\n", infoForJava(child, 1) == NULL);
    printf("nil=%d,%d\n", lookupJava(NULL) == NULL, objcToJava(Nil) == NULL);
    constructors[0].selector = @selector(initWithFixture:);
    constructors[0].signature = "(I)";
    registerConstructors([ClassMapParent class], 1, (void *)constructors);
    printf("ctor-signature=%s\n", constructorSignature([ClassMapParent class], @selector(initWithFixture:)));
    printf("ctor-selector=%s\n", sel_getName(constructorSelector([ClassMapParent class], "(I)V")));
    printf("default-selector=%s\n", sel_getName(constructorSelector([ClassMapParent class], "()V")));
    printf("default-signature=%s\n", constructorSignature([ClassMapParent class], @selector(init)));
    printf("missing-constructor=%d\n", constructorSelector([ClassMapParent class], "(J)V") == NULL);
    printf("duplicate-java=%d\n", registerClass("ClassMapBase", [ClassMapOther class]) == NULL);
    printf("duplicate-objc=%d\n", registerClass("ClassMapFixture", [ClassMapParent class]) == NULL);
    [pool release];
    return 0;
}
