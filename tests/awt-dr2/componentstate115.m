#import "../../src/rhapsody/sun/awt/awt_View.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
@interface ComponentStateProbe : NSObject
+ (BOOL)focused:(int)address;
+ (BOOL)cursor:(int)address type:(int)type;
+ (BOOL)repaint:(int)address peer:(int)peer;
+ (void)fractionalSize:(int)address;
@end
@implementation ComponentStateProbe
+ (void)fractionalSize:(int)address { [(NSView *)address setBoundsSize:NSMakeSize(40.25, 30.75)]; }
+ (BOOL)focused:(int)address { NSView *view = (NSView *)address; return [[view window] firstResponder] == view; }
+ (BOOL)cursor:(int)address type:(int)type
{
    static NSString *selectors[] = { @"arrowCursor", @"_crosshairCursor", @"IBeamCursor", @"_waitCursor",
        @"_bottomLeftResizeCursor", @"_bottomRightResizeCursor", @"_topLeftResizeCursor", @"_topRightResizeCursor",
        @"_verticalResizeCursor", @"_verticalResizeCursor", @"_horizontalResizeCursor", @"_horizontalResizeCursor",
        @"_handCursor", @"_moveCursor" };
    return [(AwtView *)address cursor] == [NSCursor performSelector:NSSelectorFromString(selectors[type])];
}
+ (BOOL)repaint:(int)address peer:(int)peer
{
    NSView *view = (NSView *)address;
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jmethodID method = (*env)->GetMethodID(env, cls, "repaintRect", "(IIII)V");
    [view setNeedsDisplay:NO];
    (*env)->CallNonvirtualVoidMethod(env, (jobject)peer, cls, method, 1, 2, 10, 12);
    (*env)->DeleteLocalRef(env, cls);
    return [view needsDisplay];
}
@end
static int viewAddress(JNIEnv *env, jobject peer)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, cls, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT jboolean JNICALL Java_ComponentState115_focused(JNIEnv *env, jclass cls, jobject peer)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ComponentStateProbe appkitProxy] focused:viewAddress(env, peer)];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_ComponentState115_fractionalSize(JNIEnv *env, jclass cls, jobject peer)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ComponentStateProbe appkitProxy] fractionalSize:viewAddress(env, peer)];
    [pool release];
}
JNIEXPORT jboolean JNICALL Java_ComponentState115_cursor(JNIEnv *env, jclass cls, jobject peer, jint type)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ComponentStateProbe appkitProxy] cursor:viewAddress(env, peer) type:type];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_ComponentState115_repaint(JNIEnv *env, jclass cls, jobject peer)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobject global = (*env)->NewGlobalRef(env, peer);
    BOOL result = [[ComponentStateProbe appkitProxy] repaint:viewAddress(env, peer) peer:(int)global];
    (*env)->DeleteGlobalRef(env, global);
    [pool release]; return result;
}
