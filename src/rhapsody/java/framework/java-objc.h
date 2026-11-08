/* Bridge entry points available without the interpreter's private headers. */
#ifndef JDK115_PUBLIC_JAVA_OBJC_H
#define JDK115_PUBLIC_JAVA_OBJC_H
#import "NSJavaObject.h"
#import "NSJavaException.h"
#import "typemap.h"

struct BridgeJavaInfo;
struct BridgeJavaInfo *NSSetJavaClassForObjcClass(const char *name, Class clazz);
JAVAHandle NSCreateJavaWrapper(id object);

struct Hjava_lang_Object *NSIdToJavaHandle(id object);
id NSJavaHandleToId(struct Hjava_lang_Object *handle);
id NSCreateJavaObject(const char *name, const char *signature, ...);
id NSCreateJavaObjectV(const char *name, const char *signature,
    BOOL shortFloats, va_list args);
struct Hjava_lang_Object *NSRetainJavaObject(struct Hjava_lang_Object *handle);
void NSReleaseJavaObject(struct Hjava_lang_Object *handle);
Class NSJavaClassToObjcClass(struct Hjava_lang_Class *clazz, BOOL create);
struct Hjava_lang_Class *NSObjcClassToJavaClass(Class clazz);
struct Hjava_lang_Class *NSJavaLookupClassName(const char *name);
void NSSetJavaMethodForObjcSelector(const char *name, SEL selector);
void NSSetOverloadedJavaMethodForObjcSelector(const char *name,
    const char *signature, SEL selector);
void NSSetJavaClassMappings(const char *name, const char *objcName,
    void *toObjc, void *toJava);
void NSSetOverloadedJavaClassMappings(const char *name, const char *objcName,
    void *toObjc, void *toJava);
void BRIDGESetJavaTypeMapping(const char *name, void *toObjc, void *toJava,
    unsigned size);
void NSSetJavaTypeMappings(const char *name, void *toObjc, void *toJava);
char *NSJavaSignatureToObjc(const char *signature);
#endif
