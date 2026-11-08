#import <AppKit/AppKit.h>
#include "jni.h"
#include <stdio.h>
#include <stdlib.h>
@interface NSObject (WindowCallbackTest)
+ (id)appkitProxy;
- (JNIEnv *)appkitEnv;
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(id)font backgroundColor:(id)background foregroundColor:(id)foreground cursor:(id)cursor;
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(id)view resizable:(BOOL)resizable title:(id)title;
- (void)awtSetFrame:(NSRect)frame display:(BOOL)display;
- (BOOL)isJavaEvent;
- (jobject)javaEvent;
- (void)releaseJavaEvent:(JNIEnv *)env;
@end
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
@interface WindowCallbackProbe : NSObject
+ (int)exercisePeer:(int)peer kind:(int)kind;
@end
@implementation WindowCallbackProbe
+ (int)exercisePeer:(int)peer kind:(int)kind
{
    JNIEnv *env = [NSApp appkitEnv];
    id view = [[NSClassFromString(@"AwtContainer") alloc] initWithPeer:(jobject)peer
        frame:NSMakeRect(0, 0, 300, 180) enabled:YES font:nil
        backgroundColor:[NSColor redColor] foregroundColor:nil cursor:nil];
    NSWindow *window = [[NSClassFromString(@"AwtWindow") alloc] initWithPeer:(jobject)peer
        frame:NSMakeRect(100, 120, 300, 180) enabled:YES contentView:view resizable:YES title:@"callbacks"];
    NSEvent *cover;
    jobject event;
    CHECK(window != nil);
    if (kind % 2 == 0) [window windowDidMove:nil];
    else {
        [window awtSetFrame:NSMakeRect(130, 140, 320, 200) display:NO];
        [window windowDidResize:nil];
    }
    switch (kind) {
        case 0: CHECK(![window windowShouldClose:nil]); break;
        case 1: [window windowDidMiniaturize:nil]; break;
        case 2: [window windowDidDeminiaturize:nil]; break;
        case 3: [window windowDidBecomeKey:nil]; break;
        case 4: [window windowDidResignKey:nil]; break;
    }
    cover = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:NO];
    CHECK(cover && [cover isJavaEvent]);
    event = (*env)->NewGlobalRef(env, [cover javaEvent]);
    while ((cover = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:YES]))
        if ([cover isJavaEvent]) [cover releaseJavaEvent:env];
    [window release];
    [view release];
    return (int)event;
}
@end
JNIEXPORT jobject JNICALL Java_WindowCallbacks115_exercise(JNIEnv *env, jclass cls, jobject source, jint kind)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RFramePeer");
    jobject peer, globalPeer, event, result;
    jfieldID field;
    jfieldID eventMask;
    jlong mask;
    jobjectArray root;
    if (!peerClass) { [pool release]; return NULL; }
    peer = (*env)->AllocObject(env, peerClass);
    field = (*env)->GetFieldID(env, peerClass, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) { [pool release]; return NULL; }
    (*env)->SetObjectField(env, peer, field, source);
    eventMask = (*env)->GetFieldID(env, (*env)->FindClass(env, "java/awt/Component"), "eventMask", "J");
    CHECK(eventMask != NULL);
    mask = (*env)->GetLongField(env, source, eventMask) | ((jlong)1 << 40);
    (*env)->SetLongField(env, source, eventMask, mask);
    globalPeer = (*env)->NewGlobalRef(env, peer);
    event = (jobject)[[WindowCallbackProbe appkitProxy] exercisePeer:(int)globalPeer kind:kind];
    CHECK((*env)->GetLongField(env, source, eventMask) == (mask | 1));
    root = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/Object"), event);
    result = (*env)->GetObjectArrayElement(env, root, 0);
    (*env)->DeleteGlobalRef(env, event);
    (*env)->DeleteGlobalRef(env, globalPeer);
    [pool release];
    return result;
}
