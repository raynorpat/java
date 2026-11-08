#ifndef JDK115_BRIDGE_DISPATCH_H
#define JDK115_BRIDGE_DISPATCH_H
#import "objects.h"

typedef struct { long first, second; } BridgeResult;
IMP bridgeImplementationForMethod(struct methodblock *method, SEL selector);
BridgeResult bridgeInvokeJava(HObject *target, SEL selector, BOOL isStatic, va_list args);
int MorphObjcArgumentsForJava(const char *signature, va_list args);
BOOL _NSJavaObjectRespondsToSelector(HObject *object, id nativeObject, SEL selector);
BOOL _NSJavaClassRespondsToSelector(ClassClass *clazz, Class nativeClass, SEL selector);
#endif
