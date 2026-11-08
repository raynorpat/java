#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DONOR
#import <JavaVM/JavaVM.h>
#else
#import "classgen.h"
#import "class-map.h"
#import "constructor.h"
#import "invocation.h"
#import "vm.h"
#include "tree.h"

/* A real Java-backed test superclass supplies dispatch to the class builder.
 * The production NSJavaObject/lifetime layer is tested separately when added. */
@interface ClassGenBase : NSObject { @public HObject *handle; }
@end
@implementation ClassGenBase
- init
{
    self = [super init];
    handle = bridgeNewJavaObject(EE(), NSObjcClassToJavaClass([self class]), "()", FALSE, NULL);
    return self;
}
@end

static long invoke(id receiver, SEL selector, BOOL isStatic)
{
    ClassClass *clazz = NSObjcClassToJavaClass(isStatic ? (Class)receiver : [receiver class]);
    struct methodblock *method = _JAVAMethodLookup(EE(), clazz, sel_getName(selector), NULL, !isStatic, isStatic);
    HObject *exception;
    long high;
    if (!method) { fprintf(stderr, "Missing generated method\n"); exit(1); }
    return __JAVAMethodInvokeNoExceptionConversion(EE(),
        isStatic ? (HObject *)clazz : ((ClassGenBase *)receiver)->handle,
        method, isStatic, TRUE, NULL, &high, &exception);
}
static int instanceInt(id self, SEL selector) { return invoke(self, selector, NO); }
static int staticInt(id self, SEL selector) { return invoke(self, selector, YES); }
static float staticFloat(id self, SEL selector)
{
    union { long bits; float value; } result;
    result.bits = invoke(self, selector, YES);
    return result.value;
}
static IMP implementation(struct methodblock *method, SEL selector)
{
    if (!strcmp(fieldname(&method->fb), "<init>"))
        return class_getInstanceMethod([ClassGenBase class], @selector(init))->method_imp;
    if (method->fb.access & ACC_STATIC) {
        if (strchr(fieldsig(&method->fb), 'F')) return (IMP)staticFloat;
        return (IMP)staticInt;
    }
    return (IMP)instanceInt;
}
#endif

@interface NSObject (GeneratedFixture)
- (int)value;
+ (int)answer;
+ (float)ratio;
@end

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    Class generated;
    id object;
    Method method;
#ifdef DONOR
    NSJavaVirtualMachine *vm;
#else
    ClassClass *clazz;
#endif
    if (argc != 2) return 2;
#ifdef DONOR
    vm = [[[NSJavaVirtualMachine alloc]
        initWithClassPath:[NSString stringWithCString:argv[1]]] autorelease];
    generated = [vm findClass:@"ClassGenFixture"];
#else
    if (!JAVALoadVirtualMachine([NSString stringWithCString:argv[1]], nil)) return 1;
    clazz = NSJavaLookupClassName("ClassGenFixture");
    if (!clazz) return 1;
    ((Class)[ClassGenBase class])->info |= 0x400;
    generated = bridgeCreatePureJavaClass(clazz, [ClassGenBase class], implementation);
#endif
    if (!generated) return 1;
    printf("name=%s\n", generated->name);
    printf("registered=%d\n", objc_lookUpClass("ClassGenFixture") == generated);
    printf("class-version=%ld\n", generated->version);
    printf("meta-version=%ld\n", generated->isa->version);
    printf("java-flag=%d\n", (generated->info & 0x400) != 0);
    printf("instance-layout=%d\n", generated->instance_size == generated->super_class->instance_size);
    method = class_getInstanceMethod(generated, @selector(value));
    printf("value-types=%s\n", method ? method->method_types : "<nil>");
    printf("private-method=%d\n", class_getInstanceMethod(generated, @selector(hidden)) != NULL);
    printf("dollar-method=%d\n", class_getInstanceMethod(generated, sel_registerName("dollar$method")) == NULL);
    object = [[generated alloc] init];
    printf("instance-call=%d\n", [object value]);
    printf("static-call=%d\n", [generated answer]);
    printf("float-call=%.2f\n", [generated ratio]);
    [object release];
    [pool release];
    return 0;
}
