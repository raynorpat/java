#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Window.h"
#import "../../src/rhapsody/sun/awt/awt_Container.h"
static int liveObjects, failureStage;
@interface BareWindowLifetimeProbe : AwtWindow
@end
@implementation BareWindowLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveObjects++; return value; }
- (void)dealloc { liveObjects--; [super dealloc]; }
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled styleMask:(int)style contentView:(NSView *)view
{
    self = [super commonInitWithPeer:peer frame:frame enabled:enabled styleMask:style contentView:view];
    if (failureStage == 2) [NSException raise:NSInvalidArgumentException format:@"bare window construction failure"];
    return self;
}
@end
@interface BareContainerLifetimeProbe : AwtContainer
@end
@implementation BareContainerLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveObjects++; return value; }
- (void)dealloc { liveObjects--; [super dealloc]; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor];
    if (failureStage == 1) [NSException raise:NSInvalidArgumentException format:@"bare view construction failure"];
    return self;
}
@end
@interface BareWindowProbe : NSObject
+ (void)track;
+ (int)liveObjects;
+ (BOOL)focusedView:(int)address;
+ (void)fractionalWindow:(int)address;
+ (void)closeWindow:(int)address;
@end
@implementation BareWindowProbe
+ (void)track { [BareWindowLifetimeProbe poseAsClass:[AwtWindow class]]; [BareContainerLifetimeProbe poseAsClass:[AwtContainer class]]; }
+ (int)liveObjects
{
    /* Synthetic Java events can leave AppKit inside nextEventMatchingMask.
     * Advance its outer loop so deferred view releases drain before recounting. */
    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
        modifierFlags:0 timestamp:1 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
    return liveObjects;
}
+ (BOOL)focusedView:(int)address { NSView *view = (NSView *)address; return [[view window] firstResponder] == view; }
+ (void)fractionalWindow:(int)address
{
    AwtWindow *window = (AwtWindow *)[(NSView *)address window];
    NSRect frame = [window frame]; frame.size = NSMakeSize(220.25, 130.75);
    [window awtSetFrame:frame display:NO];
}
+ (void)closeWindow:(int)address { [[(NSView *)address window] windowShouldClose:nil]; }
@end
static int viewAddress(JNIEnv *env, jobject peer)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, cls, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT void JNICALL Java_BareWindow115_track(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[BareWindowProbe appkitProxy] track]; [pool release]; }
JNIEXPORT jint JNICALL Java_BareWindow115_liveObjects(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; int result = [[BareWindowProbe appkitProxy] liveObjects]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_BareWindow115_failAt(JNIEnv *env, jclass cls, jint stage) { failureStage = stage; }
JNIEXPORT jboolean JNICALL Java_BareWindow115_focused(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; BOOL result = [[BareWindowProbe appkitProxy] focusedView:viewAddress(env, peer)]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_BareWindow115_fractional(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; [[BareWindowProbe appkitProxy] fractionalWindow:viewAddress(env, peer)]; [pool release]; }
JNIEXPORT void JNICALL Java_BareWindow115_close(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; [[BareWindowProbe appkitProxy] closeWindow:viewAddress(env, peer)]; [pool release]; }
