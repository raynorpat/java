#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_PopUpButton.h"
static volatile int choices;
static int failConstruction;
@interface ChoiceLifetimeProbe : AwtPopUpButton
@end
@implementation ChoiceLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) choices++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"choice construction failure"];
    return self;
}
- (void)dealloc { choices--; [super dealloc]; }
@end
@interface ChoiceProbe : NSObject
+ (void)track;
+ (BOOL)state:(int)address items:(NSArray *)items selection:(int)selection;
+ (void)choose:(int)address index:(int)index;
+ (void)chooseQueued:(int)address;
@end
@implementation ChoiceProbe
+ (void)track { [ChoiceLifetimeProbe poseAsClass:[AwtPopUpButton class]]; }
+ (BOOL)state:(int)address items:(NSArray *)items selection:(int)selection
{
    NSPopUpButton *view = (NSPopUpButton *)address;
    int index;
    if ([view numberOfItems] != [items count] || [view indexOfSelectedItem] != selection) return NO;
    for (index = 0; index < [items count]; index++)
        if (![[[view itemAtIndex:index] title] isEqual:[items objectAtIndex:index]]) return NO;
    return YES;
}
+ (void)choose:(int)address index:(int)index
{
    NSPopUpButton *view = (NSPopUpButton *)address;
    id pool = [[NSAutoreleasePool alloc] init];
    [view selectItemAtIndex:index];
    [view sendAction:[view action] to:[view target]];
    [pool release];
}
+ (void)chooseQueued:(int)address
{
    NSPopUpButton *view = (NSPopUpButton *)address;
    NSWindow *window = [view window];
    unichar keys[] = {' ', NSDownArrowFunctionKey, 13};
    int index, type;
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
    for (index = 0; index < 3; index++) {
        NSString *text = [NSString stringWithCharacters:&keys[index] length:1];
        for (type = NSKeyDown; type <= NSKeyUp; type++)
            [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0)
                modifierFlags:0 timestamp:1 windowNumber:[window windowNumber]
                context:[NSApp context] characters:text charactersIgnoringModifiers:text
                isARepeat:NO keyCode:0] atStart:NO];
    }
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
JNIEXPORT jboolean JNICALL Java_Choice115_state(JNIEnv *env, jclass cls, jobject choice, jobjectArray items, jint selection)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSMutableArray *strings = [NSMutableArray array];
    int index, count = (*env)->GetArrayLength(env, items);
    BOOL result;
    for (index = 0; index < count; index++) {
        jstring item = (*env)->GetObjectArrayElement(env, items, index);
        [strings addObject:javaToNSString(env, item)];
        (*env)->DeleteLocalRef(env, item);
    }
    result = [[ChoiceProbe appkitProxy] state:viewAddress(env, choice) items:strings selection:selection];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Choice115_choose(JNIEnv *env, jclass cls, jobject choice, jint index)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ChoiceProbe appkitProxy] choose:viewAddress(env, choice) index:index];
    [pool release];
}
JNIEXPORT void JNICALL Java_Choice115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ChoiceProbe appkitProxy] track];
    [pool release];
}
JNIEXPORT void JNICALL Java_Choice115_chooseQueued(JNIEnv *env, jclass cls, jobject choice)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ChoiceProbe appkitProxy] chooseQueued:viewAddress(env, choice)];
    [pool release];
}
JNIEXPORT jint JNICALL Java_Choice115_liveChoices(JNIEnv *env, jclass cls) { return choices; }
JNIEXPORT void JNICALL Java_Choice115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }
