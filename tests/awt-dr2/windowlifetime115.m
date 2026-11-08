#import "../../src/rhapsody/sun/awt/awt_Window.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Container.h"
static volatile int liveWindows;
static volatile int liveViews;
static int failStage;
static volatile int liveThrowables;
/* Private owner's layout is needed by DR2's fragile Objective-C ABI. */
@interface AwtThrowableReference : NSObject {
    JavaVM *_vm;
    jthrowable _throwable;
}
@end
@interface ThrowableLifetimeProbe : AwtThrowableReference
@end
@implementation ThrowableLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone
{
    id owner = [super allocWithZone:zone];
    if (owner) liveThrowables++;
    return owner;
}
- (void)dealloc { liveThrowables--; [super dealloc]; }
@end
@interface ContainerLifetimeProbe : AwtContainer
@end
@implementation ContainerLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone
{
    id view = [super allocWithZone:zone];
    if (view) liveViews++;
    return view;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font
        backgroundColor:background foregroundColor:foreground cursor:cursor];
    if (failStage == 1) [NSException raise:NSInvalidArgumentException format:@"view construction failure"];
    return self;
}
- (void)dealloc { liveViews--; [super dealloc]; }
@end
@interface WindowLifetimeProbe : AwtWindow
+ (int)visibleWindows;
+ (void)requestCloseForView:(int)view;
@end
@implementation WindowLifetimeProbe
+ (void)requestCloseForView:(int)view
{
    [[(NSView *)view window] windowShouldClose:nil];
}
+ (int)visibleWindows
{
    NSEnumerator *windows = [[NSApp windows] objectEnumerator];
    NSWindow *window;
    int count = 0;
    while ((window = [windows nextObject]))
        if ([window isKindOfClass:[AwtWindow class]] && [window isVisible]) count++;
    return count;
}
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled styleMask:(int)style contentView:(NSView *)view
{
    self = [super commonInitWithPeer:peer frame:frame enabled:enabled styleMask:style contentView:view];
    if (self) liveWindows++;
    if (failStage == 2) [NSException raise:NSInvalidArgumentException format:@"window construction failure"];
    return self;
}
- (void)dealloc { liveWindows--; [super dealloc]; }
@end
JNIEXPORT void JNICALL Java_WindowLifetime115_installTracker(JNIEnv *env, jclass cls)
{
    [ThrowableLifetimeProbe poseAsClass:[AwtThrowableReference class]];
    [WindowLifetimeProbe poseAsClass:[AwtWindow class]];
    [ContainerLifetimeProbe poseAsClass:[AwtContainer class]];
}
JNIEXPORT jint JNICALL Java_WindowLifetime115_liveWindows(JNIEnv *env, jclass cls)
{
    return liveWindows;
}
JNIEXPORT jint JNICALL Java_WindowLifetime115_liveViews(JNIEnv *env, jclass cls)
{
    return liveViews;
}
JNIEXPORT jint JNICALL Java_WindowLifetime115_visibleWindows(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    int count = [[WindowLifetimeProbe appkitProxy] visibleWindows];
    [pool release];
    return count;
}

JNIEXPORT jint JNICALL Java_WindowLifetime115_liveThrowables(JNIEnv *env, jclass cls)
{
    return liveThrowables;
}

JNIEXPORT void JNICALL Java_WindowLifetime115_failAt(JNIEnv *env, jclass cls, jint stage)
{
    failStage = stage;
}

JNIEXPORT void JNICALL Java_WindowLifetime115_requestClose(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    int view = (*env)->GetIntField(env, peer, field);
    [[WindowLifetimeProbe appkitProxy] requestCloseForView:view];
    [pool release];
}
