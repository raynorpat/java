#ifndef RHAPSODY_AWT_APPLICATION_H
#define RHAPSODY_AWT_APPLICATION_H

#import <AppKit/AppKit.h>
#include "jni.h"

@interface AwtApplication : NSApplication
+ (id)sharedApplicationUsingEnv:(JNIEnv *)env awtToolkit:(jobject)toolkit;
@end

@interface NSApplication (AWTAdditions)
- (id)proxyForClassByName:(NSString *)name;
- (id)proxyForObjectByAddress:(int)address;
- (void)buildConnectionForDO;
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env;
- (void)commonAwtInitialization;
- (NSString *)portName;
- (BOOL)inAppkitThread;
- (JNIEnv *)appkitEnv;
- (unsigned int)previousEventFlags;
- (BOOL)dispatchJavaEvent:(jobject)event isLocalRef:(BOOL)local;
- (id)findAwtTargetForEvent:(NSEvent *)event;
- (NSEvent *)sendNSEventThroughJava:(NSEvent *)event;
/* Owned global JNI handle, transmitted as an integer through NSConnection. */
- (int)nextJavaEventOfType:(int)type dequeue:(BOOL)dequeue;
@end

@interface NSObject (AWTProxy)
- (id)appkitProxy;
+ (id)appkitProxy;
+ (id)allocInAppkitThread;
- (id)windowProxy;
@end

#endif
