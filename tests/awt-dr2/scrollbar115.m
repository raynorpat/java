#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Scroller.h"
#include <math.h>
static volatile int scrollers;
static int failConstruction;
@interface ScrollerLifetimeProbe : AwtScroller
@end
@implementation ScrollerLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) scrollers++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor isVertical:(BOOL)vertical minValue:(int)minimum maxValue:(int)maximum value:(int)value visibleAmount:(int)visible lineIncrement:(int)line pageIncrement:(int)page
{
    self = [super initWithPeer:peer frame:frame enabled:enabled backgroundColor:background foregroundColor:foreground cursor:cursor isVertical:vertical minValue:minimum maxValue:maximum value:value visibleAmount:visible lineIncrement:line pageIncrement:page];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"scrollbar construction failure"];
    return self;
}
- (void)dealloc { scrollers--; [super dealloc]; }
@end
@interface ScrollbarProbe : NSObject
+ (void)track;
+ (int)live;
+ (BOOL)state:(int)address value:(int)value visible:(int)visible minimum:(int)minimum maximum:(int)maximum;
+ (void)click:(int)address part:(int)part;
+ (void)drag:(int)address vertical:(BOOL)vertical;
@end
@implementation ScrollbarProbe
+ (int)live { return scrollers; }
+ (void)track { [ScrollerLifetimeProbe poseAsClass:[AwtScroller class]]; }
+ (BOOL)state:(int)address value:(int)value visible:(int)visible minimum:(int)minimum maximum:(int)maximum
{
    NSScroller *view = (NSScroller *)address;
    int range = maximum - visible - minimum;
    if (range < 1) range = 1;
    return fabs([view floatValue] - (float)(value - minimum) / range) < 0.00001 &&
        fabs([view knobProportion] - (float)visible / (maximum - minimum)) < 0.00001;
}
+ (void)click:(int)address part:(int)part
{
    NSScroller *view = (NSScroller *)address;
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
    NSScroller *view = (NSScroller *)address;
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
JNIEXPORT jboolean JNICALL Java_Scrollbar115_state(JNIEnv *env, jclass cls, jobject bar, jint value, jint visible, jint minimum, jint maximum)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ScrollbarProbe appkitProxy] state:viewAddress(env, bar) value:value visible:visible minimum:minimum maximum:maximum];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Scrollbar115_click(JNIEnv *env, jclass cls, jobject bar, jint part)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ScrollbarProbe appkitProxy] click:viewAddress(env, bar) part:part];
    [pool release];
}
JNIEXPORT void JNICALL Java_Scrollbar115_drag(JNIEnv *env, jclass cls, jobject bar, jboolean vertical)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ScrollbarProbe appkitProxy] drag:viewAddress(env, bar) vertical:vertical];
    [pool release];
}
JNIEXPORT void JNICALL Java_Scrollbar115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init]; [[ScrollbarProbe appkitProxy] track]; [pool release];
}
JNIEXPORT jint JNICALL Java_Scrollbar115_liveScrollers(JNIEnv *env, jclass cls) {
    id pool = [[NSAutoreleasePool alloc] init];
    /* Flush pending DO wire releases before counting native controls. */
    int result = [[ScrollbarProbe appkitProxy] live];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Scrollbar115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
