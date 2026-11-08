/* DR2 libawt.A.dylib: RWindowPeer 5401c5ac--5401ce7c. */
#import "awt_Component.h"
#import "awt_Container.h"
#import "awt_Conversions.h"
#include <math.h>
#import "awt_Window.h"
#import "awt_Application.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSWindow (AWTWindowSizing)
- (NSSize)preferredSize;
- (NSSize)minimumSize;
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_createNativeWindow(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    AwtContainer *volatile view = nil;
    jobject volatile globalPeer = NULL;
    id volatile window = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSRect screen = [[[NSScreen appkitProxy] mainScreen] visibleFrame];
        NSRect frame = NSMakeRect(x, screen.size.height - (y + height), width, height);
        /* Java accessors may throw; finish conversion before taking ownership. */
        NSFont *nativeFont = javaToNSFont(env, font);
        NSColor *nativeBackground = javaToNSColor(env, background);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSSize minimum;
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtContainer alloc];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:frame enabled:enabled
            font:[nativeFont appkitProxy]
            backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy]
            cursor:nativeCursor];
        window = [[AwtWindow allocInAppkitThread] windowProxy];
        [window initWithPeer:(int)globalPeer frame:frame enabled:enabled contentView:[[view appkitProxy] windowProxy]];
        minimum = [window minimumSize];
        if (width < minimum.width || height < minimum.height) {
            if (width < minimum.width) frame.size.width = minimum.width;
            if (height < minimum.height) {
                frame.size.height = minimum.height;
                frame.origin.y -= minimum.height;
            }
            /* Creation may hold the Java tree lock. Reflect the minimum
             * geometry on this caller after AppKit returns, not in its delegate. */
            [window awtSetFrame:frame display:NO];
            awtNotifyJavaFrameChanged(env, peer, frame, screen.size.height);
        }
        if (!view || !window)
            [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtContainer/AwtWindow"];
    NS_HANDLER
        [[AwtWindow appkitProxy] discardWindow:window view:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_dispose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        AwtView *view = realViewFromPeer(env, peer);
        if (view) {
            jobject globalPeer = [view peer];
            id window = [[view appkitProxy] window];
            setViewFromPeer(env, peer, nil);
            if (window) [window disposeAwtWindow];
            else [[AwtView appkitProxy] discardView:(int)view];
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

static jobject windowDimension(JNIEnv *env, jobject peer, BOOL minimum)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id window = windowFromPeer(env, peer);
        NSSize size = minimum ? [window minimumSize] : [window preferredSize];
        jclass cls = (*env)->FindClass(env, "java/awt/Dimension");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObject(env, cls, ctor, (int)ceil(size.width), (int)ceil(size.height));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_getPreferredSize(JNIEnv *env, jobject peer)
{ return windowDimension(env, peer, NO); }
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_getMinimumSize(JNIEnv *env, jobject peer)
{ return windowDimension(env, peer, YES); }
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_requestFocus(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        id window = windowFromPeer(env, peer);
        [window makeKeyAndOrderFront:nil];
        [window makeFirstResponder:[window contentView]];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jboolean JNICALL Java_com_apple_rhapsody_awt_RWindowPeer_isFocusTraversable(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    if (pool) NativeMethodExit(env, pool);
    return JNI_FALSE;
}
