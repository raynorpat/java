/* DR2 libawt.A.dylib: peer view access at 54012484--540124a8. */
#import "awt_Component.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

static jfieldID nativeViewField(JNIEnv *env)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field;
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "nNativeView", "I");
    (*env)->DeleteLocalRef(env, cls);
    if (!field) [AwtException raiseUsingEnv:env];
    return field;
}
AwtView *realViewFromPeer(JNIEnv *env, jobject peer)
{
    return (AwtView *)(*env)->GetIntField(env, peer, nativeViewField(env));
}
id viewFromPeer(JNIEnv *env, jobject peer) { return [realViewFromPeer(env, peer) appkitProxy]; }
void setViewFromPeer(JNIEnv *env, jobject peer, AwtView *view)
{
    (*env)->SetIntField(env, peer, nativeViewField(env), (jint)view);
}
id windowFromPeer(JNIEnv *env, jobject peer) { return [[viewFromPeer(env, peer) window] windowProxy]; }

/* 54012ee4, 54013084, 54013160: state changes execute through the view proxy. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setFont(JNIEnv *env, jobject peer, jobject font)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetFont:[javaToNSFont(env, font) appkitProxy]];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setForeground(JNIEnv *env, jobject peer, jobject color)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetForegroundColor:[javaToNSColor(env, color) appkitProxy]];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setBackground(JNIEnv *env, jobject peer, jobject color)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetBackgroundColor:[javaToNSColor(env, color) appkitProxy]];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
