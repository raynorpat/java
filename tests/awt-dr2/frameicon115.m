#import <AppKit/AppKit.h>
#import "../../src/rhapsody/sun/awt/awt_Window.h"
#include "jni.h"
static BOOL lastIconArgument;
@interface FrameIconWindowProbe : AwtWindow
@end
@implementation FrameIconWindowProbe
- (void)setMiniwindowImage:(NSImage *)image
{
    /* The guest's MacOS interface style ignores miniwindow images. Observe the
     * setter argument while retaining the framework's normal behavior. */
    lastIconArgument = image != nil;
    [super setMiniwindowImage:image];
}
@end
@interface NSObject (FrameIconTest)
+ (id)appkitProxy;
@end
@interface FrameIconProbe : NSObject
+ (BOOL)nativeIcon:(int)view seed:(BOOL)seed;
+ (void)miniaturize:(int)view mini:(BOOL)mini;
@end
@implementation FrameIconProbe
+ (BOOL)nativeIcon:(int)view seed:(BOOL)seed
{
    NSWindow *window = [(NSView *)view window];
    if (seed) {
        NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];
        [image lockFocus]; [[NSColor redColor] set]; NSRectFill(NSMakeRect(0, 0, 16, 16)); [image unlockFocus];
        [window setMiniwindowImage:image]; [image release];
    }
    return lastIconArgument;
}
+ (void)miniaturize:(int)view mini:(BOOL)mini
{
    NSWindow *window = [(NSView *)view window];
    if (mini) [window miniaturize:nil]; else [window deminiaturize:nil];
}
@end
JNIEXPORT void JNICALL Java_FrameIcon115_track(JNIEnv *env, jclass cls)
{ [FrameIconWindowProbe poseAsClass:[AwtWindow class]]; }
static int viewAddress(JNIEnv *env, jobject frame)
{
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT jboolean JNICALL Java_FrameIcon115_nativeIcon(JNIEnv *env, jclass cls, jobject frame, jboolean seed)
{ id pool = [[NSAutoreleasePool alloc] init]; BOOL result = [[FrameIconProbe appkitProxy] nativeIcon:viewAddress(env, frame) seed:seed]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_FrameIcon115_miniaturize(JNIEnv *env, jclass cls, jobject frame, jboolean mini)
{ id pool = [[NSAutoreleasePool alloc] init]; [[FrameIconProbe appkitProxy] miniaturize:viewAddress(env, frame) mini:mini]; [pool release]; }
