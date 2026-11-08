#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_Button.h"
static volatile int buttons;
static int failConstruction;
@interface ButtonLifetimeProbe : AwtButton
@end
@implementation ButtonLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id button = [super allocWithZone:zone]; if (button) buttons++; return button; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor title:title];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"button construction failure"];
    return self;
}
- (void)dealloc { buttons--; [super dealloc]; }
@end
@interface ButtonProbe : NSObject
+ (void)track;
+ (void)click:(int)address;
+ (void)inject:(int)address;
+ (BOOL)state:(int)address title:(NSString *)title enabled:(BOOL)enabled size:(int)size;
+ (BOOL)foreground:(int)address;
@end
@implementation ButtonProbe
+ (void)track { [ButtonLifetimeProbe poseAsClass:[AwtButton class]]; }
+ (void)click:(int)address
{
    /* A direct DO invocation needs the autorelease boundary of a native event. */
    id pool = [[NSAutoreleasePool alloc] init];
    [(NSButton *)address performClick:nil];
    [pool release];
}
+ (void)inject:(int)address
{
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    NSPoint point = [view convertPoint:NSMakePoint(10, 10) toView:nil];
    int index, types[] = {NSLeftMouseDown, NSLeftMouseUp};
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
    for (index = 0; index < 2; index++)
        [NSApp postEvent:[NSEvent mouseEventWithType:types[index] location:point
            modifierFlags:0 timestamp:1.0 + index windowNumber:[window windowNumber]
            context:[NSApp context] eventNumber:index + 1 clickCount:1 pressure:0.5] atStart:NO];
}
+ (BOOL)foreground:(int)address
{
    NSColor *color = [[(NSButton *)address attributedTitle] attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:NULL];
    color = [color colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    return [color redComponent] == 1 && [color greenComponent] == 0 && [color blueComponent] == 0;
}
+ (BOOL)state:(int)address title:(NSString *)title enabled:(BOOL)enabled size:(int)size
{
    NSButton *button = (NSButton *)address;
    return [[button title] isEqual:title] && [button isEnabled] == enabled && [[button font] pointSize] == size;
}
@end
static int viewAddress(JNIEnv *env, jobject button)
{
    jclass cls = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, cls, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, button, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT void JNICALL Java_Button115_click(JNIEnv *env, jclass cls, jobject button)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ButtonProbe appkitProxy] click:viewAddress(env, button)];
    [pool release];
}
JNIEXPORT void JNICALL Java_Button115_inject(JNIEnv *env, jclass cls, jobject button)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ButtonProbe appkitProxy] inject:viewAddress(env, button)];
    [pool release];
}
JNIEXPORT void JNICALL Java_Button115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ButtonProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT jint JNICALL Java_Button115_liveButtons(JNIEnv *env, jclass cls) { return buttons; }
JNIEXPORT void JNICALL Java_Button115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
JNIEXPORT jboolean JNICALL Java_Button115_foreground(JNIEnv *env, jclass cls, jobject button)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ButtonProbe appkitProxy] foreground:viewAddress(env, button)];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_Button115_state(JNIEnv *env, jclass cls, jobject button, jstring title, jboolean enabled, jint size)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[ButtonProbe appkitProxy] state:viewAddress(env, button) title:javaToNSString(env, title) enabled:enabled size:size];
    [pool release]; return result;
}
