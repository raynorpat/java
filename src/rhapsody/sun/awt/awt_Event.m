/* DR2 libawt.A.dylib: NSEvent Java covers at 54005354--54005434. */
#import "awt_Event.h"
#import "awt_Exception.h"
#import "awt_View.h"

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
/* 54005478: right-button events carry Java's historical META_MASK. */
- (int)javaModifiers
{
    int modifiers = nsModifiersToJava([self modifierFlags]);
    int type = [self type];
    if (type == NSRightMouseDown || type == NSRightMouseUp || type == NSRightMouseDragged)
        modifiers |= 4;
    return modifiers;
}
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

/* 54006634: window lifecycle events use the same owned native cover. */
NSEvent *makeWrappedWindowEvent(JNIEnv *env, id window, int type)
{
    jclass cls;
    jfieldID field;
    jmethodID constructor;
    jobject source, event;
    NSEvent *cover;
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) [AwtException raiseUsingEnv:env];
    source = (*env)->GetObjectField(env, [window peer], field);
    (*env)->DeleteLocalRef(env, cls);
    cls = (*env)->FindClass(env, "java/awt/event/WindowEvent");
    if (!cls) [AwtException raiseUsingEnv:env];
    constructor = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Window;I)V");
    if (!constructor) [AwtException raiseUsingEnv:env];
    event = (*env)->NewObject(env, cls, constructor, source, type);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, source);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    cover = [SyntheticEvent coverEventForJavaEvent:event inEnv:env];
    (*env)->DeleteLocalRef(env, event);
    return cover;
}

/* 540065c4: responder transitions are queued synthetic Java events. */
NSEvent *makeWrappedFocusEvent(JNIEnv *env, id view, BOOL gained, BOOL temporary)
{
    jobject source = javaComponentForView(env, view), event;
    jclass cls = (*env)->FindClass(env, "java/awt/event/FocusEvent");
    jmethodID constructor;
    NSEvent *cover;
    if (!cls) [AwtException raiseUsingEnv:env];
    constructor = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IZ)V");
    if (!constructor) [AwtException raiseUsingEnv:env];
    event = (*env)->NewObject(env, cls, constructor, source, gained ? 1004 : 1005, temporary);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, source);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    cover = [SyntheticEvent coverEventForJavaEvent:event inEnv:env];
    (*env)->DeleteLocalRef(env, event);
    return cover;
}

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

/* 54005cc0, table 540261bc: help and caps-lock have no Java 1.1 mask. */
int nsModifiersToJava(int flags)
{
    int result = 0;
    if (flags & NSAlternateKeyMask) result |= 8;
    if (flags & NSControlKeyMask) result |= 2;
    if (flags & NSShiftKeyMask) result |= 1;
    if (flags & NSCommandKeyMask) result |= 4;
    return result;
}

jobject javaComponentForView(JNIEnv *env, id view)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field;
    jobject source;
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) [AwtException raiseUsingEnv:env];
    source = (*env)->GetObjectField(env, [view peer], field);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return source;
}

static jobject newMouseEvent(JNIEnv *env, NSEvent *event, jobject source, NSPoint point,
    int eventID, int clicks, BOOL popup)
{
    jclass cls;
    jmethodID method;
    jlong when;
    jobject result;
    cls = (*env)->FindClass(env, "java/lang/System");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetStaticMethodID(env, cls, "currentTimeMillis", "()J");
    if (!method) [AwtException raiseUsingEnv:env];
    when = (*env)->CallStaticLongMethod(env, cls, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    cls = (*env)->FindClass(env, "java/awt/event/MouseEvent");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IJIIIIZ)V");
    if (!method) [AwtException raiseUsingEnv:env];
    result = (*env)->NewObject(env, cls, method, source, eventID, when,
        [event javaModifiers], (int)point.x, (int)point.y, clicks, popup);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return result;
}

/* 5400625c: coordinates truncate toward zero; time is Java's wall clock,
 * not NSEvent's uptime timestamp. Tracking notifications are hit-tested anew. */
jobject nsMouseEventToJavaEvent(JNIEnv *env, NSEvent *event, jobject source, id view)
{
    NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
    int type = [event type], eventID, clicks = 0;
    switch (type) {
        case NSLeftMouseDown: case NSRightMouseDown: eventID = 501; clicks = [event clickCount]; break;
        case NSLeftMouseUp: case NSRightMouseUp: eventID = 502; clicks = [event clickCount]; break;
        case NSMouseMoved: eventID = 503; break;
        case NSLeftMouseDragged: case NSRightMouseDragged: eventID = 506; clicks = [event clickCount]; break;
        case NSMouseEntered: case NSMouseExited:
            eventID = view == [[view window] deepestViewUnderLocation:[event locationInWindow]
                dispatchingEventsOfType:NSMouseEntered] ? 504 : 505;
            break;
        default: return NULL;
    }
    return newMouseEvent(env, event, source, point, eventID, clicks, type == NSRightMouseDown);
}

/* 54006424: only a non-consumed left-button release generates this event. */
jobject makeMouseClickedEvent(JNIEnv *env, NSEvent *event, id view)
{
    jobject source = javaComponentForView(env, view);
    NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
    jobject result = newMouseEvent(env, event, source, point, 500, [event clickCount], NO);
    (*env)->DeleteLocalRef(env, source);
    return result;
}
