#import "../../src/rhapsody/sun/awt/awt_Window.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Container.h"
static volatile int liveWindows;
static volatile int liveViews;
@interface ContainerLifetimeProbe : AwtContainer
@end
@implementation ContainerLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone
{
    id view = [super allocWithZone:zone];
    if (view) liveViews++;
    return view;
}
- (void)dealloc { liveViews--; [super dealloc]; }
@end
@interface WindowLifetimeProbe : AwtWindow
+ (int)visibleWindows;
@end
@implementation WindowLifetimeProbe
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
    return self;
}
- (void)dealloc { liveWindows--; [super dealloc]; }
@end
JNIEXPORT void JNICALL Java_WindowLifetime115_installTracker(JNIEnv *env, jclass cls)
{
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
