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
@end

@interface SyntheticEvent : NSEvent
@end

int nsModifiersToJava(int flags);
jobject javaComponentForView(JNIEnv *env, id view);
jobject makeMouseClickedEvent(JNIEnv *env, NSEvent *event, id view);
jobject nsMouseEventToJavaEvent(JNIEnv *env, NSEvent *event, jobject source, id view);

NSEvent *makeWrappedPaintEvent(JNIEnv *env, id view, int type, NSRect rect);
NSEvent *makeWrappedWindowEvent(JNIEnv *env, id window, int type);
NSEvent *makeWrappedFocusEvent(JNIEnv *env, id view, BOOL gained, BOOL temporary);

#endif
