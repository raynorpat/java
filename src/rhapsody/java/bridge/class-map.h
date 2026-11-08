#ifndef JDK115_BRIDGE_CLASS_MAP_H
#define JDK115_BRIDGE_CLASS_MAP_H
#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include "oobj.h"

typedef struct {
    SEL selector;
    const char *signature;
} BridgeConstructor;

/* The 32-byte DR2 JavaInfo layout, including the metaclass cache ABI. */
typedef struct {
    void *toJava;
    void *toObjc;
    const char *javaName;
    ClassClass *javaClass;
    Class objcClass;
    int constructorCount;
    BridgeConstructor *constructors;
    unsigned structSize;
} BridgeJavaInfo;

void bridgeInitializeClassMap(void);
BridgeJavaInfo *CreateJavaInfo(const char *name, Class objcClass);
BridgeJavaInfo *_BRIDGEJavaInfoForJavaClassName(const char *name);
BridgeJavaInfo *_BRIDGEJavaInfoForJavaClass(ClassClass *clazz, BOOL exact);
BridgeJavaInfo *_BRIDGEJavaInfoForObjcClass(Class clazz, BOOL exact);
BridgeJavaInfo *NSSetJavaClassForObjcClass(const char *name, Class clazz);
ClassClass *NSJavaLookupClassName(const char *name);
ClassClass *NSObjcClassToJavaClass(Class clazz);
void NSRegisterJavaConstructors(Class clazz, int count, BridgeConstructor *entries);
const char *NSGetJavaConstructorSignature(Class clazz, SEL selector);
SEL NSSelectorForJavaConstructor(Class clazz, const char *signature);
#endif
