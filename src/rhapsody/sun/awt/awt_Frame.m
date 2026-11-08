/* DR2 libawt.A.dylib: RFramePeer 5401599c, 54015e34, 54015fbc. */
#import "awt_Component.h"
#import "awt_Container.h"
#import "awt_Window.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSWindow (AWTResize)
- (void)setResizable:(BOOL)resizable;
- (NSSize)minimumSize;
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFramePeer_createNativeFrame(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring title, jboolean resizable)
{
    AwtContainer *view = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSRect screen = [[[NSScreen appkitProxy] mainScreen] visibleFrame];
        NSRect frame = NSMakeRect(x, screen.size.height - (y + height), width, height);
        jobject globalPeer = (*env)->NewGlobalRef(env, peer);
        id window;
        NSSize minimum;
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtContainer alloc];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:frame enabled:enabled
            font:[javaToNSFont(env, font) appkitProxy]
            backgroundColor:[javaToNSColor(env, background) appkitProxy]
            foregroundColor:[javaToNSColor(env, foreground) appkitProxy]
            cursor:javaToNSCursor(env, cursor)];
        window = [[AwtWindow allocInAppkitThread] windowProxy];
        [window initWithPeer:(int)globalPeer frame:frame enabled:enabled contentView:[[view appkitProxy] windowProxy]
            resizable:resizable title:javaToNSString(env, title)];
        minimum = [window minimumSize];
        if (width < minimum.width || height < minimum.height) {
            if (width < minimum.width) frame.size.width = minimum.width;
            if (height < minimum.height) {
                frame.size.height = minimum.height;
                frame.origin.y -= minimum.height;
            }
            [window setFrame:frame display:NO];
        }
        if (!view || !window)
            [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtContainer/AwtWindow"];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFramePeer_setTitle(JNIEnv *env, jobject peer, jstring title)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) setTitle:javaToNSString(env, title)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFramePeer_setResizable(JNIEnv *env, jobject peer, jboolean resizable)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [windowFromPeer(env, peer) setResizable:resizable];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
