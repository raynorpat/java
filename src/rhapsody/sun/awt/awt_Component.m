/* DR2 libawt.A.dylib: peer view access at 54012484--540124a8. */
#import "awt_Component.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#import "awt_Container.h"

/* 540124f8/540112e0/54013344 share construction for View/Canvas/Container. */
static jint createChildView(JNIEnv *env, jobject peer, Class viewClass, const char *reason,
    jint x, jint y, jint width, jint height, jboolean enabled,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    AwtView *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *nativeFont = javaToNSFont(env, font);
        NSColor *nativeBackground = javaToNSColor(env, background);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [viewClass alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:reason];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_createNativeComponent(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    return createChildView(env, peer, [AwtView class], "Cannot allocate AwtView",
        x, y, width, height, enabled, font, background, foreground, cursor);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RCanvasPeer_nCreateNativeCanvas(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    return createChildView(env, peer, [AwtCanvas class], "Cannot allocate AwtCanvas",
        x, y, width, height, enabled, font, background, foreground, cursor);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RContainerPeer_nCreateNativeContainer(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    return createChildView(env, peer, [AwtContainer class], "Cannot allocate AwtContainer",
        x, y, width, height, enabled, font, background, foreground, cursor);
}

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

/* 5401272c: visibility attaches/removes a child, rather than hiding its view. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setChildComponentVisible(JNIEnv *env, jobject peer, jobject childPeer, jboolean visible)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id parent = viewFromPeer(env, peer), child = viewFromPeer(env, childPeer);
        if (visible) { [parent addSubview:child]; [parent setNeedsDisplayInRect:[child frame]]; }
        else [child removeFromSuperview];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setEnabled(JNIEnv *env, jobject peer, jboolean enabled)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetEnabled:enabled];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
/* 540129e8: negative dimensions are clamped, while origins remain signed. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_setBounds(JNIEnv *env, jobject peer, jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setFrame:NSMakeRect(x, y, width > 0 ? width : 0, height > 0 ? height : 0) display:YES];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
/* The donor's 54012954 is empty; release owned views as for native windows. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RComponentPeer_nDispose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        AwtView *view = realViewFromPeer(env, peer);
        jobject globalPeer = [view peer];
        setViewFromPeer(env, peer, nil);
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

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
