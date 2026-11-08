/* DR2 libawt.A.dylib: 54013e88, javaOtherEventToNSEvent 54006034. */
#import "awt_Application.h"
#import "awt_Event.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#import "awt_Component.h"

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_REventQueue_postSyntheticOtherEvent(JNIEnv *env, jobject queue, jobject event)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSEvent *cover = [SyntheticEvent coverEventForJavaEvent:event inEnv:env];
        [[NSApp appkitProxy] postEvent:[cover appkitProxy] atStart:NO];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

/* 54013f80/540140ac: the original Java object supplies values, not a cover. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_REventQueue_postSyntheticKeyEvent(JNIEnv *env, jobject queue,
    jobject event, jint type, jobject peer, jint code, jchar character, jint modifiers)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSEvent *native = [SyntheticEvent keyEvent:type keyCode:code keyChar:character
            modifiers:modifiers forView:viewFromPeer(env, peer)];
        if (native) [[NSApp appkitProxy] postEvent:[native appkitProxy] atStart:NO];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_REventQueue_postSyntheticMouseEvent(JNIEnv *env, jobject queue,
    jobject event, jint type, jobject peer, jint x, jint y, jint clicks, jint modifiers)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSEvent *native = [SyntheticEvent mouseEvent:type x:x y:y modifiers:modifiers
            clickCount:clicks forView:viewFromPeer(env, peer)];
        if (native) [[NSApp appkitProxy] postEvent:[native appkitProxy] atStart:NO];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
