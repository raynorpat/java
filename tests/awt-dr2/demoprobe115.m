#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (DemoProbeDispatch)
+ (id)appkitProxy;
- (JNIEnv *)appkitEnv;
- (jobject)peer;
@end
@interface DemoProbe : NSObject
+ (int)windowForView:(int)view;
+ (int)windows;
+ (void)window:(int)number action:(NSString *)action x:(int)x y:(int)y;
@end
@implementation DemoProbe
+ (int)windowForView:(int)view { return [[(NSView *)view window] windowNumber]; }
+ (int)windows
{
    JNIEnv *env = [NSApp appkitEnv];
    NSArray *windows = [NSApp windows];
    unsigned int i, count = 0;
    jclass type = (*env)->FindClass(env, "java/awt/Window");
    jclass peerType = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerType, "fJavaCounterpart", "Ljava/awt/Component;");
    jobjectArray result;
    jobject global;
    for (i = 0; i < [windows count]; i++) {
        NSWindow *window = [windows objectAtIndex:i];
        if ([window isVisible] && [window respondsToSelector:@selector(peer)] && [window peer]) count++;
    }
    result = (*env)->NewObjectArray(env, count, type, NULL);
    count = 0;
    for (i = 0; i < [windows count]; i++) {
        NSWindow *window = [windows objectAtIndex:i];
        if ([window isVisible] && [window respondsToSelector:@selector(peer)] && [window peer]) {
            jobject target = (*env)->GetObjectField(env, [window peer], field);
            (*env)->SetObjectArrayElement(env, result, count++, target);
            (*env)->DeleteLocalRef(env, target);
        }
    }
    global = (*env)->NewGlobalRef(env, result);
    (*env)->DeleteLocalRef(env, result);
    (*env)->DeleteLocalRef(env, type); (*env)->DeleteLocalRef(env, peerType);
    return (int)global;
}
+ (void)window:(int)number action:(NSString *)action x:(int)x y:(int)y
{
    NSWindow *window = [NSApp windowWithWindowNumber:number];
    NSPoint point;
    NSRect screen = [[NSScreen mainScreen] visibleFrame];
    NSEventType kind;
    if (!window) return;
    if ([action isEqual:@"activate"]) {
        [NSApp activateIgnoringOtherApps:YES]; [window makeKeyAndOrderFront:nil]; return;
    }
    if ([action isEqual:@"close"]) { [window performClose:nil]; return; }
    if ([action isEqual:@"resize"]) {
        NSRect frame = [window frame]; frame.size.width += x; frame.size.height += y;
        frame.origin.y -= y; /* Keep the top edge fixed, as a WM corner drag does. */
        [window setFrame:frame display:YES]; return;
    }
    point = [window convertScreenToBase:NSMakePoint(x, screen.size.height - y)];
    if ([action isEqual:@"click"] || [action isEqual:@"draggesture"]) {
        NSPoint start = point;
        BOOL drag = [action isEqual:@"draggesture"];
        if (drag) { start.x -= 30; start.y += 20; }
        [NSApp postEvent:[NSEvent mouseEventWithType:NSLeftMouseDown location:start modifierFlags:0 timestamp:1
            windowNumber:number context:[NSApp context] eventNumber:1 clickCount:1 pressure:1] atStart:NO];
        if (drag) [NSApp postEvent:[NSEvent mouseEventWithType:NSLeftMouseDragged location:point modifierFlags:0 timestamp:2
            windowNumber:number context:[NSApp context] eventNumber:2 clickCount:1 pressure:1] atStart:NO];
        [NSApp postEvent:[NSEvent mouseEventWithType:NSLeftMouseUp location:point modifierFlags:0 timestamp:3
            windowNumber:number context:[NSApp context] eventNumber:3 clickCount:1 pressure:0] atStart:NO];
        return;
    }
    kind = [action isEqual:@"down"] ? NSLeftMouseDown :
        [action isEqual:@"up"] ? NSLeftMouseUp : [action isEqual:@"drag"] ? NSLeftMouseDragged : NSMouseMoved;
    [NSApp postEvent:[NSEvent mouseEventWithType:kind location:point modifierFlags:0 timestamp:[NSDate timeIntervalSinceReferenceDate]
        windowNumber:number context:[NSApp context] eventNumber:1 clickCount:1 pressure:1] atStart:NO];
}
@end
JNIEXPORT jobjectArray JNICALL Java_sun_awt_DR2DemoObserver115_windows(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobjectArray global = (jobjectArray)[[DemoProbe appkitProxy] windows];
    jclass type = (*env)->FindClass(env, "java/awt/Window");
    jsize count = (*env)->GetArrayLength(env, global), i;
    jobjectArray result = (*env)->NewObjectArray(env, count, type, NULL);
    for (i = 0; i < count; i++) {
        jobject window = (*env)->GetObjectArrayElement(env, global, i);
        (*env)->SetObjectArrayElement(env, result, i, window);
        (*env)->DeleteLocalRef(env, window);
    }
    (*env)->DeleteGlobalRef(env, global); [pool release]; return result;
}
JNIEXPORT void JNICALL Java_sun_awt_DR2DemoObserver115_command(JNIEnv *env, jclass cls, jint window, jstring action, jint x, jint y)
{
    id pool = [[NSAutoreleasePool alloc] init];
    const char *bytes = (*env)->GetStringUTFChars(env, action, NULL);
    NSString *name = [NSString stringWithCString:bytes];
    (*env)->ReleaseStringUTFChars(env, action, bytes);
    [[DemoProbe appkitProxy] window:window action:name x:x y:y];
    [pool release];
}
JNIEXPORT jint JNICALL Java_sun_awt_DemoObserver115_windowId(JNIEnv *env, jclass cls, jobject window)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, window, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    int result = [[DemoProbe appkitProxy] windowForView:(*env)->GetIntField(env, peer, field)];
    [pool release];
    return result;
}
