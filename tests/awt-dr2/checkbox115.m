#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_Button.h"
static volatile int buttons;
static int failConstruction;
@interface CheckboxLifetimeProbe : AwtButton
@end
@implementation CheckboxLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id button = [super allocWithZone:zone]; if (button) buttons++; return button; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title state:(BOOL)state inGroup:(BOOL)inGroup
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor title:title state:state inGroup:inGroup];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"checkbox construction failure"];
    return self;
}
- (void)dealloc { buttons--; [super dealloc]; }
@end
@interface CheckboxProbe : NSObject
+ (void)track;
+ (void)inject:(int)address;
+ (void)click:(int)address;
+ (BOOL)state:(int)address title:(NSString *)title selected:(BOOL)selected;
@end
@implementation CheckboxProbe
+ (void)track { [CheckboxLifetimeProbe poseAsClass:[AwtButton class]]; }
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
+ (void)click:(int)address
{
    id pool = [[NSAutoreleasePool alloc] init];
    [(NSButton *)address performClick:nil];
    [pool release];
}
+ (BOOL)state:(int)address title:(NSString *)title selected:(BOOL)selected
{
    NSButton *button = (NSButton *)address;
    return [[button title] isEqual:title] && [button state] == selected;
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
JNIEXPORT void JNICALL Java_Checkbox115_click(JNIEnv *env, jclass cls, jobject box)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[CheckboxProbe appkitProxy] click:viewAddress(env, box)];
    [pool release];
}
JNIEXPORT void JNICALL Java_Checkbox115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[CheckboxProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT void JNICALL Java_Checkbox115_inject(JNIEnv *env, jclass cls, jobject box)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[CheckboxProbe appkitProxy] inject:viewAddress(env, box)];
    [pool release];
}
JNIEXPORT void JNICALL Java_Checkbox115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
JNIEXPORT jint JNICALL Java_Checkbox115_liveButtons(JNIEnv *env, jclass cls) { return buttons; }
JNIEXPORT jboolean JNICALL Java_Checkbox115_state(JNIEnv *env, jclass cls, jobject box, jstring title, jboolean selected)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[CheckboxProbe appkitProxy] state:viewAddress(env, box) title:javaToNSString(env, title) selected:selected];
    [pool release]; return result;
}
