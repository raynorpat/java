#import "../../src/rhapsody/sun/awt/awt_Application.h"
#import "../../src/rhapsody/sun/awt/awt_Conversions.h"
#import "../../src/rhapsody/sun/awt/awt_TextField.h"
@interface DialogProbe : NSObject
+ (BOOL)stateForView:(int)address title:(NSString *)title style:(int)style;
+ (BOOL)textFocused:(int)address;
+ (BOOL)modalWindow:(int)address;
+ (void)clickDialogButton:(int)address;
@end
@implementation DialogProbe
+ (BOOL)stateForView:(int)address title:(NSString *)title style:(int)style
{ NSWindow *window = [(NSView *)address window]; return [[window title] isEqual:title] && [window styleMask] == style; }
+ (BOOL)textFocused:(int)address
{ AwtTextField *field = (AwtTextField *)address; return [[field window] firstResponder] == [field getFieldEditor]; }
+ (BOOL)modalWindow:(int)address { return [NSApp modalWindow] == [(NSView *)address window]; }
+ (void)clickDialogButton:(int)address
{
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    NSPoint point = [view convertPoint:NSMakePoint(10, 10) toView:nil];
    int index, types[] = {NSLeftMouseDown, NSLeftMouseUp};
    for (index = 0; index < 2; index++)
        [NSApp postEvent:[NSEvent mouseEventWithType:types[index] location:point modifierFlags:0
            timestamp:1.0 + index windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:index + 1 clickCount:1 pressure:0.5] atStart:NO];
}
@end
static int viewAddress(JNIEnv *env, jobject peer)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, cls, "nNativeView", "I");
    return (*env)->GetIntField(env, peer, field);
}
JNIEXPORT jboolean JNICALL Java_Dialog115_nativeState(JNIEnv *env, jclass cls, jobject peer, jstring title, jint style)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[DialogProbe appkitProxy] stateForView:viewAddress(env, peer) title:javaToNSString(env, title) style:style];
    [pool release]; return result;
}
JNIEXPORT jboolean JNICALL Java_Dialog115_textFocused(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; BOOL result = [[DialogProbe appkitProxy] textFocused:viewAddress(env, peer)]; [pool release]; return result; }
JNIEXPORT jboolean JNICALL Java_Dialog115_modalWindow(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; BOOL result = [[DialogProbe appkitProxy] modalWindow:viewAddress(env, peer)]; [pool release]; return result; }

JNIEXPORT void JNICALL Java_Dialog115_clickButton(JNIEnv *env, jclass cls, jobject peer)
{ id pool = [[NSAutoreleasePool alloc] init]; [[DialogProbe appkitProxy] clickDialogButton:viewAddress(env, peer)]; [pool release]; }
