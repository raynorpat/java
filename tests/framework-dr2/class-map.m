#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string.h>
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
static void (*setMappings)(const char *, const char *, void *, void *);
static void (*overloadMappings)(const char *, const char *, void *, void *);
static void (*setTypeMapping)(const char *, void *, void *, unsigned);
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
#define setMappings NSSetJavaClassMappings
#define overloadMappings NSSetOverloadedJavaClassMappings
#define setTypeMapping BRIDGESetJavaTypeMapping
#endif

@interface ClassMapParent : NSObject @end
@implementation ClassMapParent @end
@interface ClassMapChild : ClassMapParent @end
@implementation ClassMapChild @end
@interface ClassMapOther : NSObject @end
@implementation ClassMapOther @end
@interface ClassMapConverted : NSObject @end
@implementation ClassMapConverted @end

/* Donor JavaInfo ABI, inspected independently of the reconstructed header. */
struct InfoView {
    void *toJava, *toObjc;
    const char *javaName;
    void *javaClass;
    Class objcClass;
    int constructorCount;
    void *constructors;
    unsigned structSize;
};
static void conversionOne(void) {}
static void conversionTwo(void) {}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    void *record, *base, *child;
    struct InfoView *mapped, *type;
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
    setMappings = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetJavaClassMappings"));
    overloadMappings = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSSetOverloadedJavaClassMappings"));
    setTypeMapping = NSAddressOfSymbol(NSLookupAndBindSymbol("_BRIDGESetJavaTypeMapping"));
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
    setMappings("fixture/Converted", "ClassMapConverted", (void *)conversionOne, (void *)conversionTwo);
    mapped = (struct InfoView *)infoForObjc([ClassMapConverted class], 1);
    printf("conversion-map=%d\n", mapped && mapped->toObjc == (void *)conversionOne && mapped->toJava == (void *)conversionTwo);
    setMappings("fixture/Rejected", "ClassMapConverted", NULL, NULL);
    printf("conversion-duplicate=%d\n", mapped->toObjc == (void *)conversionOne && infoForName("fixture/Rejected") == NULL);
    setMappings("fixture/Unknown", "ClassMapNoSuchClass", NULL, NULL);
    printf("conversion-unknown=%d\n", infoForName("fixture/Unknown") == NULL);
    mapped->javaClass = base;
    overloadMappings("fixture/Overloaded", "ClassMapConverted", (void *)conversionTwo, (void *)conversionOne);
    printf("conversion-overload=%d\n", (void *)infoForObjc([ClassMapConverted class], 1) == mapped && !strcmp(mapped->javaName, "fixture/Overloaded") && !mapped->javaClass && mapped->toObjc == (void *)conversionTwo && mapped->toJava == (void *)conversionOne);
    setTypeMapping("fixture/Value", (void *)conversionOne, (void *)conversionTwo, 16);
    type = (struct InfoView *)infoForName("fixture/Value");
    printf("type-map=%d\n", type && !type->objcClass && type->structSize == 16 && type->toObjc == (void *)conversionOne && type->toJava == (void *)conversionTwo);
    setTypeMapping("fixture/Value", NULL, NULL, 8);
    printf("type-duplicate=%d\n", type->structSize == 16 && type->toObjc == (void *)conversionOne);
    overloadMappings("fixture/Value", "ClassMapConverted", NULL, NULL);
    printf("type-overload-rejected=%d\n", !strcmp(mapped->javaName, "fixture/Overloaded") && mapped->toObjc == (void *)conversionTwo);
    [pool release];
    return 0;
}
