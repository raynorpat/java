/* DR2 generated native-subclass lifetime methods, 0x53d033d4-0x53d0351c. */
#import "hybrid-dispatch.h"
#import "classgen.h"
#import "native-object.h"
#import "object-map.h"
#import "bridge-exceptions.h"
#include "invocation.h"
extern id NSJavaHandleToId(HObject *);

static struct objc_super nativeSuper(id object)
{
    struct objc_super target;
    target.receiver = object;
    target.class = object->isa;
    while (target.class->info & 0x200)
        target.class = target.class->super_class;
    return target;
}

id JavaCopyImp(id object, SEL selector)
{
    HObject *handle = NSJavaLookupId(object);
    struct methodblock *method;
    if (!handle) {
        NSLog(@"ObjCJava: no java object for %@", object);
        NSLog(@"ObjCJava: can't forward method %s", sel_getName(selector));
        return nil;
    }
    method = _JAVAMethodLookup(EE(), obj_classblock(handle), "clone",
        "()Ljava/lang/Object;", TRUE, FALSE);
    return NSJavaHandleToId((HObject *)__JAVAMethodInvoke(EE(), handle,
        method, FALSE, FALSE, NULL, NULL));
}

id JavaRetainImp(id object, SEL selector)
{
    struct objc_super target = nativeSuper(object);
    HObject *handle = NSJavaLookupId(object);
    if (handle) NSRetainJavaObject(handle);
    return objc_msgSendSuper(&target, selector);
}

void JavaReleaseImp(id object, SEL selector)
{
    struct objc_super target = nativeSuper(object);
    if ([object retainCount] == 2) NSReleaseJavaObject(NSJavaLookupId(object));
    objc_msgSendSuper(&target, selector);
}

Class bridgeCreateNamedJavaClass(const char *name, ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation)
{
    struct objc_method overrides[6];
    const char *names[6] = { "copy", "retain", "release", "copyWithZone:",
        "valueForKey:", "takeValue:forKey:" };
    const char *types[6] = { "@0@0:0", "@0@0:0", "v0@0:0", "@0@0:0i0",
        "@0@0:0@0", "v0@0:0@0@0" };
    IMP implementations[6];
    unsigned i, count = superclass->info & (0x400 | 0x200) ? 0 : 6;
    implementations[0] = implementations[3] = (IMP)JavaCopyImp;
    implementations[1] = (IMP)JavaRetainImp;
    implementations[2] = (IMP)JavaReleaseImp;
    implementations[4] = (IMP)ValueForKey;
    implementations[5] = (IMP)TakeValueForKey;
    for (i = 0; i < count; ++i) {
        overrides[i].method_name = sel_registerName(names[i]);
        overrides[i].method_types = (char *)types[i];
        overrides[i].method_imp = implementations[i];
    }
    return bridgeCreateNamedJavaClassWithOverrides(name, javaClass, superclass,
        implementation, overrides, count);
}


Class bridgeCreateJavaClass(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation)
{
    return bridgeCreateNamedJavaClass(cbName(javaClass), javaClass, superclass, implementation);
}
