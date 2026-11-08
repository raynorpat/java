/* Public Objective-C proxy interface. Java handles remain opaque to clients. */
#ifndef JDK115_PUBLIC_NSJAVAOBJECT_H
#define JDK115_PUBLIC_NSJAVAOBJECT_H
#import <Foundation/Foundation.h>
#include <stdarg.h>
#import "java-types.h"

@interface NSJavaObject : NSObject <NSCoding> {
    struct Hjava_lang_Object *_javaHandle;
}
+ (id)objectWithJavaHandle:(struct Hjava_lang_Object *)handle;
+ (id)newWithSignature:(NSString *)signature, ...;
+ (id)newWithSignature:(NSString *)signature arguments:(va_list)args;
- initWithJavaHandle:(struct Hjava_lang_Object *)handle;
- (struct Hjava_lang_Object *)javaHandle;
- (struct Hjava_lang_Class *)javaClass;
- (id)valueForKey:(NSString *)key;
- (void)takeValue:(id)value forKey:(NSString *)key;
@end
#endif
