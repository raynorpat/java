/* DR2 libawt.A.dylib: NSEvent Java covers at 54005354--54005434. */
#import "awt_Event.h"
#import "awt_Exception.h"
#import "awt_View.h"
#import "awt_Application.h"

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
/* 54005804: the donor's x86 shift test always permits this assignment. */
- (void)updateLocationTo:(NSPoint)point { _location = point; }
/* 540054c8/54005560: character and virtual code use different native strings. */
- (unichar)javaCharacter
{
    if ([self type] == NSFlagsChanged) return 0;
    if ([self type] != NSKeyDown && [self type] != NSKeyUp)
        [NSException raise:@"AWT error" format:@"Attempt to get character code from non-key event!"];
    return nsCharToJavaChar([[self characters] characterAtIndex:0]);
}
- (int)javaKeyCode
{
    int code;
    BOOL typed;
    if ([self type] < NSKeyDown || [self type] > NSFlagsChanged)
        [NSException raise:@"AWT error" format:@"Attempt to get virtual key code from non-key event!"];
    if ([self type] == NSFlagsChanged)
        nsFlagsChangedToJavaVirtualKeyCode([self modifierFlags], &code, &typed);
    else
        nsCharToJavaVirtualKeyCode([[self charactersIgnoringModifiers] characterAtIndex:0],
            [self modifierFlags], &code, &typed);
    return code;
}
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
/* 54005638/540057bc: listener changes become the native key's new state. */
- (void)updateToMatchJavaEvent:(jobject)event
{
    JNIEnv *env;
    jclass cls;
    jmethodID method;
    int modifiers, code;
    unichar character;
    NSString *characters;
    if ([self type] < NSKeyDown || [self type] > NSFlagsChanged) return;
    env = [NSApp appkitEnv];
    cls = (*env)->FindClass(env, "java/awt/event/KeyEvent");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "getModifiers", "()I");
    if (!method) [AwtException raiseUsingEnv:env];
    modifiers = (*env)->CallIntMethod(env, event, method);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "getKeyChar", "()C");
    if (!method) [AwtException raiseUsingEnv:env];
    character = (*env)->CallCharMethod(env, event, method);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "getKeyCode", "()I");
    if (!method) [AwtException raiseUsingEnv:env];
    code = (*env)->CallIntMethod(env, event, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    if (modifiers != [self javaModifiers]) {
        _modifierFlags = 0;
        if (modifiers & 8) _modifierFlags |= NSAlternateKeyMask;
        if (modifiers & 2) _modifierFlags |= NSControlKeyMask;
        if (modifiers & 1) _modifierFlags |= NSShiftKeyMask;
        if (modifiers & 4) _modifierFlags |= NSCommandKeyMask;
    }
    if (character != [self javaCharacter] || code != [self javaKeyCode]) {
        /* Reverse 54026680, taking the first matching Java character. */
        switch (character) {
            case 10: character = 3; break;
            case 8: character = 127; break;
            case 127: character = 63272; break;
            case 9: character = 25; break;
        }
        characters = [NSString stringWithCharacters:&character length:1];
        [_data.key.keys release];
        [_data.key.unmodKeys release];
        _data.key.keys = [characters retain];
        _data.key.unmodKeys = [characters retain];
    }
}
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

/* 54026210: ordered character/VK/typed/required-mask table. */
static const struct { unsigned int character; int code; BOOL typed; int flags; } keyTable[] = {
    {3, 10, 1, 0},
    {13, 10, 1, 0},
    {10, 10, 1, 0},
    {8, 8, 1, 0},
    {127, 8, 1, 0},
    {9, 9, 1, 0},
    {25, 9, 1, 131072},
    {63280, 19, 0, 0},
    {27, 27, 1, 0},
    {32, 32, 1, 0},
    {63276, 33, 0, 0},
    {63277, 34, 0, 0},
    {63275, 35, 0, 0},
    {63273, 36, 0, 0},
    {63234, 37, 0, 0},
    {63232, 38, 0, 0},
    {63235, 39, 0, 0},
    {63233, 40, 0, 0},
    {42, 106, 1, 2097152},
    {43, 107, 1, 2097152},
    {45, 109, 1, 2097152},
    {46, 110, 1, 2097152},
    {47, 111, 1, 2097152},
    {44, 44, 1, 0},
    {46, 46, 1, 0},
    {47, 47, 1, 0},
    {59, 59, 1, 0},
    {61, 61, 1, 0},
    {91, 91, 1, 0},
    {92, 92, 1, 0},
    {93, 93, 1, 0},
    {63236, 112, 0, 0},
    {63237, 113, 0, 0},
    {63238, 114, 0, 0},
    {63239, 115, 0, 0},
    {63240, 116, 0, 0},
    {63241, 117, 0, 0},
    {63242, 118, 0, 0},
    {63243, 119, 0, 0},
    {63244, 120, 0, 0},
    {63245, 121, 0, 0},
    {63246, 122, 0, 0},
    {63247, 123, 0, 0},
    {63272, 127, 1, 0},
    {63279, 145, 0, 0},
    {63278, 154, 0, 0},
    {63271, 155, 0, 0},
    {63302, 156, 0, 0},
    {96, 192, 1, 0},
    {39, 222, 1, 0},
    {126, 192, 1, 131072},
    {33, 49, 1, 131072},
    {64, 50, 1, 131072},
    {35, 51, 1, 131072},
    {36, 52, 1, 131072},
    {37, 53, 1, 131072},
    {94, 54, 1, 131072},
    {38, 55, 1, 131072},
    {42, 56, 1, 131072},
    {40, 57, 1, 131072},
    {41, 48, 1, 131072},
    {43, 61, 1, 131072},
    {124, 92, 1, 131072},
    {123, 91, 1, 131072},
    {125, 93, 1, 131072},
    {58, 59, 1, 131072},
    {34, 222, 1, 131072},
    {60, 44, 1, 131072},
    {62, 46, 1, 131072},
    {63, 47, 1, 131072},
    {0, 0, 0, 0}
};
/* Private Foundation conversion used by the donor (54005d6c). */
extern void NSConvertToLower(unichar character, unichar *result, int flags);
void nsCharToJavaVirtualKeyCode(unichar character, int flags, int *code, BOOL *typed)
{
    int index;
    unichar lower;
    if ([[NSCharacterSet letterCharacterSet] characterIsMember:character]) {
        NSConvertToLower(character, &lower, 0);
        if (lower >= 'a' && lower <= 'z') {
            *code = lower - 'a' + 65;
            *typed = YES;
            return;
        }
    }
    if ([[NSCharacterSet decimalDigitCharacterSet] characterIsMember:character] &&
        character >= '0' && character <= '9') {
        *code = (flags & NSNumericPadKeyMask) ? character - '0' + 96 : character;
        *typed = YES;
        return;
    }
    for (index = 0; keyTable[index].character; index++) {
        if (keyTable[index].character == character &&
            (!keyTable[index].flags || (flags & keyTable[index].flags))) {
            *code = keyTable[index].code;
            *typed = keyTable[index].typed;
            return;
        }
    }
    *code = 0;
    *typed = NO;
}
/* 54005ecc: only the first changed modifier is represented by a native event. */
void nsFlagsChangedToJavaVirtualKeyCode(int flags, int *code, BOOL *pressed)
{
    static const int masks[] = {NSAlternateKeyMask, NSControlKeyMask, NSShiftKeyMask,
        NSCommandKeyMask, NSHelpKeyMask, NSAlphaShiftKeyMask};
    static const int codes[] = {18, 17, 16, 157, 156, 20};
    unsigned int previous = [NSApp previousEventFlags];
    int index;
    for (index = 0; index < 6; index++) {
        if ((flags & masks[index]) != (previous & masks[index])) {
            *code = codes[index];
            *pressed = (flags & masks[index]) != 0;
            return;
        }
    }
    *code = 0;
    *pressed = NO;
}
/* 54005f40, table 54026680. */
unichar nsCharToJavaChar(unichar character)
{
    switch (character) {
        case 3: case 13: return 10;
        case 127: return 8;
        case 63272: return 127;
        case 25: return 9;
        default: return character;
    }
}
static jobject newKeyEvent(JNIEnv *env, NSEvent *event, jobject source, int eventID, int code)
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
    cls = (*env)->FindClass(env, "java/awt/event/KeyEvent");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IJIIC)V");
    if (!method) [AwtException raiseUsingEnv:env];
    result = (*env)->NewObject(env, cls, method, source, eventID, when,
        [event javaModifiers], code, [event javaCharacter]);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return result;
}

/* 54006060: Java time is wall-clock time, not native event uptime. */
jobject nsKeyEventToJavaEvent(JNIEnv *env, NSEvent *event, jobject source)
{
    int type = [event type], code, eventID;
    BOOL pressed;
    if (type == NSFlagsChanged) {
        nsFlagsChangedToJavaVirtualKeyCode([event modifierFlags], &code, &pressed);
        if (!code) return NULL;
        eventID = pressed ? 401 : 402;
    } else if (type == NSKeyDown || type == NSKeyUp) {
        code = [event javaKeyCode];
        eventID = type == NSKeyDown ? 401 : 402;
    } else return NULL;
    return newKeyEvent(env, event, source, eventID, code);
}

/* 54006178: only characters marked as typable generate KEY_TYPED. */
jobject makeKeyTypedEvent(JNIEnv *env, NSEvent *event, id view)
{
    int code;
    BOOL typed;
    jobject source, result;
    nsCharToJavaVirtualKeyCode([[event charactersIgnoringModifiers] characterAtIndex:0],
        [event modifierFlags], &code, &typed);
    if (!typed) return NULL;
    source = javaComponentForView(env, view);
    result = newKeyEvent(env, event, source, 400, 0);
    (*env)->DeleteLocalRef(env, source);
    return result;
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
