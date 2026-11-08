#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (InputTest)
+ (id)appkitProxy;
@end
@interface InputProbe : NSObject
+ (void)injectView:(int)address;
@end
@implementation InputProbe
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
