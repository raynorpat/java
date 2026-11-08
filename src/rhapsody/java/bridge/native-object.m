/* DR2 native wrapper creation, retain/release interlock and NativeObject stubs. */
#import "native-object.h"
#import "object-map.h"
#import "class-map.h"
#import "bridge-exceptions.h"
#import "bootstrap.h"
#import "coding.h"
#import "key-value.h"
#include "javaString.h"
#include <stdlib.h>

extern id NSJavaHandleToId(HObject *);
extern HObject *NSIdToJavaHandle(id);
extern Class NSJavaClassToObjcClass(ClassClass *, BOOL);
extern Hjava_lang_String *_NSStringToJavaString(id);
extern id _JavaStringToNSString(Hjava_lang_String *);
extern SEL _NSConvertSelFromJava(HObject *);
extern BOOL _NSJavaObjectRespondsToSelector(HObject *, id, SEL);
extern void _objc_flush_caches(Class);
static IMP rootRetain, rootRelease;
static NSMapTable *retains, *releases;

static Method ownMethod(Class clazz, SEL selector)
{
    void *iterator = NULL;
    struct objc_method_list *list;
    int i;
    while ((list = class_nextMethodList(clazz, &iterator)))
        for (i = 0; i < list->method_count; ++i)
            if (list->method_list[i].method_name == selector) return &list->method_list[i];
    return NULL;
}
static void mapHierarchy(NSMapTable *map, IMP imp, Class start, Class end)
{
    for (;;) {
        NSMapInsert(map, start, imp);
        if (start == end) break;
        start = start->super_class;
    }
}
static IMP originalImp(NSMapTable *map, Class clazz, BOOL retain)
{
    Class parent = clazz;
    IMP imp = NSMapGet(map, parent);
    if (!imp) {
        do { parent = parent->super_class; } while (parent && !(imp = NSMapGet(map, parent)));
        if (!parent || !parent->super_class)
            [NSException raise:NSInternalInconsistencyException format:retain
                ? @"ReplacedRetainImp called for a class that hasn't had its retain method replaced!"
                : @"ReplacedReleaseImp called for a class that hasn't had its release method replaced!"];
        mapHierarchy(map, imp, clazz, parent);
    }
    return imp;
}
static void updateRoot(id object, BOOL retain)
{
    HObject *handle;
    if ([object retainCount] == (retain ? 1 : 2) && (handle = NSJavaLookupId(object))) {
        if (retain) NSRetainJavaObject(handle);
        else NSReleaseJavaObject(handle);
    }
}
static id RetainImp(id object, SEL selector) { updateRoot(object, YES); return rootRetain(object, selector); }
static void ReleaseImp(id object, SEL selector) { updateRoot(object, NO); rootRelease(object, selector); }
static id ReplacedRetainImp(id object, SEL selector)
{
    IMP imp = originalImp(retains, object->isa, YES);
    updateRoot(object, YES);
    return imp(object, selector);
}
static void ReplacedReleaseImp(id object, SEL selector)
{
    IMP imp = originalImp(releases, object->isa, NO);
    updateRoot(object, NO);
    imp(object, selector);
}
static BOOL replaceMethod(Class clazz, Class highest, SEL selector, NSMapTable *map, IMP replacement, IMP fallback)
{
    Method method = ownMethod(highest, selector);
    struct objc_method_list *list;
    if (method) {
        mapHierarchy(map, method->method_imp, clazz, highest);
        method->method_imp = replacement;
        return YES;
    }
    list = calloc(1, sizeof(struct objc_method_list));
    list->method_count = 1;
    list->method_list[0].method_name = selector;
    list->method_list[0].method_types = "v0@0:0";
    list->method_list[0].method_imp = fallback;
    class_addMethods(highest, list);
    return NO;
}
void ReplaceRetainReleaseMethods(Class clazz)
{
    Class highest = clazz, parent;
    BOOL changed;
    if (clazz->info & 0x800) return;
    clazz->info |= 0x800;
    for (parent = clazz->super_class; parent; parent = parent->super_class) {
        if (parent->info & 0x800) return;
        if (!parent->super_class) break;
        highest = parent;
        highest->info |= 0x800;
    }
    changed = replaceMethod(clazz, highest, @selector(release), releases, (IMP)ReplacedReleaseImp, (IMP)ReleaseImp);
    changed |= replaceMethod(clazz, highest, @selector(retain), retains, (IMP)ReplacedRetainImp, (IMP)RetainImp);
    if (changed) _objc_flush_caches(clazz);
}
void CreateInitialization(void)
{
    rootRetain = class_getInstanceMethod([NSObject class], @selector(retain))->method_imp;
    rootRelease = class_getInstanceMethod([NSObject class], @selector(release))->method_imp;
    retains = NSCreateMapTableWithZone(NSNonOwnedPointerMapKeyCallBacks, NSNonOwnedPointerMapValueCallBacks, 1, NULL);
    releases = NSCreateMapTableWithZone(NSNonOwnedPointerMapKeyCallBacks, NSNonOwnedPointerMapValueCallBacks, 1, NULL);
}
HObject *NSCreateExplicitJavaWrapper(id object, ClassClass *clazz)
{
    long args[2];
    HObject *handle;
    args[0] = 0; args[1] = (long)object;
    handle = JAVANewObject(PRIVILEGED_EE, clazz, "(ZI)", FALSE, (va_list)args);
    if (!handle) [NSException raise:@"Failed to construct Java object" format:@"java class '%s(boolean,int)' failed to construct object", cbName(clazz)];
    ReplaceRetainReleaseMethods(object->isa);
    NSJavaMapObjects(handle, object, YES);
    if ([object retainCount] > 2) NSRetainJavaObject(handle);
    return handle;
}
HObject *NSCreateJavaWrapper(id object) { return NSCreateExplicitJavaWrapper(object, NSObjcClassToJavaClass(object->isa)); }
id _NSAllocateObjcObjectForJavaHandle(HObject *handle)
{
    return [NSJavaClassToObjcClass(obj_classblock(handle), YES) allocWithZone:NULL];
}
void _NSAssociateJavaObject(HObject *handle, id object)
{
    *(id *)unhand(handle) = object;
    if (object) NSJavaMapObjects(handle, object, YES);
}
void _NSAssociateJavaObjectNoRetain(HObject *handle, id object)
{
    *(id *)unhand(handle) = object;
    if (object) NSJavaMapObjects(handle, object, NO);
}
void _NSDisassociateJavaObjectNoRelease(HObject *handle, id object)
{
    if (object) NSJavaRemoveMapping(handle, object, NO);
    *(id *)unhand(handle) = nil;
}
static struct objc_super nativeSuper(id object)
{
    struct objc_super target;
    Class clazz = object ? object->isa : Nil;
    while (clazz && (clazz->info & 0x200)) clazz = clazz->super_class;
    target.receiver = object; target.class = clazz;
    return target;
}
id ValueForKey(id object, SEL selector, NSString *key)
{
    HObject *handle = NSJavaLookupId(object);
    id value;
    BOOL found;
    struct objc_super target;
    if (!handle) {
        NSLog(@"ObjCJava: no java object for %@", object);
        NSLog(@"ObjCJava: can't forward method %s", selector);
        return nil;
    }
    found = NSJavaValueForKey(handle, key, &value);
    bridgeRaisePendingJavaException(EE());
    if (found) return value;
    target = nativeSuper(object);
    return objc_msgSendSuper(&target, selector, key);
}
void TakeValueForKey(id object, SEL selector, id value, NSString *key)
{
    HObject *handle = NSJavaLookupId(object);
    struct objc_super target;
    if (!handle) {
        NSLog(@"ObjCJava: no java object for %@", object);
        NSLog(@"ObjCJava: can't forward method %s", selector);
        return;
    }
    if (NSJavaTakeValueForKey(handle, value, key)) bridgeRaisePendingJavaException(EE());
    else { target = nativeSuper(object); objc_msgSendSuper(&target, selector, value, key); }
}

#define NATIVE(name) stack_item *Java_com_apple_alpha_core_NativeObject_##name##_stub(stack_item *args, ExecEnv *env)
NATIVE(nukeObjcObject)
{
    id *instance = (id *)unhand(args[0].h);
    NSJavaRemoveMapping(args[0].h, *instance, NO);
    _NSJavaSafeRelease(*instance);
    *instance = nil;
    return args;
}
NATIVE(allocateObjcObject)
{
    HObject *handle = args[0].h;
    int state = _NSJavaStubEnter();
    id initialized;
    struct objc_super target;
    NS_DURING
        target = nativeSuper(_NSAllocateObjcObjectForJavaHandle(handle));
        _NSAssociateJavaObjectNoRetain(handle, target.receiver);
        initialized = objc_msgSendSuper(&target, @selector(init));
        if (initialized != target.receiver) {
            _NSDisassociateJavaObjectNoRelease(handle, target.receiver);
            if (initialized) _NSAssociateJavaObjectNoRetain(handle, initialized);
        }
        if (!initialized) SignalError(env, "java/lang/NullPointerException", NULL);
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args;
}
NATIVE(copyObjcObject)
{
    int state = _NSJavaStubEnter();
    struct objc_super target = nativeSuper(*(id *)unhand(args[0].h));
    id copy = objc_msgSendSuper(&target, args[2].i ? @selector(mutableCopyWithZone:) : @selector(copyWithZone:), NULL);
    _NSAssociateJavaObject(args[1].h, copy);
    _NSJavaStubExit(state);
    args[0].i = (long)copy;
    return args + 1;
}
NATIVE(toString)
{
    id object = *(id *)unhand(args[0].h);
    int state = _NSJavaStubEnter();
    args[0].h = (HObject *)_NSStringToJavaString(object ? [object description] : @"");
    _NSJavaStubExit(state);
    return args + 1;
}
NATIVE(hashCode)
{
    int state = _NSJavaStubEnter();
    struct objc_super target;
    NS_DURING
        target = nativeSuper(*(id *)unhand(args[0].h));
        args[0].i = (long)objc_msgSendSuper(&target, @selector(hash));
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args + 1;
}
NATIVE(equals)
{
    int state = _NSJavaStubEnter();
    struct objc_super target;
    NS_DURING
        target = nativeSuper(*(id *)unhand(args[0].h));
        args[0].i = (BOOL)(long)objc_msgSendSuper(&target, @selector(isEqual:), NSJavaHandleToId(args[1].h));
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args + 1;
}
NATIVE(respondsToMethod)
{
    args[0].i = _NSJavaObjectRespondsToSelector(args[0].h, *(id *)unhand(args[0].h), _NSConvertSelFromJava(args[1].h));
    return args + 1;
}
NATIVE(performMethodWithObjects)
{
    int state = _NSJavaStubEnter();
    NS_DURING
        args[0].h = NSIdToJavaHandle(objc_msgSend(*(id *)unhand(args[0].h),
            _NSConvertSelFromJava(args[1].h), NSJavaHandleToId(args[2].h), NSJavaHandleToId(args[3].h)));
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args + 1;
}
NATIVE(valueForKey)
{
    id object = *(id *)unhand(args[0].h);
    int state = _NSJavaStubEnter();
    NSString *key = _JavaStringToNSString((Hjava_lang_String *)args[1].h);
    NS_DURING
        args[0].h = NSIdToJavaHandle(ValueForKey(object, @selector(valueForKey:), key));
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args + 1;
}
NATIVE(takeValueForKey)
{
    id value = NSJavaHandleToId(args[1].h), object = *(id *)unhand(args[0].h);
    int state = _NSJavaStubEnter();
    NSString *key = _JavaStringToNSString((Hjava_lang_String *)args[2].h);
    NS_DURING
        TakeValueForKey(object, @selector(takeValue:forKey:), value, key);
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    _NSJavaStubExit(state);
    return args;
}
NATIVE(serializeObjcObject)
{
    NS_DURING
        NSSerializeObjCObject(*(id *)unhand(args[0].h), args[1].h);
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    return args;
}
NATIVE(deserializeObjcObject)
{
    id object;
    HObject *exception, *message;
    ClassClass *clazz;
    NS_DURING
        object = NSDeserializeObjCObject(args[1].h);
        if (object) _NSAssociateJavaObjectNoRetain(args[0].h, object);
        else {
            clazz = FindClass(env, "java/io/IOException", TRUE);
            message = (HObject *)_NSStringToJavaString(@"class instantiation error in readObject()");
            exception = JAVANewObject(env, clazz, "(Ljava/lang/String;)", FALSE, (va_list)&message);
            exceptionThrow(env, exception);
        }
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    return args;
}
