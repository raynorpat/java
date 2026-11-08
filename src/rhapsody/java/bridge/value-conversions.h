#ifndef JDK115_BRIDGE_VALUE_CONVERSIONS_H
#define JDK115_BRIDGE_VALUE_CONVERSIONS_H
#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
#include "oobj.h"
HObject *_NSNumberToJavaNumber(id number);
id _JavaNumberToNSNumber(HObject *number);
SEL _NSConvertSelFromJava(HObject *selector);
HObject *_NSConvertSelToJava(SEL selector);
HObject *_NSConvertSelToJavaFromStack(SEL *selector, unsigned *size);
void SelectorInitialization(void);
#endif
