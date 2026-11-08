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
@end

@interface SyntheticEvent : NSEvent
@end

NSEvent *makeWrappedPaintEvent(JNIEnv *env, id view, int type, NSRect rect);
NSEvent *makeWrappedWindowEvent(JNIEnv *env, id window, int type);

#endif
