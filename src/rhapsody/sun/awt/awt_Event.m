/* DR2 libawt.A.dylib: NSEvent Java covers at 54005354--54005434. */
#import "awt_Event.h"
#import "awt_Exception.h"

@interface NSObject (AWTPeerAccess)
- (jobject)peer;
@end

@implementation NSEvent (AWTEventCover)
+ (id)coverEventForJavaEvent:(jobject)event inEnv:(JNIEnv *)env
{
    jobject reference = (*env)->NewGlobalRef(env, event);
    return [self otherEventWithType:NSApplicationDefined location:NSMakePoint(0, 0)
        modifierFlags:0 timestamp:0 windowNumber:0 context:[NSApp context]
        subtype:NSAWTEventType data1:(int)reference data2:0];
}
- (BOOL)isJavaEvent
{
    return [self type] == NSApplicationDefined && [self subtype] == NSAWTEventType;
}
- (BOOL)isSynthetic { return NO; }
- (jobject)javaEvent { return (jobject)[self data1]; }
- (void)releaseJavaEvent:(JNIEnv *)env
{
    (*env)->DeleteGlobalRef(env, [self javaEvent]);
    _data.misc.data1 = 0;
}
@end

@implementation SyntheticEvent
- (BOOL)isSynthetic { return YES; }
@end

/* 54006684: preserve the native rectangle's truncation toward zero. */
NSEvent *makeWrappedPaintEvent(JNIEnv *env, id view, int type, NSRect rect)
{
    jclass cls;
    jfieldID field;
    jmethodID constructor;
    jobject source, rectangle, event;
    NSEvent *cover;
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) [AwtException raiseUsingEnv:env];
    source = (*env)->GetObjectField(env, [view peer], field);
    (*env)->DeleteLocalRef(env, cls);
    cls = (*env)->FindClass(env, "java/awt/Rectangle");
    if (!cls) [AwtException raiseUsingEnv:env];
    constructor = (*env)->GetMethodID(env, cls, "<init>", "(IIII)V");
    if (!constructor) [AwtException raiseUsingEnv:env];
    rectangle = (*env)->NewObject(env, cls, constructor, (int)rect.origin.x,
        (int)rect.origin.y, (int)rect.size.width, (int)rect.size.height);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    cls = (*env)->FindClass(env, "java/awt/event/PaintEvent");
    if (!cls) [AwtException raiseUsingEnv:env];
    constructor = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;ILjava/awt/Rectangle;)V");
    if (!constructor) [AwtException raiseUsingEnv:env];
    event = (*env)->NewObject(env, cls, constructor, source, type, rectangle);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, source);
    (*env)->DeleteLocalRef(env, rectangle);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    cover = [SyntheticEvent coverEventForJavaEvent:event inEnv:env];
    (*env)->DeleteLocalRef(env, event);
    return cover;
}
