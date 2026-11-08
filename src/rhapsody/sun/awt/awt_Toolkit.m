/* DR2 libawt.A.dylib: 5401babc--5401bca4. */
#import "awt_Application.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"
#include <math.h>
#import <AppKit/psops.h>

@interface NSThread (AwtDR2)
- (void)_setThreadName:(NSString *)name;
@end

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RToolkit_initialize(JNIEnv *env, jclass cls)
{
    /* The donor entry is empty; Foundation setup belongs to VM bootstrap. */
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RToolkit_run(JNIEnv *env, jobject toolkit)
{
    NS_DURING
        [[NSThread currentThread] _setThreadName:@"AppKit -run"];
        [AwtApplication sharedApplicationUsingEnv:env awtToolkit:toolkit];
        [NSApp run];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
}

JNIEXPORT jboolean JNICALL Java_com_apple_rhapsody_awt_RToolkit_appkitIsRunning(JNIEnv *env, jobject toolkit)
{
    return [NSApp isRunning];
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RToolkit_tellAppkitAboutAwtToolkit(JNIEnv *env, jobject toolkit)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [NSApp setupForAwtToolkitUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RToolkit_getScreenSize(JNIEnv *env, jobject toolkit)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSRect frame = [[[NSScreen appkitProxy] mainScreen] visibleFrame];
        jclass cls = (*env)->FindClass(env, "java/awt/Dimension");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObject(env, cls, ctor, (jint)ceil(frame.size.width), (jint)ceil(frame.size.height));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RToolkit_getScreenResolution(JNIEnv *env, jobject toolkit)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSDictionary *description = [[[NSScreen appkitProxy] mainScreen] deviceDescription];
        result = (jint)[[description objectForKey:NSDeviceResolution] sizeValue].width;
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RToolkit_nDefaultBackgroundColor(JNIEnv *env, jclass toolkit)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSColor *color = [[NSColor controlColor] colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        if (color) {
            float red, green, blue, alpha;
            jclass cls;
            jmethodID ctor;
            [color getRed:&red green:&green blue:&blue alpha:&alpha];
            cls = (*env)->FindClass(env, "java/awt/Color");
            if (!cls) [AwtException raiseUsingEnv:env];
            ctor = (*env)->GetMethodID(env, cls, "<init>", "(FFF)V");
            if (!ctor) [AwtException raiseUsingEnv:env];
            result = (*env)->NewObject(env, cls, ctor, (double)red, (double)green, (double)blue);
            (*env)->DeleteLocalRef(env, cls);
            if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RToolkit_sync(JNIEnv *env, jobject toolkit)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        PSWait();
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RToolkit_beep(JNIEnv *env, jobject toolkit)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSBeep();
        PSWait();
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
