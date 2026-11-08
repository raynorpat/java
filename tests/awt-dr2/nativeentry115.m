#import <AppKit/AppKit.h>
#include "jni.h"

extern void *NativeMethodEnter(void *env);
extern void NativeMethodExit(void *env, void *pool);
extern void *get_awt_cookie(void *env);

static void fail(JNIEnv *env, const char *message)
{
    jclass error = (*env)->FindClass(env, "java/lang/Error");
    if (error) (*env)->ThrowNew(env, error, message);
}

JNIEXPORT void JNICALL Java_NativeEntry115_initialize(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    [pool release];
}

JNIEXPORT void JNICALL Java_NativeEntry115_enter(JNIEnv *env, jclass cls, jint depth)
{
    void *token = NativeMethodEnter(env);
    id context = [[[NSThread currentThread] threadDictionary] objectForKey:@"AwtThreadContext Key"];
    jmethodID callback;
    if (!context || ![NSDPSContext currentContext]) {
        fail(env, "missing native thread context");
    } else if ((depth == 2) != (token != NULL)) {
        fail(env, "wrong nested native-entry token");
    } else {
        callback = (*env)->GetStaticMethodID(env, cls, "callback", "(I)V");
        if (callback) (*env)->CallStaticVoidMethod(env, cls, callback, depth);
        if (!(*env)->ExceptionOccurred(env) && get_awt_cookie(env) != context)
            fail(env, "native thread cookie changed during callback");
    }
    if (token) NativeMethodExit(env, token);
}
