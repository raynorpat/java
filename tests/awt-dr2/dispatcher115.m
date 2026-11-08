#import <AppKit/AppKit.h>
#include "jni.h"
#import "../../src/rhapsody/sun/awt/awt_Event.h"

@interface NSApplication (DispatcherTest)
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env;
- (BOOL)inAppkitThread;
- (JNIEnv *)appkitEnv;
@end
@interface NSObject (DispatcherTest)
- (id)appkitProxy;
+ (id)defaultDispatcher;
- (BOOL)dispatchJavaEvent:(jobject)event;
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer;
@end

static int releasedCovers;
static int postedCovers;
@interface QueueCoverProbe : NSEvent
@end
@implementation QueueCoverProbe
- (void)releaseJavaEvent:(JNIEnv *)env
{
    if ([self isJavaEvent] && [self javaEvent]) releasedCovers++;
    [super releaseJavaEvent:env];
}
@end
@interface DispatchProbe : NSObject
- (BOOL)dispatchAddress:(int)address;
- (void)menuAddress:(int)address;
- (BOOL)postAddress:(int)address;
- (BOOL)coverReleased;
@end
@implementation DispatchProbe
- (BOOL)postAddress:(int)address
{
    NSEvent *cover, *peek;
    if (!postedCovers) [QueueCoverProbe poseAsClass:[NSEvent class]];
    cover = [NSEvent coverEventForJavaEvent:(jobject)address inEnv:[NSApp appkitEnv]];
    postedCovers++;
    [NSApp postEvent:cover atStart:YES];
    peek = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:NO];
    return [peek isJavaEvent] && [peek javaEvent] == [cover javaEvent] && [cover javaEvent] != NULL;
}
- (BOOL)coverReleased { return postedCovers == releasedCovers; }
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

JNIEXPORT jboolean JNICALL Java_ToolkitStartup115_post(JNIEnv *env, jclass cls, jobject event)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jobject global = (*env)->NewGlobalRef(env, event);
    BOOL result = [[probe appkitProxy] postAddress:(int)global];
    (*env)->DeleteGlobalRef(env, global);
    [pool release];
    return result;
}
JNIEXPORT jboolean JNICALL Java_ToolkitStartup115_coverReleased(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    BOOL result = [[probe appkitProxy] coverReleased];
    [pool release];
    return result;
}
