#ifndef JDK115_BRIDGE_OBJECTS_H
#define JDK115_BRIDGE_OBJECTS_H
#import "class-map.h"
#include "interpreter.h"

#import "../framework/NSJavaObject.h"
void MappingInitialization(void);
Class NSJavaClassToObjcClass(ClassClass *clazz, BOOL create);
HObject *NSIdToJavaHandle(id object);
id NSJavaHandleToId(HObject *handle);
id NSCreateJavaObject(const char *name, const char *signature, ...);
id NSCreateJavaObjectV(const char *name, const char *signature, BOOL shortFloats, va_list args);
id _NSCreateJavaObjectV(ClassClass *clazz, const char *signature, BOOL shortFloats, va_list args);
#endif
