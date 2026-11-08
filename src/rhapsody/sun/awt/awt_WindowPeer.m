/* DR2 libawt.A.dylib: RWindowPeer 5401c5ac--5401ce7c. */
#import "awt_Component.h"
#import "awt_Window.h"
#import "awt_Application.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_dispose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        AwtView *view = realViewFromPeer(env, peer);
        if (view) {
            jobject globalPeer = [view peer];
            id window = [[view appkitProxy] window];
            setViewFromPeer(env, peer, nil);
            [window disposeAwtWindow];
            (*env)->DeleteGlobalRef(env, globalPeer);
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_setVisible(JNIEnv *env, jobject peer, jboolean visible)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        if (visible) [windowFromPeer(env, peer) makeKeyAndOrderFront:nil];
        else [windowFromPeer(env, peer) orderOut:nil];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_setBounds(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id window = windowFromPeer(env, peer);
        NSRect screen = [[window screen] visibleFrame];
        [window awtSetFrame:NSMakeRect(x, screen.size.height - (y + height), width, height) display:YES];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_getInsets(JNIEnv *env, jobject peer)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id window = windowFromPeer(env, peer);
        NSRect frame = [window frame];
        NSRect content = [[NSWindow appkitProxy] contentRectForFrameRect:frame styleMask:[window styleMask]];
        jclass cls = (*env)->FindClass(env, "java/awt/Insets");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(IIII)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObject(env, cls, ctor, (int)(NSMaxY(frame) - NSMaxY(content)),
            (int)(content.origin.x - frame.origin.x), (int)(content.origin.y - frame.origin.y),
            (int)(NSMaxX(frame) - NSMaxX(content)));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_getLocationOnScreen(JNIEnv *env, jobject peer)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id window = windowFromPeer(env, peer);
        NSRect frame = [window frame];
        NSRect screen = [[window screen] visibleFrame];
        jclass cls = (*env)->FindClass(env, "java/awt/Point");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObject(env, cls, ctor, (int)frame.origin.x, (int)(screen.size.height - NSMaxY(frame)));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_toFront(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) orderFront:nil];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_toBack(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) orderBack:nil];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
