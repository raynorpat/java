#import "forwarding.h"
#import "dispatch.h"
#import "selectors.h"
#include "invocation.h"
#include <stdlib.h>
#include <string.h>

void *_BRIDGEStructReturnAddress(void)
{
    void *destination;
    __asm__ volatile ("movl %%ebx,%0" : "=r" (destination));
    return destination;
}

HObject *_BRIDGEConvertObjcArgToJava(const char *name, void *argument, int *consumed)
{
    BridgeJavaInfo *info = _BRIDGEJavaInfoForJavaClassName(name);
    if (info && !info->objcClass)
        return ((HObject *(*)(void *, int *))info->toJava)(argument, consumed);
    *consumed = 4;
    return NSIdToJavaHandle(*(id *)argument);
}

long long _NSInvokeJavaMethod(HObject *target, SEL selector, va_list args, void *destination)
{
    NSObjCValue result;
    char type;
    memset(&result, 0, sizeof(result));
    result.value.structLocation = destination;
    type = bridgeForwardJava(target, selector, NO, args, &result);
    if (type == 'F') __asm__ volatile ("flds %0" : : "m" (result.value.floatValue));
    else if (type == 'D') __asm__ volatile ("fldl %0" : : "m" (result.value.doubleValue));
    return result.value.longlongValue;
}

long long _NSInvokeStaticJavaMethod(ClassClass *target, SEL selector, va_list args, void *destination)
{
    NSObjCValue result;
    char type;
    memset(&result, 0, sizeof(result));
    result.value.structLocation = destination;
    type = bridgeForwardJava((HObject *)target, selector, YES, args, &result);
    if (type == 'F') __asm__ volatile ("flds %0" : : "m" (result.value.floatValue));
    else if (type == 'D') __asm__ volatile ("fldl %0" : : "m" (result.value.doubleValue));
    return result.value.longlongValue;
}

void _BRIDGEConvertJavaRetvalToObjc(HObject *handle, const char *name, NSObjCValue *value)
{
    BridgeJavaInfo *info = _BRIDGEJavaInfoForJavaClassName(name);
    if (!info || info->objcClass) value->value.objectValue = NSJavaHandleToId(handle);
    else {
        if (!info->toObjc)
            [NSException raise:NSInternalInconsistencyException format:@"no conversion function for Java type %s.", name];
        if (info->structSize == 0 || info->structSize == 4)
            value->value.longValue = ((long (*)(HObject *))info->toObjc)(handle);
        else if (info->structSize == 8)
            value->value.longlongValue = ((long long (*)(HObject *))info->toObjc)(handle);
        else {
            if (!value->value.structLocation)
                [NSException raise:NSInternalInconsistencyException format:@"no struct return address; can't morph type %s.", info->javaName];
            /* DR2 i386 aggregate-return functions receive their destination in EBX. */
            __asm__ volatile ("pushl %1\n\tcall *%0\n\taddl $4, %%esp"
                : : "S" (info->toObjc), "D" (handle), "b" (value->value.structLocation)
                : "eax", "ecx", "edx", "memory");
        }
    }
}

char bridgeForwardJava(HObject *target, SEL selector, BOOL isStatic, va_list args, NSObjCValue *value)
{
    const char *name = NULL, *signature = NULL, *resultType;
    struct methodblock *method = NULL;
    ClassClass *clazz = isStatic ? (ClassClass *)target : obj_classblock(target);
    BridgeResult result;
    char type;
    if (NSGetJavaMethodForSelector(selector, &name, &signature))
        method = _JAVAMethodLookup(EE(), clazz, name, signature, !isStatic, isStatic);
    /* bridgeInvokeJava owns the donor's missing-method exception. */
    result = bridgeInvokeJava(target, selector, isStatic, args);
    resultType = strchr(fieldsig(&method->fb), ')') + 1;
    type = *resultType;
    switch (type) {
    case 'L': {
        char className[512];
        unsigned length = 0;
        ++resultType;
        while (*resultType && *resultType != ';') {
            if (length == sizeof(className)-1)
                [NSException raise:NSInternalInconsistencyException format:@"NSInvokeJavaMethod(): class name buffer overflow"];
            className[length++] = *resultType++;
        }
        className[length] = 0;
        _BRIDGEConvertJavaRetvalToObjc((HObject *)result.first, className, value);
        break;
    }
    case 'D':
        ((long *)&value->value)[0] = result.second;
        ((long *)&value->value)[1] = result.first;
        break;
    case 'J':
        /* Preserve the donor i386 forwarding path's truncated high word. */
        value->value.longlongValue = (unsigned long)result.second;
        break;
    default:
        value->value.longlongValue = (unsigned long)result.first;
        break;
    }
    return type;
}

void bridgeForwardInvocation(HObject *target, NSInvocation *invocation)
{
    NSMethodSignature *signature = [invocation methodSignature];
    unsigned count = [signature numberOfArguments], i, size, alignment, offset = 0;
    unsigned returnSize = [signature methodReturnLength];
    char *args = malloc([signature frameLength]), *storage = NULL;
    NSObjCValue value;
    memset(&value, 0, sizeof(value));
    if (*[signature methodReturnType] == '{' && returnSize > 8) {
        storage = malloc(returnSize);
        value.value.structLocation = storage;
    }
    NS_DURING
        for (i = 2; i < count; ++i) {
            NSGetSizeAndAlignment([signature getArgumentTypeAtIndex:i], &size, &alignment);
            [invocation getArgument:args + offset atIndex:i];
            offset += (size + 3) & ~3;
        }
        bridgeForwardJava(target, [invocation selector], NO, (va_list)args, &value);
        /* The donor does not copy register-sized structs back to NSInvocation. */
        if (returnSize && (*[signature methodReturnType] != '{' || storage))
            [invocation setReturnValue:storage ? storage : (void *)&value.value];
    NS_HANDLER
        free(storage); free(args);
        [localException raise];
    NS_ENDHANDLER
    free(storage); free(args);
}
