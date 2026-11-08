/* DR2 ObjCJava selector mapping, 0x53d0b2d0–0x53d0b948. */
#import "selectors.h"
#import "diagnostics.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *signature;
    SEL selector;
} SelectorMapping;

static NSHashTable *javaSelectors, *objcSelectors;

static unsigned stringHash(const char *text)
{
    unsigned hash = 0, shift = 0;
    if (text) while (*text) {
        hash ^= (unsigned)(unsigned char)*text++ << shift;
        shift = (shift + 8) & 31;
    }
    return hash;
}

static unsigned javaHash(NSHashTable *table, const void *pointer)
{
    const SelectorMapping *mapping = pointer;
    return stringHash(mapping->name) ^ stringHash(mapping->signature);
}

static BOOL javaEqual(NSHashTable *table, const void *left, const void *right)
{
    const SelectorMapping *a = left, *b = right;
    if (strcmp(a->name, b->name)) return NO;
    if (!a->signature || !b->signature) return a->signature == b->signature;
    return strcmp(a->signature, b->signature) == 0;
}

static unsigned objcHash(NSHashTable *table, const void *pointer)
{
    return (unsigned)((const SelectorMapping *)pointer)->selector;
}

static BOOL objcEqual(NSHashTable *table, const void *left, const void *right)
{
    return ((const SelectorMapping *)left)->selector ==
        ((const SelectorMapping *)right)->selector;
}

void bridgeInitializeSelectorMaps(void)
{
    NSHashTableCallBacks callbacks;
    if (javaSelectors) return;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.hash = javaHash;
    callbacks.isEqual = javaEqual;
    javaSelectors = NSCreateHashTableWithZone(callbacks, 1, NULL);
    callbacks.hash = objcHash;
    callbacks.isEqual = objcEqual;
    objcSelectors = NSCreateHashTableWithZone(callbacks, 1, NULL);
}

void NSSetOverloadedJavaMethodForObjcSelector(const char *name,
    const char *signature, SEL selector)
{
    SelectorMapping key, *mapping, *old;
    bridgeInitializeSelectorMaps();
    if (!name)
        [NSException raise:NSInternalInconsistencyException
            format:@"ObjCJava: invalid selector mapping { %s, %s, %s }",
            name, signature, sel_getName(selector)];
    key.name = name;
    key.signature = signature;
    key.selector = selector;
    old = NSHashGet(javaSelectors, &key);
    if (old) {
        if (old->selector == selector)
            _BRIDGEComplain(0, @"warning: duplicate selector mapping %s for %s",
                name, sel_getName(selector));
        else {
            if (signature)
                _BRIDGEComplain(0, @"error: attempt to map %s%s to %s; a mapping from %s%s to %s already exists.",
                    name, signature, sel_getName(selector), old->name,
                    old->signature, sel_getName(old->selector));
            else
                _BRIDGEComplain(0, @"error: attempt to map %s to %s; a mapping from %s to %s already exists.",
                    name, sel_getName(selector), old->name, sel_getName(old->selector));
            _BRIDGEComplain(0, @"       this will throw an exception soon!");
        }
    }
    old = NSHashGet(objcSelectors, &key);
    if (old && !javaEqual(NULL, old, &key)) {
        if (signature)
            _BRIDGEComplain(0, @"error: attempt to map %s%s to %s; a mapping from %s%s to %s already exists.",
                name, signature, sel_getName(selector), old->name,
                old->signature, sel_getName(old->selector));
        else
            _BRIDGEComplain(0, @"error: attempt to map %s to %s; a mapping from %s to %s already exists.",
                name, sel_getName(selector), old->name, sel_getName(old->selector));
        _BRIDGEComplain(0, @"       this will throw an exception soon!");
    }
    mapping = malloc(sizeof(*mapping));
    /* As in DR2, registered names/signatures have process lifetime. */
    mapping->name = name;
    mapping->signature = signature;
    mapping->selector = selector;
    NSHashInsert(objcSelectors, mapping);
    NSHashInsert(javaSelectors, mapping);
}

void NSSetJavaMethodForObjcSelector(const char *name, SEL selector)
{
    NSSetOverloadedJavaMethodForObjcSelector(name, NULL, selector);
}

SEL NSLookupJavaMethodName(const char *name, const char *signature)
{
    SelectorMapping key, *mapping;
    if (!javaSelectors) return NULL;
    key.name = name;
    key.signature = signature;
    mapping = NSHashGet(javaSelectors, &key);
    if (!mapping && signature) {
        key.signature = NULL;
        mapping = NSHashGet(javaSelectors, &key);
    }
    return mapping ? mapping->selector : NULL;
}

SEL NSJavaMethodNameToSelector(const char *name, const char *signature,
    Class superclass)
{
    SEL selector;
    const char *p;
    unsigned count = 0, length;
    char *buffer;
    if (!name) return NULL;
    selector = NSLookupJavaMethodName(name, signature);
    if (selector) return selector;
    while (superclass) {
        void *iterator = NULL;
        struct objc_method_list *list;
        while ((list = class_nextMethodList(superclass, &iterator)) != NULL) {
            unsigned i;
            for (i = 0; i < list->method_count; ++i) {
                const char *method = sel_getName(list->method_list[i].method_name);
                length = strcspn(method, ":");
                if (strlen(name) == length && !strncmp(name, method, length))
                    return list->method_list[i].method_name;
            }
        }
        superclass = superclass->super_class;
    }
    if (!signature || *signature != '(') return NULL;
    p = signature + 1;
    while (*p && *p != ')') {
        while (*p == '[') ++p;
        if (*p == 'L') while (*p && *p != ';') ++p;
        if (*p) ++p;
        ++count;
    }
    length = strlen(name);
    buffer = malloc(length + count + 1);
    memcpy(buffer, name, length);
    memset(buffer + length, ':', count);
    buffer[length + count] = 0;
    selector = sel_registerName(buffer);
    free(buffer);
    return selector;
}

BOOL NSGetJavaMethodForSelector(SEL selector, const char **name,
    const char **signature)
{
    SelectorMapping key, *mapping;
    if (!objcSelectors || !selector) return NO;
    key.selector = selector;
    mapping = NSHashGet(objcSelectors, &key);
    if (mapping) {
        *name = mapping->name;
        *signature = mapping->signature;
    } else {
        const char *method = sel_getName(selector);
        unsigned length = strcspn(method, ":");
        char *derived = malloc(length + 1);
        memcpy(derived, method, length);
        derived[length] = 0;
        NSSetJavaMethodForObjcSelector(derived, selector);
        *name = derived;
        *signature = NULL;
    }
    return YES;
}
