/* DR2 libawt.A.dylib: RDialogPeer 54013738--54013c8c. */
#import "awt_Component.h"
#import "awt_Container.h"
#import "awt_Window.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RDialogPeer_createNativeDialog(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring title, jboolean resizable, jboolean modal)
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
        NSString *nativeTitle = javaToNSString(env, title);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtContainer alloc];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:frame enabled:enabled
            font:[nativeFont appkitProxy]
            backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy]
            cursor:nativeCursor];
        window = [[AwtWindow allocInAppkitThread] windowProxy];
        [window initWithPeer:(int)globalPeer frame:frame enabled:enabled contentView:[[view appkitProxy] windowProxy]
            resizable:resizable title:nativeTitle parent:nil];
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
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RDialogPeer_setTitle(JNIEnv *env, jobject peer, jstring title)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) setTitle:javaToNSString(env, title)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
/* 54013bf0: the original binding leaves the native resize style unchanged. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RDialogPeer_setResizable(JNIEnv *env, jobject peer, jboolean resizable)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RDialogPeer_ensureSuitableFirstResponder(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) ensureSuitableFirstResponderForDialog];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RDialogPeer_modalSetVisible(JNIEnv *env, jobject peer, jboolean visible)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        if (visible) [[NSApp appkitProxy] runModalForWindow:windowFromPeer(env, peer)];
        else {
            [NSApp stopModal];
            [windowFromPeer(env, peer) orderOut:nil];
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
