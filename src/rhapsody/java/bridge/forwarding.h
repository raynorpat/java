#ifndef JDK115_BRIDGE_FORWARDING_H
#define JDK115_BRIDGE_FORWARDING_H
#import <Foundation/Foundation.h>
#include "oobj.h"
void _BRIDGEConvertJavaRetvalToObjc(HObject *handle, const char *name, NSObjCValue *value);
HObject *_BRIDGEConvertObjcArgToJava(const char *name, void *argument, int *consumed);
void *_BRIDGEStructReturnAddress(void);
long long _NSInvokeJavaMethod(HObject *target, SEL selector, va_list args, void *destination);
long long _NSInvokeStaticJavaMethod(ClassClass *target, SEL selector, va_list args, void *destination);
char bridgeForwardJava(HObject *target, SEL selector, BOOL isStatic, va_list args, NSObjCValue *value);
void bridgeForwardInvocation(HObject *target, NSInvocation *invocation);
#endif
