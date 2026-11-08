#ifndef RHAPSODY_AWT_CONVERSIONS_H
#define RHAPSODY_AWT_CONVERSIONS_H

#import <AppKit/AppKit.h>
#include "jni.h"

NSString *javaToNSString(JNIEnv *env, jstring value);
NSColor *javaToNSColor(JNIEnv *env, jobject value);

#endif
