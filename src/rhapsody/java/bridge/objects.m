#import "objects.h"
#import "object-map.h"
#import "classgen.h"
#import "dispatch.h"
#import "bridge-exceptions.h"
#include "javaString.h"

extern HObject *NSCreateJavaWrapper(id);
extern Hjava_lang_String *_NSStringToJavaString(id);
extern id _JavaStringToNSString(Hjava_lang_String *);
ClassClass *JavaLangClassClass;
Class NSJavaObjectIsa;

void MappingInitialization(void)
{
    static BOOL initialized;
    if (initialized) return;
    initialized = YES;
    JavaLangClassClass = FindClass(EE(), "java/lang/Class", TRUE);
    NSJavaObjectIsa = [NSJavaObject class];
    bridgeInitializeClassMap();
    bridgeInitializeObjectMap();
    NSSetJavaClassForObjcClass("java/lang/Object", [NSJavaObject class]);
    NSSetJavaClassForObjcClass("com/apple/yellow/foundation/NSObject", [NSObject class]);
    CreateJavaInfo("com/apple/alpha/core/NativeObject", [NSObject class]);
}

Class NSJavaClassToObjcClass(ClassClass *clazz, BOOL create)
{
    BridgeJavaInfo *info;
    Class superclass;
    if (!clazz) return Nil;
    info = _BRIDGEJavaInfoForJavaClass(clazz, YES);
    if (info && info->objcClass) return info->objcClass;
    if (!create) return Nil;
    superclass = NSJavaClassToObjcClass(cbSuperclass(clazz), YES);
    return bridgeCreateJavaClass(clazz, superclass, bridgeImplementationForMethod);
}

HObject *NSIdToJavaHandle(id object)
{
    HObject *handle;
    BridgeJavaInfo *info;
    Class clazz;
    if (!object) return NULL;
    clazz = object->isa;
    if (clazz->info & CLS_META) return (HObject *)NSObjcClassToJavaClass((Class)object);
    if (clazz->info & 0x400) {
        handle = [(NSJavaObject *)object javaHandle];
        if (handle) return handle;
    }
    handle = NSJavaLookupId(object);
    if (handle) return handle;
    info = _BRIDGEJavaInfoForObjcClass(clazz, NO);
    if (info && info->toJava) return ((HObject *(*)(id))info->toJava)(object);
    return NSCreateJavaWrapper(object);
}

id NSJavaHandleToId(HObject *handle)
{
    id object;
    ClassClass *clazz;
    BridgeJavaInfo *info;
    if (!handle) return nil;
    object = NSJavaLookupHandle(handle);
    if (object) return object;
    clazz = obj_classblock(handle);
    if (clazz == classJavaLangClass) return NSJavaClassToObjcClass((ClassClass *)handle, YES);
    info = _BRIDGEJavaInfoForJavaClass(clazz, NO);
    if (info && info->toObjc) return ((id (*)(HObject *))info->toObjc)(handle);
    return [[NSJavaClassToObjcClass(clazz, YES) allocWithZone:NULL] initWithJavaHandle:handle];
}

Class _NSConvertClassFromJava(ClassClass *clazz)
{
    return NSJavaClassToObjcClass(clazz, YES);
}

ClassClass *_NSConvertClassToJava(Class clazz)
{
    return NSObjcClassToJavaClass(clazz);
}

id _NSCreateJavaObjectV(ClassClass *clazz, const char *signature, BOOL shortFloats, va_list args)
{
    HObject *handle;
    if (signature[1] != ')') MorphObjcArgumentsForJava(signature, args);
    handle = JAVANewObject(EE(), clazz, signature, shortFloats, args);
    if (!handle)
        [NSException raise:@"Failed to construct Java object" format:@"java class '%s' signature '%s' failed to construct object", cbName(clazz), signature];
    return NSJavaHandleToId(handle);
}
id NSCreateJavaObjectV(const char *name, const char *signature, BOOL shortFloats, va_list args)
{
    ClassClass *clazz = NSJavaLookupClassName(name);
    bridgeRaisePendingJavaException(EE());
    return _NSCreateJavaObjectV(clazz, signature ? signature : "()", shortFloats, args);
}
id NSCreateJavaObject(const char *name, const char *signature, ...)
{
    id object;
    va_list args;
    va_start(args, signature);
    object = NSCreateJavaObjectV(name, signature, NO, args);
    va_end(args);
    return object;
}

/* DR2 retained both public and private class-conversion spellings. */
Class BRIDGEConvertClassFromJava(ClassClass *clazz) { return NSJavaClassToObjcClass(clazz, YES); }
ClassClass *BRIDGEConvertClassToJava(Class clazz) { return NSObjcClassToJavaClass(clazz); }
void ClassInitialization(void) { }
