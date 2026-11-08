#import <AppKit/AppKit.h>
#include "jni.h"

@interface NSEvent (EventCoverTest)
+ (id)coverEventForJavaEvent:(jobject)event inEnv:(JNIEnv *)env;
- (BOOL)isJavaEvent;
- (jobject)javaEvent;
- (void)releaseJavaEvent:(JNIEnv *)env;
@end

JNIEXPORT jint JNICALL Java_EventCover115_cover(JNIEnv *env, jclass cls, jobject value)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSEvent *event = [[NSEvent coverEventForJavaEvent:value inEnv:env] retain];
    NSEvent *ordinary = [NSEvent otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
        modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    if (![event isJavaEvent] || [event type] != NSApplicationDefined ||
        [event subtype] != NSAWTEventType || [event data2] != 0 || [ordinary isJavaEvent])
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "invalid cover event");
    [pool release];
    return (jint)event;
}
JNIEXPORT jobject JNICALL Java_EventCover115_unwrap(JNIEnv *env, jclass cls, jint handle)
{
    return [(NSEvent *)handle javaEvent];
}
JNIEXPORT jboolean JNICALL Java_EventCover115_release(JNIEnv *env, jclass cls, jint handle)
{
    NSEvent *event = (NSEvent *)handle;
    BOOL cleared;
    [event releaseJavaEvent:env];
    cleared = [event javaEvent] == NULL;
    [event release];
    return cleared;
}
