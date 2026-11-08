#import <AppKit/AppKit.h>
#include "jni.h"

@interface NSApplication (DispatcherTest)
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env;
- (BOOL)inAppkitThread;
@end
@interface NSObject (DispatcherTest)
- (id)appkitProxy;
+ (id)defaultDispatcher;
- (BOOL)dispatchJavaEvent:(jobject)event;
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer;
@end

@interface DispatchProbe : NSObject
- (BOOL)dispatchAddress:(int)address;
- (void)menuAddress:(int)address;
@end
@implementation DispatchProbe
- (void)menuAddress:(int)address
{
    if (![NSApp inAppkitThread])
        [NSException raise:@"WrongThread" format:@"menu dispatch must reach AppKit"];
    [[NSClassFromString(@"AwtDispatcher") defaultDispatcher] dispatchActionEventToMenuItemPeer:(jobject)address];
}
- (BOOL)dispatchAddress:(int)address
{
    if (![NSApp inAppkitThread])
        [NSException raise:@"WrongThread" format:@"dispatch must reach AppKit"];
    return [[NSClassFromString(@"AwtDispatcher") defaultDispatcher] dispatchJavaEvent:(jobject)address];
}
@end
static DispatchProbe *probe;

JNIEXPORT jboolean JNICALL Java_ToolkitStartup115_dispatch(JNIEnv *env, jclass cls, jobject event)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobject global = (*env)->NewGlobalRef(env, event);
    BOOL result;
    if (!probe) probe = [DispatchProbe new];
    result = [[probe appkitProxy] dispatchAddress:(int)global];
    (*env)->DeleteGlobalRef(env, global);
    [pool release];
    return result;
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Dispatcher115_menu(JNIEnv *env, jclass cls, jobject peer)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobject global = (*env)->NewGlobalRef(env, peer);
    [[probe appkitProxy] menuAddress:(int)global];
    (*env)->DeleteGlobalRef(env, global);
    [pool release];
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_Dispatcher115_initialize(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    [NSApp setupForAwtToolkitUsingEnv:env];
    probe = [DispatchProbe new];
    [pool release];
}

JNIEXPORT jboolean JNICALL Java_com_apple_rhapsody_awt_Dispatcher115_dispatch(JNIEnv *env, jclass cls, jobject event)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobject global = (*env)->NewGlobalRef(env, event);
    BOOL result = [[probe appkitProxy] dispatchAddress:(int)global];
    (*env)->DeleteGlobalRef(env, global);
    [pool release];
    return result;
}
