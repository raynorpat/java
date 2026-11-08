#ifndef RHAPSODY_AWT_EVENT_H
#define RHAPSODY_AWT_EVENT_H

#import <AppKit/AppKit.h>
#include "jni.h"

@interface NSEvent (AWTEventCover)
+ (id)coverEventForJavaEvent:(jobject)event inEnv:(JNIEnv *)env;
- (BOOL)isJavaEvent;
- (jobject)javaEvent;
- (void)releaseJavaEvent:(JNIEnv *)env;
- (BOOL)isSynthetic;
- (int)javaModifiers;
- (void)updateLocationTo:(NSPoint)point;
- (unichar)javaCharacter;
- (int)javaKeyCode;
- (void)updateToMatchJavaEvent:(jobject)event;
@end

@interface SyntheticEvent : NSEvent
+ (id)keyEvent:(int)type keyCode:(int)code keyChar:(unichar)character modifiers:(int)modifiers forView:(id)view;
+ (id)mouseEvent:(int)type x:(int)x y:(int)y modifiers:(int)modifiers clickCount:(int)clicks forView:(id)view;
@end

int nsModifiersToJava(int flags);
int javaModifiersToNS(int modifiers);
void nsCharToJavaVirtualKeyCode(unichar character, int flags, int *code, BOOL *typed);
void nsFlagsChangedToJavaVirtualKeyCode(int flags, int *code, BOOL *pressed);
unichar nsCharToJavaChar(unichar character);
jobject nsKeyEventToJavaEvent(JNIEnv *env, NSEvent *event, jobject source);
jobject makeKeyTypedEvent(JNIEnv *env, NSEvent *event, id view);
jobject javaComponentForView(JNIEnv *env, id view);
jobject makeMouseClickedEvent(JNIEnv *env, NSEvent *event, id view);
jobject nsMouseEventToJavaEvent(JNIEnv *env, NSEvent *event, jobject source, id view);

NSEvent *makeWrappedPaintEvent(JNIEnv *env, id view, int type, NSRect rect);
NSEvent *makeWrappedWindowEvent(JNIEnv *env, id window, int type);
NSEvent *makeWrappedFocusEvent(JNIEnv *env, id view, BOOL gained, BOOL temporary);

#endif
