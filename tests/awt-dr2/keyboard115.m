#import <AppKit/AppKit.h>
#include "jni.h"
@interface NSObject (KeyboardTest)
+ (id)appkitProxy;
@end
@interface KeyboardProbe : NSObject
+ (void)injectView:(int)address kind:(int)kind;
@end
@implementation KeyboardProbe
+ (void)injectView:(int)address kind:(int)kind
{
    NSView *view = (NSView *)address;
    NSWindow *window = [view window];
    int type;
    unichar arrow = NSUpArrowFunctionKey;
    NSString *characters = kind == 5 ? [NSString stringWithCharacters:&arrow length:1] : @"A";
    NSString *plain = kind == 5 ? characters : @"a";
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
    if (kind == 4) {
        int index, flags[] = {0, NSAlternateKeyMask, 0};
        for (index = 0; index < 3; index++) {
            NSEvent *event = [NSEvent keyEventWithType:NSFlagsChanged location:NSMakePoint(0, 0)
                modifierFlags:flags[index] timestamp:1 windowNumber:[window windowNumber]
                context:[NSApp context] characters:@"" charactersIgnoringModifiers:@""
                isARepeat:NO keyCode:0];
            [NSApp postEvent:event atStart:NO];
        }
        return;
    }
    for (type = NSKeyDown; type <= NSKeyUp; type++) {
        NSEvent *event = [NSEvent keyEventWithType:type location:NSMakePoint(0, 0)
            modifierFlags:NSShiftKeyMask timestamp:1 windowNumber:[window windowNumber]
            context:[NSApp context] characters:characters charactersIgnoringModifiers:plain
            isARepeat:NO keyCode:0];
        [NSApp postEvent:event atStart:NO];
    }
}
@end
JNIEXPORT void JNICALL Java_Keyboard115_inject(JNIEnv *env, jclass cls, jobject frame, jint kind)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass component = (*env)->FindClass(env, "java/awt/Component");
    jmethodID method = (*env)->GetMethodID(env, component, "getPeer", "()Ljava/awt/peer/ComponentPeer;");
    jobject peer = (*env)->CallObjectMethod(env, frame, method);
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    jfieldID field = (*env)->GetFieldID(env, peerClass, "nNativeView", "I");
    [[KeyboardProbe appkitProxy] injectView:(*env)->GetIntField(env, peer, field) kind:kind];
    [pool release];
}
