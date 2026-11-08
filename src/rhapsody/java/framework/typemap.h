#ifndef JDK115_PUBLIC_TYPEMAP_H
#define JDK115_PUBLIC_TYPEMAP_H
#import <Foundation/Foundation.h>
#import "java-types.h"
typedef void *(NSJavaClassMapFunction)(void *object);
typedef JAVAHandle (NSJavaTypeMapFunction)(void *value, int *stackSize);
JAVAHandle _NSStringToJavaString(id string);
id _JavaStringToNSString(JAVAHandle string);
JAVAHandle _NSNumberToJavaNumber(id number);
id _JavaNumberToNSNumber(JAVAHandle number);
id _JavaIntegerToNSNumber(JAVAHandle number);
id _JavaFloatToNSNumber(JAVAHandle number);
id _JavaDoubleToNSNumber(JAVAHandle number);
id _JavaLongToNSNumber(JAVAHandle number);
SEL _NSConvertSelFromJava(JAVAHandle selector);
JAVAHandle _NSConvertSelToJava(SEL selector);
JAVAHandle _NSConvertSelToJavaFromStack(SEL *selector, int *stackSize);
NSException *_JavaExceptionToNSException(JAVAHandle exception);
JAVAHandle _NSExceptionToJavaException(NSException *exception);
Class _NSConvertClassFromJava(JAVAClass clazz);
JAVAClass _NSConvertClassToJava(Class clazz);
#endif
