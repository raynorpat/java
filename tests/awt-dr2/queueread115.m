#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (QueueReadTest)
+ (id)appkitProxy;
- (JNIEnv *)appkitEnv;
@end
@interface QueueReadProbe : NSObject
+ (int)exerciseQueue:(int)queue view:(int)address;
+ (void)activateView:(int)address;
+ (void)postKeyView:(int)address;
@end
@implementation QueueReadProbe
+ (void)postKeyView:(int)address
{
    NSWindow *window = [(NSView *)address window];
    NSEvent *key = [NSEvent keyEventWithType:NSKeyDown location:NSMakePoint(0, 0) modifierFlags:0 timestamp:1
        windowNumber:[window windowNumber] context:[NSApp context] characters:@"a" charactersIgnoringModifiers:@"a" isARepeat:NO keyCode:0];
    [NSApp postEvent:key atStart:YES];
}
+ (void)activateView:(int)address
{
    NSView *view = (NSView *)address;
    [NSApp activateIgnoringOtherApps:YES]; [[view window] makeKeyAndOrderFront:nil]; [[view window] makeFirstResponder:view];
}
+ (int)exerciseQueue:(int)queue view:(int)address
{
    JNIEnv *env = [NSApp appkitEnv];
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    jclass cls = (*env)->GetObjectClass(env, (jobject)queue);
    jmethodID peek = (*env)->GetMethodID(env, cls, "peekEvent", "()Ljava/awt/AWTEvent;");
    jmethodID filtered = (*env)->GetMethodID(env, cls, "peekEvent", "(I)Ljava/awt/AWTEvent;");
    jmethodID get = (*env)->GetMethodID(env, cls, "getNextEvent", "()Ljava/awt/AWTEvent;");
    jclass eventClass = (*env)->FindClass(env, "java/awt/AWTEvent");
    jobjectArray result = (*env)->NewObjectArray(env, 5, eventClass, NULL);
    jobject event;
    int index;
    (*env)->DeleteLocalRef(env, eventClass);
    if ([NSApp keyWindow] != window) {
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "queue test window is not key");
        return 0;
    }
    [self postKeyView:address];
    for (index = 0; index < 5; index++) {
        if (index == 0) event = (*env)->CallObjectMethod(env, (jobject)queue, peek);
        else if (index == 4) event = (*env)->CallObjectMethod(env, (jobject)queue, get);
        else event = (*env)->CallObjectMethod(env, (jobject)queue, filtered, index == 1 ? 401 : index == 2 ? 402 : 400);
        if ((*env)->ExceptionOccurred(env)) { (*env)->ExceptionDescribe(env); return 0; }
        (*env)->SetObjectArrayElement(env, result, index, event);
        (*env)->DeleteLocalRef(env, event);
    }
    event = (*env)->NewGlobalRef(env, result);
    (*env)->DeleteLocalRef(env, result); (*env)->DeleteLocalRef(env, cls);
    return (int)event;
}
@end
JNIEXPORT jobjectArray JNICALL Java_QueueRead115_exercise(JNIEnv *env, jclass cls, jobject queue, jobject frame, jint mode)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    jobject globalQueue, globalResult;
    jobjectArray root, result;
    if (mode == 1) {
        [[QueueReadProbe appkitProxy] activateView:(*env)->GetIntField(env, peer, field)];
        [pool release]; return NULL;
    }
    if (mode == 2) {
        [[QueueReadProbe appkitProxy] postKeyView:(*env)->GetIntField(env, peer, field)];
        [pool release]; return NULL;
    }
    globalQueue = (*env)->NewGlobalRef(env, queue);
    globalResult = (jobject)[[QueueReadProbe appkitProxy] exerciseQueue:(int)globalQueue view:(*env)->GetIntField(env, peer, field)];
    root = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/Object"), globalResult);
    result = (jobjectArray)(*env)->GetObjectArrayElement(env, root, 0);
    (*env)->DeleteGlobalRef(env, globalQueue); (*env)->DeleteGlobalRef(env, globalResult);
    [pool release]; return result;
}
