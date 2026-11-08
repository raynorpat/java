#include "jni_impl.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_Menu.h"
@interface AwtMenuItem (CallbackTest)
- (void)checkboxMenuItemSelected:(id)sender;
@end
static volatile int components;
static int failKind;
/* Count live JNI handles on the persistent AppKit native frame. */
static int retainedCallbackRefs;
static int callbackRefs(void)
{
    JNIRefTable *table = JNIEnvGetLocalRefs([NSApp appkitEnv]);
    int index, count = 0;
    for (index = 0; index < table->top; index++)
        count += table->elements[index].refCount & JNI_REF_COUNT_MASK;
    return count;
}
#import <objc/objc-class.h>
static BOOL failEventDispatch;
static IMP originalEventDispatch;
static BOOL dispatchFailure(id application, SEL selector, jobject event, BOOL local)
{
    if (failEventDispatch)
        [NSException raise:NSInvalidArgumentException format:@"callback dispatch failure"];
    return ((BOOL (*)(id, SEL, jobject, BOOL))originalEventDispatch)(application, selector, event, local);
}
static void hookEventDispatch(void)
{
    struct objc_method *method = class_getInstanceMethod([NSApp class], @selector(dispatchJavaEvent:isLocalRef:));
    originalEventDispatch = method->method_imp;
    method->method_imp = (IMP)dispatchFailure;
}
@interface MenuLifetimeProbe : AwtMenu
@end
@implementation MenuLifetimeProbe
- (id)initWithPeer:(int)peer withLabel:(NSString *)label tearOff:(BOOL)tearOff isEnabled:(BOOL)enabled isHelpMenu:(BOOL)help withFont:(NSFont *)font inMenuComponent:(id)parent
{
    self = [super initWithPeer:peer withLabel:label tearOff:tearOff isEnabled:enabled isHelpMenu:help withFont:font inMenuComponent:parent];
    if (failKind == 2) [NSException raise:NSInvalidArgumentException format:@"menu construction failure"];
    return self;
}
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) components++; return value; }
- (void)dealloc { components--; [super dealloc]; }
@end
@interface MenuBarLifetimeProbe : AwtMenuBar
@end
@implementation MenuBarLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) components++; return value; }
- (void)dealloc { components--; [super dealloc]; }
@end
@interface MenuItemLifetimeProbe : AwtMenuItem
@end
@implementation MenuItemLifetimeProbe
- (void)checkboxMenuItemSelected:(id)sender
{
    int before = callbackRefs();
    [super checkboxMenuItemSelected:sender];
    retainedCallbackRefs += callbackRefs() - before;
}
- (id)commonInitWithPeer:(int)peer withLabel:(NSString *)label isEnabled:(BOOL)enabled withShortcut:(NSString *)shortcut usesShiftModifier:(BOOL)shift action:(SEL)action withFont:(NSFont *)font inMenu:(AwtMenu *)menu
{
    self = [super commonInitWithPeer:peer withLabel:label isEnabled:enabled withShortcut:shortcut usesShiftModifier:shift action:action withFont:font inMenu:menu];
    if (failKind == 1) [NSException raise:NSInvalidArgumentException format:@"menu construction failure"];
    return self;
}
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) components++; return value; }
- (void)dealloc { components--; [super dealloc]; }
@end
@interface MenuProbe : NSObject
+ (int)failedDispatch:(int)address;
+ (void)track;
+ (int)live;
+ (BOOL)state:(int)address label:(NSString *)label enabled:(BOOL)enabled state:(int)state;
+ (BOOL)hierarchy:(int)bar file:(int)file nested:(int)nested help:(int)help;
+ (void)choose:(int)address;
+ (void)shortcut:(int)frame;
+ (BOOL)active:(int)bar;
+ (void)activateFrameForMenuTest:(int)frame;
+ (void)popupKeys:(int)frame;
+ (void)postPopupKeys:(NSTimer *)timer;
@end
@implementation MenuProbe
+ (int)failedDispatch:(int)address
{
    AwtMenuItem *item = (AwtMenuItem *)address;
    int before = callbackRefs(), state = [item state];
    BOOL volatile caught = NO;
    failEventDispatch = YES;
    NS_DURING
        [item checkboxMenuItemSelected:item];
    NS_HANDLER
        caught = [[localException reason] isEqual:@"callback dispatch failure"];
    NS_ENDHANDLER
    failEventDispatch = NO; [item setState:state];
    return caught ? callbackRefs() - before : -1;
}
+ (void)track
{
    hookEventDispatch();
    [MenuLifetimeProbe poseAsClass:[AwtMenu class]];
    [MenuBarLifetimeProbe poseAsClass:[AwtMenuBar class]];
    [MenuItemLifetimeProbe poseAsClass:[AwtMenuItem class]];
}
+ (int)live
{
    /* Synthetic callbacks can leave AppKit waiting inside nextEventMatchingMask.
     * Advance the outer event loop before recounting deferred releases. */
    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
        modifierFlags:0 timestamp:1 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
    return components;
}
+ (BOOL)state:(int)address label:(NSString *)label enabled:(BOOL)enabled state:(int)state
{
    NSMenuItem *item = (NSMenuItem *)address;
    return [[item title] isEqual:label] && [item isEnabled] == enabled && (state < 0 || [item state] == state);
}
+ (BOOL)hierarchy:(int)bar file:(int)file nested:(int)nested help:(int)help
{
    NSMenu *nativeBar = (NSMenu *)bar, *nativeFile = (NSMenu *)file;
    return [nativeBar indexOfItemWithSubmenu:(NSMenu *)file] >= 0 &&
        [nativeBar indexOfItemWithSubmenu:(NSMenu *)help] == [nativeBar numberOfItems] - 1 &&
        [nativeFile numberOfItems] == 4 && [[nativeFile itemAtIndex:1] isSeparatorItem] &&
        [[nativeFile itemAtIndex:3] submenu] == (NSMenu *)nested &&
        [(NSMenu *)nested numberOfItems] == 2;
}
+ (void)choose:(int)address
{
    NSMenuItem *item = (NSMenuItem *)address;
    [[item menu] performActionForItemAtIndex:[[item menu] indexOfItem:item]];
}
+ (void)shortcut:(int)frame
{
    NSWindow *window = [(NSView *)frame window];
    int type;
    [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil];
    for (type = NSKeyDown; type <= NSKeyUp; type++)
        [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0)
            modifierFlags:NSCommandKeyMask | NSShiftKeyMask timestamp:1 windowNumber:[window windowNumber]
            context:[NSApp context] characters:@"B" charactersIgnoringModifiers:@"b"
            isARepeat:NO keyCode:0] atStart:NO];
}
+ (BOOL)active:(int)bar { return [NSApp mainMenu] == (NSMenu *)bar; }
+ (void)activateFrameForMenuTest:(int)frame { [[(NSView *)frame window] makeKeyAndOrderFront:nil]; }
+ (void)popupKeys:(int)frame
{
    NSWindow *window = [(NSView *)frame window];
    NSEvent *event;
    [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil];
    event = [NSEvent mouseEventWithType:NSLeftMouseUp
        location:[(NSView *)frame convertPoint:NSMakePoint(40, 70) toView:nil] modifierFlags:0
        timestamp:1 windowNumber:[window windowNumber] context:nil eventNumber:1 clickCount:1 pressure:0];
    [[NSRunLoop currentRunLoop] addTimer:[NSTimer timerWithTimeInterval:0.3
        target:self selector:@selector(postPopupKeys:) userInfo:event repeats:NO]
        forMode:NSEventTrackingRunLoopMode];
}
+ (void)postPopupKeys:(NSTimer *)timer
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSEvent *event = [timer userInfo];
    unichar keys[] = {NSDownArrowFunctionKey, 13};
    int index, type;
    [NSApp postEvent:event atStart:NO];
    for (index = 0; index < 2; index++) {
        NSString *text = [NSString stringWithCharacters:&keys[index] length:1];
        for (type = NSKeyDown; type <= NSKeyUp; type++)
            [NSApp postEvent:[NSEvent keyEventWithType:type location:NSMakePoint(0, 0)
                modifierFlags:0 timestamp:1 windowNumber:[event windowNumber] context:nil
                characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:0] atStart:NO];
    }
    [pool release];
}

@end
static int menuAddress(JNIEnv *env, jobject component)
{
    jclass cls = (*env)->FindClass(env, "java/awt/MenuComponent");
    jmethodID method = (*env)->GetMethodID(env, cls, "getPeer", "()Ljava/awt/peer/MenuComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, component, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RMenuComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeMenuComponent", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT void JNICALL Java_Menu115_track(JNIEnv *env, jclass cls)
{ id pool = [[NSAutoreleasePool alloc] init]; [[MenuProbe appkitProxy] track]; [pool release]; }
JNIEXPORT jint JNICALL Java_Menu115_live(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init]; int result = [[MenuProbe appkitProxy] live];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_Menu115_state(JNIEnv *env, jclass cls, jobject item, jstring label, jboolean enabled, jint state)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[MenuProbe appkitProxy] state:menuAddress(env, item) label:javaToNSString(env, label) enabled:enabled state:state];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_Menu115_hierarchy(JNIEnv *env, jclass cls, jobject bar, jobject file, jobject nested, jobject help)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[MenuProbe appkitProxy] hierarchy:menuAddress(env, bar) file:menuAddress(env, file)
        nested:menuAddress(env, nested) help:menuAddress(env, help)];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_Menu115_choose(JNIEnv *env, jclass cls, jobject item)
{ id pool = [[NSAutoreleasePool alloc] init]; [[MenuProbe appkitProxy] choose:menuAddress(env, item)]; [pool release]; }
JNIEXPORT void JNICALL Java_Menu115_shortcut(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[MenuProbe appkitProxy] shortcut:(*env)->GetIntField(env, peer, field)];
    [pool release];
}

JNIEXPORT void JNICALL Java_Menu115_failConstruction(JNIEnv *env, jclass cls, jint kind) { failKind = kind; }

JNIEXPORT void JNICALL Java_Menu115_popupKeys(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[MenuProbe appkitProxy] popupKeys:(*env)->GetIntField(env, peer, field)];
    [pool release];
}

JNIEXPORT jboolean JNICALL Java_Menu115_active(JNIEnv *env, jclass cls, jobject bar)
{ id pool = [[NSAutoreleasePool alloc] init]; BOOL result = [[MenuProbe appkitProxy] active:menuAddress(env, bar)]; [pool release]; return result; }
JNIEXPORT void JNICALL Java_Menu115_activate(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[MenuProbe appkitProxy] activateFrameForMenuTest:(*env)->GetIntField(env, peer, field)];
    [pool release];
}

JNIEXPORT jint JNICALL Java_Menu115_retainedCallbackRefs(JNIEnv *env, jclass cls)
{ return retainedCallbackRefs; }

JNIEXPORT jint JNICALL Java_Menu115_failedDispatch(JNIEnv *env, jclass cls, jobject item)
{
    id pool = [[NSAutoreleasePool alloc] init];
    int result = [[MenuProbe appkitProxy] failedDispatch:menuAddress(env, item)];
    [pool release]; return result;
}
