#import <Foundation/Foundation.h>
#include "jni.h"

/* The same test links against either the donor class or our implementation. */
@interface NSObject (ExceptionBridgeProbe)
+ (void)raiseUsingEnv:(JNIEnv *)env;
+ (void)raise:(JNIEnv *)env exceptionName:(const char *)name reason:(const char *)reason;
+ (void)throwToJava:(JNIEnv *)env exception:(NSException *)exception;
@end

JNIEXPORT void JNICALL Java_ExceptionBridge115_test(JNIEnv *env, jclass cls, jint mode, jthrowable value)
{
    id pool;
    Class bridge;
    jclass system;
    jmethodID gc, callback;
    pool = [[NSAutoreleasePool alloc] init];
    bridge = NSClassFromString(@"AwtException");
    system = (*env)->FindClass(env, "java/lang/System");
    gc = (*env)->GetStaticMethodID(env, system, "gc", "()V");
    callback = (*env)->GetStaticMethodID(env, cls, "callback", "()V");
    NS_DURING
        if (mode == 0) {
            (*env)->Throw(env, value);
            [bridge raiseUsingEnv:env];
        } else if (mode == 1) {
            [NSException raise:NSInvalidArgumentException format:@"native failure"];
        } else if (mode == 2) {
            [bridge raise:env exceptionName:"java/lang/IllegalArgumentException" reason:"new Java failure"];
        } else if (mode == 3) {
            (*env)->CallStaticVoidMethod(env, cls, callback);
            [bridge raiseUsingEnv:env];
        } else {
            [bridge raise:env exceptionName:mode == 4 ? "no/such/Throwable" : NULL reason:"fallback"];
        }
    NS_HANDLER
        (*env)->CallStaticVoidMethod(env, system, gc);
        [bridge throwToJava:env exception:localException];
    NS_ENDHANDLER
    [pool release];
}
