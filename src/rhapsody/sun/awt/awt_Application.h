#ifndef RHAPSODY_AWT_APPLICATION_H
#define RHAPSODY_AWT_APPLICATION_H

#import <AppKit/AppKit.h>
#include "jni.h"

@interface NSApplication (AWTAdditions)
- (id)proxyForClassByName:(NSString *)name;
- (id)proxyForObjectByAddress:(int)address;
- (void)buildConnectionForDO;
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env;
- (void)commonAwtInitialization;
- (NSString *)portName;
- (BOOL)inAppkitThread;
- (JNIEnv *)appkitEnv;
@end

@interface NSObject (AWTProxy)
- (id)appkitProxy;
+ (id)appkitProxy;
+ (id)allocInAppkitThread;
- (id)windowProxy;
@end

#endif
