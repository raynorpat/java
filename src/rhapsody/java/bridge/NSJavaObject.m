#import "objects.h"
#import "object-map.h"
#import "bridge-exceptions.h"
#import "dispatch.h"
#import "selectors.h"
#import "key-value.h"
#import "coding.h"
#import "forwarding.h"
#include "invocation.h"
#include "signature.h"
#include "javaString.h"
#include <stdlib.h>
#include <string.h>
extern id _JavaStringToNSString(Hjava_lang_String *);
@interface NSMethodSignature (BridgePrivate)
+ (id)signatureWithObjCTypes:(const char *)types;
@end

@implementation NSJavaObject
+ (void)initialize
{
    if (self == [NSJavaObject class]) ((Class)self)->info |= 0x400;
}
+ (id)objectWithJavaHandle:(HObject *)handle { return NSJavaHandleToId(handle); }
+ (id)newWithSignature:(NSString *)signature arguments:(va_list)args
{
    return _NSCreateJavaObjectV(NSObjcClassToJavaClass(self), [signature cString], NO, args);
}
+ (id)newWithSignature:(NSString *)signature, ...
{
    va_list args;
    id object;
    va_start(args, signature);
    object = [self newWithSignature:signature arguments:args];
    va_end(args);
    return object;
}
- (ClassClass *)javaClass { return NSObjcClassToJavaClass(isa); }
- (HObject *)javaHandle { return _javaHandle; }
- init
{
    return [self initWithJavaHandle:JAVANewObject(EE(), [self javaClass], "()", FALSE, NULL)];
}
- initWithJavaHandle:(HObject *)handle
{
    if (!handle) { [super dealloc]; return nil; }
    _javaHandle = NSRetainJavaObject(handle);
    NSJavaMapObjects(_javaHandle, self, YES);
    return self;
}
- (void)dealloc
{
    NSReleaseJavaObject(_javaHandle);
    _javaHandle = NULL;
    [super dealloc];
}
- (void)release
{
    if ([self retainCount] == 2) {
        [super release];
        NSJavaRemoveMapping(_javaHandle, self, YES);
    } else [super release];
}
- (NSString *)description
{
    ClassClass *clazz;
    struct methodblock *method;
    Hjava_lang_String *string;
    if (!_javaHandle) return [super description];
    clazz = obj_classblock(_javaHandle);
    method = _JAVAMethodLookup(EE(), clazz, "toString", "()Ljava/lang/String;", TRUE, FALSE);
    string = (Hjava_lang_String *)__JAVAMethodInvoke(EE(), _javaHandle, method, FALSE, FALSE, NULL, NULL);
    return [NSString stringWithFormat:@"<%s: %@>", cbName(clazz), _JavaStringToNSString(string)];
}
+ (BOOL)respondsToSelector:(SEL)selector
{
    return _NSJavaClassRespondsToSelector(NSObjcClassToJavaClass(self), Nil, selector) || class_respondsToMethod(self, selector);
}
- (BOOL)respondsToSelector:(SEL)selector
{
    return _NSJavaObjectRespondsToSelector(_javaHandle, nil, selector) || class_respondsToMethod(isa, selector);
}
- (BOOL)respondsTo:(SEL)selector { return _NSJavaObjectRespondsToSelector(_javaHandle, nil, selector) || class_respondsToMethod(isa, selector); }
- (BOOL)conformsToProtocol:(Protocol *)protocol { return NO; }
+ (long long)forward:(SEL)selector :(void *)frame
{
    NSObjCValue result;
    void *destination;
    char type;
    __asm__ volatile ("movl %%ebx,%0" : "=r" (destination));
    memset(&result, 0, sizeof(result));
    result.value.structLocation = destination;
    type = bridgeForwardJava((HObject *)NSObjcClassToJavaClass(self), selector, YES, (va_list)((char *)frame + 8), &result);
    if (type == 'F') __asm__ volatile ("flds %0" : : "m" (result.value.floatValue));
    else if (type == 'D') __asm__ volatile ("fldl %0" : : "m" (result.value.doubleValue));
    return result.value.longlongValue;
}
- (long long)forward:(SEL)selector :(void *)frame
{
    NSObjCValue result;
    void *destination;
    char type;
    __asm__ volatile ("movl %%ebx,%0" : "=r" (destination));
    if (!_javaHandle) [NSException raise:NSObjectNotAvailableException
        format:@"-[NSJavaObject forward:%s] called on uninitialized object", sel_getName(selector)];
    memset(&result, 0, sizeof(result));
    result.value.structLocation = destination;
    type = bridgeForwardJava(_javaHandle, selector, NO, (va_list)((char *)frame + 8), &result);
    if (type == 'F') __asm__ volatile ("flds %0" : : "m" (result.value.floatValue));
    else if (type == 'D') __asm__ volatile ("fldl %0" : : "m" (result.value.doubleValue));
    return result.value.longlongValue;
}
- (void)forwardInvocation:(NSInvocation *)invocation { bridgeForwardInvocation(_javaHandle, invocation); }
- (id)performSelector:(SEL)selector
{
    NSObjCValue result;
    if (class_respondsToMethod(isa, selector)) return objc_msgSend(self, selector);
    memset(&result, 0, sizeof(result));
    bridgeForwardJava(_javaHandle, selector, NO, NULL, &result);
    return result.value.objectValue;
}
- (id)performSelector:(SEL)selector withObject:(id)object
{
    NSObjCValue result;
    if (class_respondsToMethod(isa, selector)) return objc_msgSend(self, selector, object);
    memset(&result, 0, sizeof(result));
    bridgeForwardJava(_javaHandle, selector, NO, (va_list)&object, &result);
    return result.value.objectValue;
}
- (id)performSelector:(SEL)selector withObject:(id)first withObject:(id)second
{
    NSObjCValue result;
    id args[2];
    if (class_respondsToMethod(isa, selector)) return objc_msgSend(self, selector, first, second);
    args[0] = first; args[1] = second;
    memset(&result, 0, sizeof(result));
    bridgeForwardJava(_javaHandle, selector, NO, (va_list)args, &result);
    return result.value.objectValue;
}
- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    const char *name, *signature;
    struct methodblock *method;
    char *types;
    NSMethodSignature *result;
    if (!_javaHandle || !NSGetJavaMethodForSelector(selector, &name, &signature)) return nil;
    method = _JAVAMethodLookup(EE(), obj_classblock(_javaHandle), name, signature, TRUE, FALSE);
    if (!method) return nil;
    types = NSJavaSignatureToObjc(signature ? signature : fieldsig(&method->fb));
    result = [NSMethodSignature signatureWithObjCTypes:types];
    free(types);
    return result;
}
- (id)valueForKey:(NSString *)key
{
    id value;
    if (NSJavaValueForKey(_javaHandle, key, &value)) return value;
    [NSException raise:NSInvalidArgumentException format:@"[%@ valueForKey] object has no valueForKey(%@)",
        [NSString stringWithCString:cbName(obj_classblock(_javaHandle))], key];
    return nil;
}
- (void)takeValue:(id)value forKey:(NSString *)key { NSJavaTakeValueForKey(_javaHandle, value, key); }
- (id)validateValue:(id *)value forKey:(NSString *)key
{
    NSObjCValue result;
    id args[2];
    if (!_NSJavaObjectRespondsToSelector(_javaHandle, nil, _cmd)) return nil;
    args[0] = *value; args[1] = key;
    memset(&result, 0, sizeof(result));
    bridgeForwardJava(_javaHandle, _cmd, NO, (va_list)args, &result);
    if ([result.value.objectValue isKindOfClass:[NSException class]]) return result.value.objectValue;
    *value = result.value.objectValue;
    return nil;
}
- (void)encodeWithCoder:(NSCoder *)coder { NSArchiveJavaObject(_javaHandle, coder); }
- initWithCoder:(NSCoder *)coder { return [self initWithJavaHandle:NSUnarchiveJavaObject(coder)]; }
- (Class)classForCoder { return isa; }
- (Class)classForArchiver { return isa; }
- replacementObjectForCoder:(NSCoder *)coder { return self; }
- replacementObjectForArchiver:(NSArchiver *)coder { return self; }
- awakeAfterUsingCoder:(NSCoder *)coder { return self; }
- awakeAfterUsingArchiver:(NSArchiver *)coder { return self; }
- initWithKeyValueUnarchiver:(id)coder
{
    HObject *argument = NSIdToJavaHandle(coder);
    return [self initWithJavaHandle:JAVANewObject(EE(), [self javaClass],
        "(Lcom/apple/yellow/eocontrol/EOKeyValueUnarchiver;)", FALSE, (va_list)&argument)];
}
@end
