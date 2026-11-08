#ifndef JDK115_PUBLIC_NSJAVAEXCEPTION_H
#define JDK115_PUBLIC_NSJAVAEXCEPTION_H
#import <Foundation/Foundation.h>
struct Hjava_lang_Object;

@interface NSJavaException : NSException {
    struct Hjava_lang_Object *_javaException;
}
- initWithName:(NSString *)name reason:(NSString *)reason
    userInfo:(NSDictionary *)info javaException:(struct Hjava_lang_Object *)exception;
- (struct Hjava_lang_Object *)javaException;
- (NSString *)stackTrace;
@end
#endif
