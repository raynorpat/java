/* DR2 class registration/cache and constructor selectors, libObjCJava. */
#import "class-map.h"
#import "diagnostics.h"
#include "interpreter.h"
#include <stdlib.h>
#include <string.h>

static NSHashTable *javaClasses;
static BridgeJavaInfo **rootClasses;
static unsigned rootCount;

static unsigned classHash(NSHashTable *table, const void *pointer)
{
    const char *name = ((const BridgeJavaInfo *)pointer)->javaName;
    unsigned hash = 0, shift = 0;
    while (*name) {
        hash ^= (unsigned)(unsigned char)*name++ << shift;
        shift = (shift + 8) & 31;
    }
    return hash;
}

static BOOL classEqual(NSHashTable *table, const void *left, const void *right)
{
    return !strcmp(((const BridgeJavaInfo *)left)->javaName,
        ((const BridgeJavaInfo *)right)->javaName);
}

void bridgeInitializeClassMap(void)
{
    NSHashTableCallBacks callbacks;
    if (javaClasses) return;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.hash = classHash;
    callbacks.isEqual = classEqual;
    javaClasses = NSCreateHashTableWithZone(callbacks, 1, NULL);
}

BridgeJavaInfo *CreateJavaInfo(const char *name, Class clazz)
{
    BridgeJavaInfo *info = calloc(1, sizeof(*info));
    bridgeInitializeClassMap();
    info->javaName = name;
    info->objcClass = clazz;
    if (clazz) {
        if (clazz->super_class)
            clazz->isa->ivars = (struct objc_ivar_list *)info;
        else {
            rootClasses = realloc(rootClasses, (rootCount + 1) * sizeof(*rootClasses));
            rootClasses[rootCount++] = info;
        }
    }
    NSHashInsert(javaClasses, info);
    return info;
}

BridgeJavaInfo *_BRIDGEJavaInfoForJavaClassName(const char *name)
{
    BridgeJavaInfo key;
    if (!javaClasses) return NULL;
    key.javaName = name;
    return NSHashGet(javaClasses, &key);
}

BridgeJavaInfo *_BRIDGEJavaInfoForJavaClass(ClassClass *clazz, BOOL exact)
{
    BridgeJavaInfo *info;
    while (clazz) {
        info = _BRIDGEJavaInfoForJavaClassName(cbName(clazz));
        if (info || exact) return info;
        clazz = cbSuperclass(clazz);
    }
    return NULL;
}

BridgeJavaInfo *_BRIDGEJavaInfoForObjcClass(Class clazz, BOOL exact)
{
    while (clazz) {
        if (!clazz->super_class) {
            unsigned i;
            for (i = 0; i < rootCount; ++i)
                if (rootClasses[i]->objcClass == clazz) return rootClasses[i];
            return NULL;
        }
        if (clazz->isa->ivars) return (BridgeJavaInfo *)clazz->isa->ivars;
        if (exact) return NULL;
        clazz = clazz->super_class;
    }
    return NULL;
}

BridgeJavaInfo *NSSetJavaClassForObjcClass(const char *name, Class clazz)
{
    BridgeJavaInfo *old;
    if (!name || !clazz) {
        _BRIDGEComplain(0, @"ObjCJava: can't map Java class %s to Objective C class %s",
            name, clazz ? clazz->name : NULL);
        return NULL;
    }
    old = _BRIDGEJavaInfoForObjcClass(clazz, YES);
    if (old) {
        _BRIDGEComplain(0, @"ObjCJava: a mapping for %s to %s already exists; cannot create overloaded mapping to %s",
            clazz->name, old->javaName, name);
        return NULL;
    }
    old = _BRIDGEJavaInfoForJavaClassName(name);
    if (old) {
        _BRIDGEComplain(0, @"ObjCJava: a mapping for %s to %s already exists; cannot create overloaded mapping to %s",
            name, old->objcClass ? old->objcClass->name : "an ObjC type", clazz->name);
        return NULL;
    }
    return CreateJavaInfo(name, clazz);
}

ClassClass *NSJavaLookupClassName(const char *name)
{
    BridgeJavaInfo *info;
    if (!name) return NULL;
    info = _BRIDGEJavaInfoForJavaClassName(name);
    if (info && info->javaClass) return info->javaClass;
    return FindClass(EE(), (char *)name, TRUE);
}

ClassClass *NSObjcClassToJavaClass(Class clazz)
{
    BridgeJavaInfo *info;
    if (!clazz) return NULL;
    info = _BRIDGEJavaInfoForObjcClass(clazz, NO);
    if (info) {
        if (!info->javaClass) info->javaClass = FindClass(EE(), (char *)info->javaName, TRUE);
        return info->javaClass;
    }
    _BRIDGEComplain(0, @"warning: can't find Java class for Objective C class %s.  Returning %s.",
        clazz->name, "com/apple/yellow/foundation/NSObject");
    return FindClass(EE(), "com/apple/yellow/foundation/NSObject", TRUE);
}

void NSRegisterJavaConstructors(Class clazz, int count, BridgeConstructor *entries)
{
    BridgeJavaInfo *info = _BRIDGEJavaInfoForObjcClass(clazz, YES);
    info->constructorCount = count;
    info->constructors = entries;
}

const char *NSGetJavaConstructorSignature(Class clazz, SEL selector)
{
    BridgeJavaInfo *info;
    int i;
    while (clazz->info & 0x200) clazz = clazz->super_class;
    info = _BRIDGEJavaInfoForObjcClass(clazz, YES);
    if (info) for (i = 0; i < info->constructorCount; ++i)
        if (info->constructors[i].selector == selector) return info->constructors[i].signature;
    return selector == @selector(init) ? "()" : NULL;
}

SEL NSSelectorForJavaConstructor(Class clazz, const char *signature)
{
    BridgeJavaInfo *info;
    int i;
    while (clazz->info & 0x200) clazz = clazz->super_class;
    info = _BRIDGEJavaInfoForObjcClass(clazz, YES);
    if (info) for (i = 0; i < info->constructorCount; ++i)
        if (!strncmp(signature, info->constructors[i].signature, strlen(signature) - 1))
            return info->constructors[i].selector;
    return signature[0] == '(' && signature[1] == ')' ? @selector(init) : NULL;
}
