/* DR2 libawt.A.dylib 54006788--54006b2c. */
#import <Foundation/Foundation.h>
#import "awt_Exception.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* NSException is copied by NSConnection. Its userInfo carries a remote
 * owner for the JNI global reference, so both copies keep the throwable alive. */
@interface AwtThrowableReference : NSObject {
    JavaVM *_vm;
    jthrowable _throwable;
}
- (id)initWithEnv:(JNIEnv *)env throwable:(jthrowable)value;
- (int)javaExceptionHandle;
@end
@implementation AwtThrowableReference
- (id)initWithEnv:(JNIEnv *)env throwable:(jthrowable)value
{
    self = [super init];
    (*env)->GetJavaVM(env, &_vm);
    _throwable = (*env)->NewGlobalRef(env, value);
    return self;
}
- (int)javaExceptionHandle { return (int)_throwable; }
- (id)replacementObjectForPortCoder:(NSPortCoder *)coder
{
    return [NSDistantObject proxyWithLocal:self connection:[coder connection]];
}
- (void)dealloc
{
    JNIEnv *env;
    /* Owners live on attached Java or AppKit threads. */
    (*_vm)->AttachCurrentThread(_vm, &env, NULL);
    (*env)->DeleteGlobalRef(env, _throwable);
    [super dealloc];
}
@end

static NSDictionary *throwableInfo(JNIEnv *env, jthrowable value)
{
    id owner = [[[AwtThrowableReference alloc] initWithEnv:env throwable:value] autorelease];
    return [NSDictionary dictionaryWithObject:owner forKey:@"JavaThrowable"];
}

@implementation AwtException
+ (void)raiseUsingEnv:(JNIEnv *)env
{
    [[[[AwtException allocWithZone:NULL] initUsingEnv:env] autorelease] raise];
}

- (id)initUsingEnv:(JNIEnv *)env
{
    jthrowable local = (*env)->ExceptionOccurred(env);
    if (local) {
        (*env)->ExceptionClear(env);
        _javaException = local;
    }
    [self initWithName:@"AwtException" reason:@"See Java exception object"
        userInfo:throwableInfo(env, _javaException)];
    (*env)->DeleteLocalRef(env, local);
    _javaException = [self javaException];
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
        jthrowable local;
        (*env)->ThrowNew(env, type, exceptionReason);
        local = (*env)->ExceptionOccurred(env);
        if (local) {
            (*env)->ExceptionClear(env);
            _javaException = local;
        }
    }
    [self initWithName:[NSString stringWithCString:exceptionName]
               reason:[NSString stringWithCString:exceptionReason ? exceptionReason : "unknown"]
               userInfo:throwableInfo(env, _javaException)];
    (*env)->DeleteLocalRef(env, _javaException);
    _javaException = [self javaException];
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
    /* The throwable owner may live on AppKit. Retrieve its global handle
     * and raise locally without sending a thread-local JNIEnv remotely. */
    (*env)->Throw(env, (jthrowable)[translated javaExceptionHandle]);
}

- (int)javaExceptionHandle { return [[[self userInfo] objectForKey:@"JavaThrowable"] javaExceptionHandle]; }
- (jthrowable)javaException { return (jthrowable)[self javaExceptionHandle]; }
- (void)raiseToJava:(JNIEnv *)env { (*env)->Throw(env, [self javaException]); }
@end
