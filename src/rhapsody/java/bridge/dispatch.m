#import "dispatch.h"
#import "bridge-exceptions.h"
#import "selectors.h"
#import "forwarding.h"
#include "invocation.h"
#include <stdlib.h>
#include <string.h>

int MorphObjcArgumentsForJava(const char *signature, va_list args)
{
    const char *p = signature;
    char *input = (char *)args, *output = (char *)args;
    if (*p++ != '(')
        [NSException raise:NSInvalidArgumentException format:@"*** invalid method signature %s", signature];
    while (*p && *p != ')') {
        char type = *p++;
        unsigned size = 4;
        if (type == 'J' || type == 'D') size = 8;
        else if (type == '[')
            [NSException raise:NSInvalidArgumentException format:@"*** can't convert argument to Java array"];
        else if (type == 'L') {
            char name[512];
            unsigned length = 0;
            BridgeJavaInfo *info;
            HObject *handle;
            int consumed = 4;
            while (*p && *p != ';') {
                if (length == sizeof(name)-1)
                    [NSException raise:NSInternalInconsistencyException format:@"NSInvokeJavaMethod(): class name buffer overflow"];
                name[length++] = *p++;
            }
            name[length] = 0;
            if (*p) ++p;
            info = _BRIDGEJavaInfoForJavaClassName(name);
            if (info && !info->objcClass)
                handle = ((HObject *(*)(void *, int *))info->toJava)(input, &consumed);
            else handle = NSIdToJavaHandle(*(id *)input);
            *(HObject **)output = handle;
            input += consumed; output += 4;
            continue;
        } else if (!strchr("BCFISZ", type))
            [NSException raise:NSInvalidArgumentException format:@"*** unknown Java type '%c'", type];
        /* The donor advances primitive slots without moving them after structs. */
        input += size; output += size;
    }
    return output - (char *)args;
}

static struct methodblock *methodForSelector(ClassClass *clazz, SEL selector, BOOL isStatic)
{
    const char *name, *signature;
    if (!NSGetJavaMethodForSelector(selector, &name, &signature)) return NULL;
    return _JAVAMethodLookup(EE(), clazz, name, signature, !isStatic, isStatic);
}
BOOL _NSJavaClassRespondsToSelector(ClassClass *clazz, Class nativeClass, SEL selector)
{
    return selector && clazz && methodForSelector(clazz, selector, YES) != NULL;
}
BOOL _NSJavaObjectRespondsToSelector(HObject *object, id nativeObject, SEL selector)
{
    struct objc_super super;
    if (!selector) return NO;
    if (object && methodForSelector(obj_classblock(object), selector, NO)) return YES;
    if (!nativeObject) return NO;
    super.receiver = nativeObject;
    super.class = nativeObject->isa;
    while (super.class && (super.class->info & 0x200)) super.class = super.class->super_class;
    return ((BOOL (*)(struct objc_super *, SEL, SEL))objc_msgSendSuper)(&super, @selector(respondsToSelector:), selector);
}
BridgeResult bridgeInvokeJava(HObject *target, SEL selector, BOOL isStatic, va_list args)
{
    BridgeResult result;
    ClassClass *clazz = isStatic ? (ClassClass *)target : obj_classblock(target);
    struct methodblock *method = methodForSelector(clazz, selector, isStatic);
    if (!method)
        [NSException raise:NSInvalidArgumentException format:@"*** java class %s doesn't implement %s", cbName(clazz), sel_getName(selector)];
    MorphObjcArgumentsForJava(fieldsig(&method->fb), args);
    result.second = 0;
    result.first = __JAVAMethodInvoke(EE(), target, method, isStatic, TRUE, args, &result.second);
    return result;
}

static BridgeResult invokeReceiver(id receiver, SEL selector, va_list args)
{
    BOOL isStatic = (receiver->isa->info & CLS_META) != 0;
    HObject *target = isStatic ? (HObject *)NSObjcClassToJavaClass((Class)receiver) : NSIdToJavaHandle(receiver);
    return bridgeInvokeJava(target, selector, isStatic, args);
}
static long wordImp(id receiver, SEL selector, ...)
{
    BridgeResult result; va_list args;
    va_start(args, selector); result = invokeReceiver(receiver, selector, args); va_end(args);
    return result.first;
}
static long long wideImp(id receiver, SEL selector, ...)
{
    BridgeResult result; va_list args;
    va_start(args, selector); result = invokeReceiver(receiver, selector, args); va_end(args);
    /* The donor's generated i386 long-return path loses the high word. */
    return (unsigned long)result.second;
}
static float floatImp(id receiver, SEL selector, ...)
{
    BridgeResult result; va_list args;
    union { long bits; float value; } value;
    va_start(args, selector); result = invokeReceiver(receiver, selector, args); va_end(args);
    value.bits = result.first;
    return value.value;
}
static double doubleImp(id receiver, SEL selector, ...)
{
    BridgeResult result; va_list args;
    union { unsigned long words[2]; double value; } value;
    va_start(args, selector); result = invokeReceiver(receiver, selector, args); va_end(args);
    value.words[0] = result.second; value.words[1] = result.first;
    return value.value;
}
/* Object/type conversions may return either one or two register words. */
static long long objectImp(id receiver, SEL selector, ...)
{
    NSObjCValue result;
    void *destination;
    BOOL isStatic;
    HObject *target;
    va_list args;
    __asm__ volatile ("movl %%ebx,%0" : "=r" (destination));
    isStatic = (receiver->isa->info & CLS_META) != 0;
    target = isStatic ? (HObject *)NSObjcClassToJavaClass((Class)receiver) : NSIdToJavaHandle(receiver);
    memset(&result, 0, sizeof(result));
    result.value.structLocation = destination;
    va_start(args, selector);
    bridgeForwardJava(target, selector, isStatic, args, &result);
    va_end(args);
    return result.value.longlongValue;
}
static id initImp(id receiver, SEL selector, ...)
{
    ClassClass *clazz = NSObjcClassToJavaClass(receiver->isa);
    const char *signature = NSGetJavaConstructorSignature(receiver->isa, selector);
    va_list args;
    id object;
    va_start(args, selector);
    NSDeallocateObject(receiver);
    object = [_NSCreateJavaObjectV(clazz, signature, YES, args) retain];
    va_end(args);
    return object;
}
IMP bridgeImplementationForMethod(struct methodblock *method, SEL selector)
{
    const char *result;
    if (!strcmp(fieldname(&method->fb), "<init>")) return (IMP)initImp;
    result = strchr(fieldsig(&method->fb), ')') + 1;
    switch (*result) {
    case 'J': return (IMP)wideImp;
    case 'F': return (IMP)floatImp;
    case 'D': return (IMP)doubleImp;
    case 'L': return (IMP)objectImp;
    default: return (IMP)wordImp;
    }
}
