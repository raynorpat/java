/* DR2 libawt.A.dylib 54006788--54006b2c. */
#import <Foundation/Foundation.h>
#import "awt_Exception.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

@implementation AwtException
+ (void)raiseUsingEnv:(JNIEnv *)env
{
    [[[[AwtException allocWithZone:NULL] initUsingEnv:env] autorelease] raise];
}

- (id)initUsingEnv:(JNIEnv *)env
{
    _javaException = (*env)->ExceptionOccurred(env);
    if (_javaException) (*env)->ExceptionClear(env);
    [self initWithName:@"AwtException" reason:@"See Java exception object" userInfo:nil];
    return self;
}

+ (void)raise:(JNIEnv *)env exceptionName:(const char *)exceptionName reason:(const char *)exceptionReason
{
    [[[[AwtException allocWithZone:NULL] init:env exceptionName:exceptionName reason:exceptionReason] autorelease] raise];
}

- (id)init:(JNIEnv *)env exceptionName:(const char *)exceptionName reason:(const char *)exceptionReason
{
    static jclass panicExceptionClass;
    jclass type = exceptionName ? (*env)->FindClass(env, exceptionName) : NULL;
    char *message = NULL;
    if (!type) {
        if (!panicExceptionClass) {
            type = (*env)->FindClass(env, "java/awt/AWTError");
            if (type) panicExceptionClass = (*env)->NewGlobalRef(env, type);
        }
        type = panicExceptionClass;
        if (exceptionName) {
            /* Preserve the donor's literal .80 suffix without its fixed-size
             * sprintf buffer overflow for a long class name. */
            message = malloc(strlen(exceptionName) + 32);
            if (message) {
                sprintf(message, "Unknown throwable class: %s.80", exceptionName);
                exceptionReason = message;
            } else exceptionReason = "Unknown throwable class, out of memory!";
        } else exceptionReason = "Missing Java exception class name while trying to throw a new Java exception";
        exceptionName = "java/awt/AWTError";
    }
    if (type) {
        (*env)->ThrowNew(env, type, exceptionReason);
        _javaException = (*env)->ExceptionOccurred(env);
        if (_javaException) (*env)->ExceptionClear(env);
    }
    [self initWithName:[NSString stringWithCString:exceptionName]
               reason:[NSString stringWithCString:exceptionReason ? exceptionReason : "unknown"] userInfo:nil];
    free(message);
    return self;
}

+ (void)throwToJava:(JNIEnv *)env exception:(NSException *)exception
{
    AwtException *translated;
    if ([exception isKindOfClass:[AwtException class]]) translated = (AwtException *)exception;
    else {
        NSString *message = [NSString stringWithFormat:@"Non-Java exception raised, not handled! (Original problem: %@)",
                                                        [exception description]];
        translated = [[[AwtException allocWithZone:NULL] init:env
            exceptionName:"java/lang/RuntimeException" reason:[message cString]] autorelease];
    }
    [translated raiseToJava:env];
}

- (jthrowable)javaException { return _javaException; }
- (void)raiseToJava:(JNIEnv *)env { (*env)->Throw(env, _javaException); }
@end
