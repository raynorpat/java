/* DR2 CreateObjCClassForJavaClass method tables, 0x53d03a08. */
#import "classgen.h"
#import "selectors.h"
#include "signature.h"
#include "tree.h"
#include "interpreter.h"
#include <stdlib.h>
#include <string.h>

static struct objc_method_list *methodList(unsigned count)
{
    return calloc(1, sizeof(struct objc_method_list) + count * sizeof(struct objc_method));
}

static void installMethods(Class clazz, struct objc_method_list *list)
{
    clazz->methodLists = malloc(2 * sizeof(*clazz->methodLists));
    if (list->method_count) {
        clazz->methodLists[0] = list;
        clazz->methodLists[1] = (struct objc_method_list *)-1;
    } else {
        free(list);
        clazz->methodLists[0] = (struct objc_method_list *)-1;
    }
}

Class bridgeCreateNamedJavaClassWithOverrides(const char *name, ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation, const struct objc_method *overrides,
    unsigned overrideCount)
{
    Class clazz, meta;
    struct objc_method_list *instanceMethods, *classMethods;
    unsigned i, count = cbMethodsCount(javaClass);
    BridgeJavaInfo *info;
    clazz = calloc(2, sizeof(struct objc_class));
    meta = clazz + 1;
    clazz->name = meta->name = strdup(name);
    clazz->isa = meta;
    clazz->super_class = superclass;
    meta->isa = objc_getMetaClass("NSObject");
    meta->super_class = superclass->isa;
    clazz->version = 1;
    meta->version = 5;
    clazz->instance_size = superclass->instance_size;
    meta->instance_size = sizeof(struct objc_class);
    instanceMethods = methodList(count + overrideCount);
    classMethods = methodList(count);
    for (i = 0; i < count; ++i) {
        struct methodblock *method = &cbMethods(javaClass)[i];
        const char *name = fieldname(&method->fb);
        SEL selector;
        char *types;
        IMP imp;
        struct objc_method_list *list;
        struct objc_method *entry;
        if (!strcmp(name, "<clinit>") || strchr(name, '$')) continue;
        if (!strcmp(name, "<init>")) {
            Method inherited;
            selector = NSSelectorForJavaConstructor(superclass, fieldsig(&method->fb));
            if (!selector) continue;
            inherited = class_getInstanceMethod(superclass, selector);
            if (!inherited)
                [NSException raise:NSInternalInconsistencyException
                    format:@"Missing Objective-C constructor method %s", sel_getName(selector)];
            types = strdup(inherited->method_types);
        } else {
            selector = NSJavaMethodNameToSelector(name, fieldsig(&method->fb), superclass);
            types = NSJavaSignatureToObjc(fieldsig(&method->fb));
        }
        imp = implementation(method, selector);
        if (!imp)
            [NSException raise:NSInternalInconsistencyException
                format:@"Missing bridge implementation for %s", name];
        list = method->fb.access & ACC_STATIC ? classMethods : instanceMethods;
        entry = &list->method_list[list->method_count++];
        entry->method_name = selector;
        entry->method_types = types;
        entry->method_imp = imp;
    }
    for (i = 0; i < overrideCount; ++i)
        instanceMethods->method_list[instanceMethods->method_count++] = overrides[i];
    installMethods(clazz, instanceMethods);
    installMethods(meta, classMethods);
    objc_addClass(clazz);
    clazz->info |= superclass->info & 0x400 ? 0x400 : 0x200;
    meta->info |= (superclass->info & 0x400 ? 0x400 : 0x200) | CLS_META;
    info = NSSetJavaClassForObjcClass(cbName(javaClass), clazz);
    if (info) info->javaClass = javaClass;
    return clazz;
}

Class bridgeCreatePureJavaClass(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation)
{
    if (!(superclass->info & 0x400))
        [NSException raise:NSInvalidArgumentException
            format:@"Pure Java class generation requires a Java proxy superclass"];
    return bridgeCreateJavaClassWithOverrides(javaClass, superclass, implementation, NULL, 0);
}

Class bridgeCreateJavaClassWithOverrides(ClassClass *javaClass, Class superclass,
    BridgeMethodImplementation implementation, const struct objc_method *overrides,
    unsigned overrideCount)
{
    return bridgeCreateNamedJavaClassWithOverrides(cbName(javaClass), javaClass,
        superclass, implementation, overrides, overrideCount);
}
