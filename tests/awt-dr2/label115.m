#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_TextField.h"
static volatile int labels;
static int failConstruction;
@interface LabelLifetimeProbe : AwtTextField
@end
@implementation LabelLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id view = [super allocWithZone:zone]; if (view) labels++; return view; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text alignment:(int)alignment
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor text:text alignment:alignment];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"label construction failure"];
    return self;
}
- (void)dealloc { labels--; [super dealloc]; }
@end
@interface LabelProbe : NSObject
+ (void)track;
+ (void)inject:(int)address;
+ (BOOL)state:(int)address text:(NSString *)text alignment:(int)alignment enabled:(BOOL)enabled size:(int)size;
+ (BOOL)colors:(int)address;
@end
@implementation LabelProbe
+ (void)track { [LabelLifetimeProbe poseAsClass:[AwtTextField class]]; }
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
+ (BOOL)state:(int)address text:(NSString *)text alignment:(int)alignment enabled:(BOOL)enabled size:(int)size
{
    NSTextField *view = (NSTextField *)address;
    int alignments[] = {0, 2, 1};
    return [[view stringValue] isEqual:text] && [view alignment] == alignments[alignment] &&
        [view isEnabled] == enabled && [[view font] pointSize] == size &&
        ![view isEditable] && ![view isSelectable] && ![view isBordered] && ![view isBezeled];
}
+ (BOOL)colors:(int)address
{
    NSTextField *view = (NSTextField *)address;
    NSColor *foreground = [[view textColor] colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    NSColor *background = [[view backgroundColor] colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    return [view drawsBackground] && [foreground redComponent] == 1 && [foreground greenComponent] == 0 &&
        [foreground blueComponent] == 0 && [background redComponent] == 1 && [background greenComponent] == 1 && [background blueComponent] == 0;
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
JNIEXPORT jboolean JNICALL Java_Label115_state(JNIEnv *env, jclass cls, jobject label, jstring text, jint alignment, jboolean enabled, jint size)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[LabelProbe appkitProxy] state:viewAddress(env, label) text:javaToNSString(env, text) alignment:alignment enabled:enabled size:size];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_Label115_colors(JNIEnv *env, jclass cls, jobject label)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[LabelProbe appkitProxy] colors:viewAddress(env, label)];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Label115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[LabelProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT void JNICALL Java_Label115_inject(JNIEnv *env, jclass cls, jobject label)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[LabelProbe appkitProxy] inject:viewAddress(env, label)];
    [pool release];
}
JNIEXPORT void JNICALL Java_Label115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
JNIEXPORT jint JNICALL Java_Label115_liveLabels(JNIEnv *env, jclass cls) { return labels; }
