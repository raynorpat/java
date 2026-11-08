#ifndef JDK115_NATIVE_OBJECT_H
#define JDK115_NATIVE_OBJECT_H
#import <Foundation/Foundation.h>
#include "oobj.h"
void CreateInitialization(void);
HObject *NSCreateJavaWrapper(id object);
HObject *NSCreateExplicitJavaWrapper(id object, ClassClass *clazz);
void ReplaceRetainReleaseMethods(Class clazz);
id _NSAllocateObjcObjectForJavaHandle(HObject *handle);
void _NSAssociateJavaObject(HObject *handle, id object);
void _NSAssociateJavaObjectNoRetain(HObject *handle, id object);
void _NSDisassociateJavaObjectNoRelease(HObject *handle, id object);
id ValueForKey(id object, SEL selector, NSString *key);
void TakeValueForKey(id object, SEL selector, id value, NSString *key);
#endif
