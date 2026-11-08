#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (SyntheticTest)
+ (id)keyEvent:(int)type keyCode:(int)code keyChar:(unichar)character modifiers:(int)modifiers forView:(id)view;
+ (id)mouseEvent:(int)type x:(int)x y:(int)y modifiers:(int)modifiers clickCount:(int)clicks forView:(id)view;
- (BOOL)isSynthetic;
@end
@interface SyntheticView : NSObject
- (id)window;
- (int)windowNumber;
- (NSPoint)convertPoint:(NSPoint)point toView:(id)view;
@end
@implementation SyntheticView
- (id)window { return self; }
- (int)windowNumber { return 37; }
- (NSPoint)convertPoint:(NSPoint)point toView:(id)view { return NSMakePoint(point.x + 10, point.y + 20); }
@end
JNIEXPORT jintArray JNICALL Java_SyntheticValues115_convert(JNIEnv *env, jclass cls,
    jboolean key, jint type, jint code, jchar character, jint modifiers)
{
    id pool = [[NSAutoreleasePool alloc] init];
    Class factory = NSClassFromString(@"SyntheticEvent");
    id view = [[[SyntheticView alloc] init] autorelease];
    NSEvent *event = nil;
    jint values[9] = {0};
    jintArray result = NULL;
    int index, exceptions = 0;
    SEL selectors[] = {@selector(eventNumber), @selector(trackingNumber), @selector(userData), @selector(isARepeat), @selector(keyCode)};
    if (![factory respondsToSelector:@selector(keyEvent:keyCode:keyChar:modifiers:forView:)]) {
        (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "missing synthetic event factories");
        [pool release];
        return NULL;
    }
    NS_DURING
        if (key) event = [factory keyEvent:type keyCode:code keyChar:character modifiers:modifiers forView:view];
        else event = [factory mouseEvent:type x:-7 y:9 modifiers:modifiers clickCount:3 forView:view];
    NS_HANDLER
        if (type == 999 && [[localException name] isEqualToString:@"AWT error"]) values[0] = -1;
        else (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), [[localException reason] cString]);
    NS_ENDHANDLER
    if (event) {
        values[0] = [event type]; values[1] = [event modifierFlags];
        values[2] = (int)[event locationInWindow].x; values[3] = (int)[event locationInWindow].y;
        if (key && [event type] != NSFlagsChanged) {
            values[4] = [[event characters] characterAtIndex:0];
            if (![[event characters] isEqualToString:[event charactersIgnoringModifiers]])
                (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "synthetic character strings differ");
        }
        values[5] = [event windowNumber]; values[6] = key ? 0 : [event clickCount];
        values[7] = [event isSynthetic];
        for (index = 0; index < 5; index++) {
            NS_DURING
                [event performSelector:selectors[index]];
            NS_HANDLER
                if ([[localException name] isEqualToString:@"AWT error"]) exceptions++;
            NS_ENDHANDLER
        }
        values[8] = exceptions;
        if ([event timestamp] != 0 || (!key && [event pressure] != 1.0))
            (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "synthetic timestamp or pressure differs");
    }
    if (event || values[0] == -1) {
        result = (*env)->NewIntArray(env, 9);
        (*env)->SetIntArrayRegion(env, result, 0, 9, values);
    }
    [pool release];
    return result;
}

@interface NSObject (SyntheticActivation)
+ (id)appkitProxy;
@end
@interface SyntheticActivationProbe : NSObject
+ (void)activateView:(int)address;
@end
@implementation SyntheticActivationProbe
+ (void)activateView:(int)address
{
    NSView *view = (NSView *)address;
    [NSApp activateIgnoringOtherApps:YES];
    [[view window] makeKeyAndOrderFront:nil];
    [[view window] makeFirstResponder:view];
}
@end
JNIEXPORT void JNICALL Java_SyntheticInput115_activate(JNIEnv *env, jclass cls, jobject frame)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[SyntheticActivationProbe appkitProxy] activateView:(*env)->GetIntField(env, peer, field)];
    [pool release];
}
