#import "../../src/rhapsody/sun/awt/awt_ScrollingText.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
static volatile int fields, editors;
static int failConstruction;
@interface TextAreaLifetimeProbe : AwtScrollingText
@end
@implementation TextAreaLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) fields++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable wantsScrollers:(int)scrollers selection:(NSRange)selection
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor text:text editable:editable wantsScrollers:scrollers selection:selection];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"text area construction failure"];
    return self;
}
- (void)dealloc { fields--; [super dealloc]; }
@end
@interface TextEditorLifetimeProbe : AwtTextView
@end
@implementation TextEditorLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) editors++; return result; }
- (void)dealloc { editors--; [super dealloc]; }
@end
@interface TextAreaProbe : NSObject
+ (void)track;
+ (void)inject:(int)address character:(int)character;
+ (BOOL)state:(int)address editable:(BOOL)editable mode:(int)mode;
@end
@implementation TextAreaProbe
+ (void)track
{
    [TextAreaLifetimeProbe poseAsClass:[AwtScrollingText class]];
    [TextEditorLifetimeProbe poseAsClass:[AwtTextView class]];
}
+ (void)inject:(int)address character:(int)character
{
    NSScrollView *view = (NSScrollView *)address;
    NSWindow *window = [view window];
    unichar ch = character;
    NSString *text = [NSString stringWithCharacters:&ch length:1];
    int type;
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
    for (type = NSKeyDown; type <= NSKeyUp; type++)
        [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0)
            modifierFlags:0 timestamp:1 windowNumber:[window windowNumber]
            context:[NSApp context] characters:text charactersIgnoringModifiers:text
            isARepeat:NO keyCode:0] atStart:NO];
}
+ (BOOL)state:(int)address editable:(BOOL)editable mode:(int)mode
{
    NSScrollView *view = (NSScrollView *)address;
    NSTextView *text = [view documentView];
    return [text isEditable] == editable && [text isSelectable] == editable &&
        [view hasHorizontalScroller] == (mode == 0 || mode == 2) &&
        [view hasVerticalScroller] == (mode == 0 || mode == 1) && [view borderType] == NSBezelBorder;
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
JNIEXPORT void JNICALL Java_TextArea115_inject(JNIEnv *env, jclass cls, jobject field, jint character)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[TextAreaProbe appkitProxy] inject:viewAddress(env, field) character:character];
    [pool release];
}
JNIEXPORT jboolean JNICALL Java_TextArea115_state(JNIEnv *env, jclass cls, jobject field, jboolean editable, jint mode)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[TextAreaProbe appkitProxy] state:viewAddress(env, field) editable:editable mode:mode];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_TextArea115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[TextAreaProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT jint JNICALL Java_TextArea115_liveFields(JNIEnv *env, jclass cls) { return fields; }
JNIEXPORT jint JNICALL Java_TextArea115_liveEditors(JNIEnv *env, jclass cls) { return editors; }
JNIEXPORT void JNICALL Java_TextArea115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
