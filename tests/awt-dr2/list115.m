#include "jni_impl.h"
#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_ScrollingList.h"
@interface AwtScrollingList (CallbackTest)
- (void)doubleClick:(id)sender;
@end
static volatile int lists, tables;
static int failConstruction;
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
@interface TableLifetimeProbe : AwtTableView
@end
@implementation TableLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) tables++; return result; }
- (void)dealloc { tables--; [super dealloc]; }
@end
@interface ListLifetimeProbe : AwtScrollingList
@end
@implementation ListLifetimeProbe
- (void)doubleClick:(id)sender
{
    int before = callbackRefs();
    [super doubleClick:sender];
    retainedCallbackRefs += callbackRefs() - before;
}
- (void)updateForNewSelection
{
    int before = callbackRefs();
    [super updateForNewSelection];
    retainedCallbackRefs += callbackRefs() - before;
}
+ (id)allocWithZone:(NSZone *)zone { id result = [super allocWithZone:zone]; if (result) lists++; return result; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor allowMultipleSelection:(BOOL)multiple
{
    self = [super initWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor allowMultipleSelection:multiple];
    if (failConstruction) [NSException raise:NSInvalidArgumentException format:@"list construction failure"];
    return self;
}
- (void)dealloc { lists--; [super dealloc]; }
@end
@interface ListProbe : NSObject
+ (int)failedDispatch:(int)address;
+ (int)clickedRow;
+ (void)track;
+ (int)live;
+ (void)click:(int)address row:(int)row count:(int)count;
@end
@implementation ListProbe
+ (int)clickedRow { return 1; }
+ (int)failedDispatch:(int)address
{
    AwtScrollingList *list = (AwtScrollingList *)address;
    NSTableView *table = [list documentView];
    int before = callbackRefs(), kind, caught = 0;
    for (kind = 0; kind < 2; kind++) {
        failEventDispatch = YES;
        NS_DURING
            if (kind == 0) [list doubleClick:self];
            else {
                [table setDelegate:nil]; [table selectRow:0 byExtendingSelection:NO];
                [table setDelegate:list]; [list updateForNewSelection];
            }
        NS_HANDLER
            if ([[localException reason] isEqual:@"callback dispatch failure"]) caught++;
        NS_ENDHANDLER
        failEventDispatch = NO;
    }
    return caught == 2 ? callbackRefs() - before : -1;
}
+ (int)live { return lists + tables; }
+ (void)track { hookEventDispatch(); [ListLifetimeProbe poseAsClass:[AwtScrollingList class]]; [TableLifetimeProbe poseAsClass:[AwtTableView class]]; }
+ (void)click:(int)address row:(int)row count:(int)count
{
    NSScrollView *list = (NSScrollView *)address;
    NSTableView *view = [list documentView];
    NSWindow *window = [view window];
    NSRect rect = [view rectOfRow:row];
    NSPoint point = [view convertPoint:NSMakePoint(NSMidX(rect), NSMidY(rect)) toView:nil];
    int index, types[] = {NSLeftMouseDown, NSLeftMouseUp};
    [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil];
    for (index = 0; index < 2; index++)
        [NSApp postEvent:[NSEvent mouseEventWithType:types[index] location:point modifierFlags:0
            timestamp:1 + index windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:index + 1 clickCount:count pressure:0.5] atStart:NO];
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
JNIEXPORT void JNICALL Java_List115_click(JNIEnv *env, jclass cls, jobject list, jint row, jint count)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [[ListProbe appkitProxy] click:viewAddress(env, list) row:row count:count];
    [pool release];
}
JNIEXPORT void JNICALL Java_List115_track(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init]; [[ListProbe appkitProxy] track]; [pool release];
}
JNIEXPORT jint JNICALL Java_List115_liveLists(JNIEnv *env, jclass cls) {
    id pool = [[NSAutoreleasePool alloc] init];
    /* Flush pending DO wire releases before counting native controls. */
    int result = [[ListProbe appkitProxy] live];
    [pool release]; return result;
}
JNIEXPORT void JNICALL Java_List115_failConstruction(JNIEnv *env, jclass cls, jboolean fail) { failConstruction = fail; }

JNIEXPORT jint JNICALL Java_List115_retainedCallbackRefs(JNIEnv *env, jclass cls)
{ return retainedCallbackRefs; }

JNIEXPORT jint JNICALL Java_List115_failedDispatch(JNIEnv *env, jclass cls, jobject list)
{
    id pool = [[NSAutoreleasePool alloc] init];
    int result = [[ListProbe appkitProxy] failedDispatch:viewAddress(env, list)];
    [pool release]; return result;
}
