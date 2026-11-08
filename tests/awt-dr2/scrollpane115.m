#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_ScrollView.h"
#include <math.h>
static volatile int panes;
static int failConstruction;
@interface ScrollPaneLifetimeProbe : AwtScrollView
@end
@implementation ScrollPaneLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) panes++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor scrollerPolicy:(int)policy verticalLineIncrement:(int)vertical horizontalLineIncrement:(int)horizontal
{
    self = [super initWithPeer:peer frame:frame enabled:enabled backgroundColor:background foregroundColor:foreground cursor:cursor scrollerPolicy:policy verticalLineIncrement:vertical horizontalLineIncrement:horizontal];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"scroll pane construction failure"];
    return self;
}
- (void)dealloc { panes--; [super dealloc]; }

@end
@interface ScrollPaneProbe : NSObject
+ (BOOL)state:(int)address x:(int)x y:(int)y enabled:(BOOL)enabled;
+ (void)track;
+ (int)live;
+ (void)click:(int)address part:(int)part vertical:(BOOL)vertical;
+ (void)drag:(int)address vertical:(BOOL)vertical;
@end
@implementation ScrollPaneProbe
+ (int)live { return panes; }
+ (BOOL)state:(int)address x:(int)x y:(int)y enabled:(BOOL)enabled
{
    NSScrollView *pane = (NSScrollView *)address;
    NSPoint origin = [[pane contentView] bounds].origin;
    return fabs(origin.x - x) < 0.01 && fabs(origin.y - y) < 0.01 &&
        (! [pane hasHorizontalScroller] || [[pane horizontalScroller] isEnabled] == enabled) &&
        (! [pane hasVerticalScroller] || [[pane verticalScroller] isEnabled] == enabled) &&
        [pane horizontalLineScroll] == 7 && [pane verticalLineScroll] == 9;
}
+ (void)track { [ScrollPaneLifetimeProbe poseAsClass:[AwtScrollView class]]; }
+ (void)click:(int)address part:(int)part vertical:(BOOL)vertical
{
    NSScrollView *pane = (NSScrollView *)address;
    NSScroller *view = vertical ? [pane verticalScroller] : [pane horizontalScroller];
    NSWindow *window = [view window];
    NSRect rect = [view rectForPart:part];
    NSPoint point = [view convertPoint:NSMakePoint(NSMidX(rect), NSMidY(rect)) toView:nil];
    int index, types[] = {NSLeftMouseDown, NSLeftMouseUp};
    [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil];
    for (index = 0; index < 2; index++)
        [NSApp postEvent:[NSEvent mouseEventWithType:types[index] location:point modifierFlags:0
            timestamp:1 + index windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:index + 1 clickCount:1 pressure:0.5] atStart:NO];
}
+ (void)drag:(int)address vertical:(BOOL)vertical
{
    NSScrollView *pane = (NSScrollView *)address;
    NSScroller *view = vertical ? [pane verticalScroller] : [pane horizontalScroller];
    NSWindow *window = [view window];
    NSRect knob = [view rectForPart:NSScrollerKnob], slot = [view rectForPart:NSScrollerKnobSlot];
    NSPoint start = NSMakePoint(NSMidX(knob), NSMidY(knob));
    NSPoint end = vertical ? NSMakePoint(start.x, slot.origin.y + slot.size.height * 0.8) :
        NSMakePoint(slot.origin.x + slot.size.width * 0.8, start.y);
    int index, types[] = {NSLeftMouseDown, NSLeftMouseDragged, NSLeftMouseUp};
    [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil];
    for (index = 0; index < 3; index++)
        [NSApp postEvent:[NSEvent mouseEventWithType:types[index]
            location:[view convertPoint:index == 0 ? start : end toView:nil] modifierFlags:0
            timestamp:1 + index windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:index + 1 clickCount:1 pressure:0.5] atStart:NO];
}
@end
static int viewAddress(JNIEnv *env, jobject component)
{
    jclass cls = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, cls, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, component, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT void JNICALL Java_ScrollPane115_click(JNIEnv *env, jclass cls, jobject bar, jint part, jboolean vertical)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ScrollPaneProbe appkitProxy] click:viewAddress(env, bar) part:part vertical:vertical];
    [pool release];
}
JNIEXPORT void JNICALL Java_ScrollPane115_drag(JNIEnv *env, jclass cls, jobject bar, jboolean vertical)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ScrollPaneProbe appkitProxy] drag:viewAddress(env, bar) vertical:vertical];
    [pool release];
}
JNIEXPORT void JNICALL Java_ScrollPane115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init]; [[ScrollPaneProbe appkitProxy] track]; [pool release];
}
JNIEXPORT jint JNICALL Java_ScrollPane115_livePanes(JNIEnv *env, jclass cls) {
    id pool = [[NSAutoreleasePool alloc] init];
    /* Flush pending DO wire releases before counting native controls. */
    int result = [[ScrollPaneProbe appkitProxy] live];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_ScrollPane115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }

JNIEXPORT jboolean JNICALL Java_ScrollPane115_state(JNIEnv *env, jclass cls, jobject pane, jint x, jint y, jboolean enabled)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ScrollPaneProbe appkitProxy] state:viewAddress(env, pane) x:x y:y enabled:enabled];
    [pool release]; return result;
}
