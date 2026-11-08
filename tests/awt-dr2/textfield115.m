#import "../../src/rhapsody/sun/awt/awt_TextField.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
static volatile int fields, editors;
static int failConstruction;
@interface TextFieldLifetimeProbe : AwtTextField
@end
@implementation TextFieldLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) fields++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable echoChar:(unsigned short)echo
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor text:text editable:editable echoChar:echo];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"text field construction failure"];
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
@interface TextFieldProbe : NSObject
+ (void)track;
+ (void)inject:(int)address character:(int)character;
+ (BOOL)state:(int)address editable:(BOOL)editable;
@end
@implementation TextFieldProbe
+ (void)track
{
    [TextFieldLifetimeProbe poseAsClass:[AwtTextField class]];
    [TextEditorLifetimeProbe poseAsClass:[AwtTextView class]];
}
+ (void)inject:(int)address character:(int)character
{
    NSTextField *view = (NSTextField *)address;
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
+ (BOOL)state:(int)address editable:(BOOL)editable
{
    NSTextField *view = (NSTextField *)address;
    return [view isEditable] == editable && [view isSelectable] == editable &&
        ![view isBordered] && [view isBezeled] && ![[view cell] wraps] && [[view cell] isScrollable];
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
JNIEXPORT void JNICALL Java_TextField115_inject(JNIEnv *env, jclass cls, jobject field, jint character)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[TextFieldProbe appkitProxy] inject:viewAddress(env, field) character:character];
    [pool release];
}
JNIEXPORT jboolean JNICALL Java_TextField115_state(JNIEnv *env, jclass cls, jobject field, jboolean editable)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[TextFieldProbe appkitProxy] state:viewAddress(env, field) editable:editable];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_TextField115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[TextFieldProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT jint JNICALL Java_TextField115_liveFields(JNIEnv *env, jclass cls) { return fields; }
JNIEXPORT jint JNICALL Java_TextField115_liveEditors(JNIEnv *env, jclass cls) { return editors; }
JNIEXPORT void JNICALL Java_TextField115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
