#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (InputTest)
+ (id)appkitProxy;
- (void)awtSetFrame:(NSRect)frame display:(BOOL)display;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (void)setLastJavaViewEntered:(id)view;
@end
@interface InputProbe : NSObject
+ (void)injectView:(int)address;
+ (void)hoverView:(int)address step:(int)step;
+ (void)focusView:(int)address temporary:(BOOL)temporary;
@end
@implementation InputProbe
+ (void)hoverView:(int)address step:(int)step
{
    static NSRect original;
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    NSPoint mouse = [window convertBaseToScreen:[window mouseLocationOutsideOfEventStream]];
    NSRect frame = [window frame];
    NSEvent *event;
    if (step == 0) {
        original = frame;
        [view setLastJavaViewEntered:nil];
        [view setWantsMouseMotionEvents:NO];
        [window setAcceptsMouseMovedEvents:NO];
    }
    frame.origin = NSMakePoint(mouse.x - (step == 2 ? -30 : 30), mouse.y - frame.size.height + 50);
    [window awtSetFrame:frame display:NO];
    if (step == 1) {
        event = [NSEvent mouseEventWithType:NSMouseMoved location:[window mouseLocationOutsideOfEventStream]
            modifierFlags:0 timestamp:1 windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:1 clickCount:0 pressure:0];
        [NSApp postEvent:event atStart:NO];
    } else {
        /* Deliberately stale location: the bridge must query the current pointer. */
        event = [NSEvent enterExitEventWithType:NSMouseEntered location:NSMakePoint(-999, -999)
            modifierFlags:0 timestamp:1 windowNumber:[window windowNumber] context:[NSApp context]
            eventNumber:1 trackingNumber:1 userData:NULL];
        [view mouseEntered:event];
    }
    if (step == 2) [window awtSetFrame:original display:NO];
}
+ (void)focusView:(int)address temporary:(BOOL)temporary
{
    NSView *view = (NSView *)address;
    if (temporary) {
        [[view window] resignKeyWindow];
        [[view window] becomeKeyWindow];
    } else {
        [view resignFirstResponder];
        [view becomeFirstResponder];
    }
}
+ (void)injectView:(int)address
{
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    NSPoint point = [view convertPoint:NSMakePoint(30, 50) toView:nil];
    int index, types[] = {NSLeftMouseDown, NSLeftMouseDragged, NSLeftMouseUp};
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
    for (index = 0; index < 3; index++) {
        NSEvent *event = [NSEvent mouseEventWithType:types[index] location:point
            modifierFlags:NSShiftKeyMask | NSControlKeyMask timestamp:1.0 + index
            windowNumber:[window windowNumber] context:[NSApp context] eventNumber:index + 1 clickCount:1 pressure:0.5];
        [NSApp postEvent:event atStart:NO];
    }
}
@end
JNIEXPORT void JNICALL Java_Input115_focus(JNIEnv *env, jclass cls, jobject frame, jboolean temporary)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[InputProbe appkitProxy] focusView:(*env)->GetIntField(env, peer, field) temporary:temporary];
    [pool release];
}
JNIEXPORT void JNICALL Java_Input115_inject(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[InputProbe appkitProxy] injectView:(*env)->GetIntField(env, peer, field)];
    [pool release];
}

JNIEXPORT void JNICALL Java_Input115_hover(JNIEnv *env, jclass cls, jobject frame, jint step)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[InputProbe appkitProxy] hoverView:(*env)->GetIntField(env, peer, field) step:step];
    [pool release];
}
