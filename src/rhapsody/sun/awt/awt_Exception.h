#ifndef RHAPSODY_AWT_EXCEPTION_H
#define RHAPSODY_AWT_EXCEPTION_H

#import <Foundation/NSException.h>
#include "jni.h"

@interface AwtException : NSException {
    jthrowable _javaException;
}
+ (void)raiseUsingEnv:(JNIEnv *)env;
- (id)initUsingEnv:(JNIEnv *)env;
+ (void)raise:(JNIEnv *)env exceptionName:(const char *)name reason:(const char *)reason;
- (id)init:(JNIEnv *)env exceptionName:(const char *)name reason:(const char *)reason;
+ (void)throwToJava:(JNIEnv *)env exception:(NSException *)exception;
- (jthrowable)javaException;
- (int)javaExceptionHandle;
- (void)raiseToJava:(JNIEnv *)env;
@end

#endif
