#ifndef RHAPSODY_AWT_CONVERSIONS_H
#define RHAPSODY_AWT_CONVERSIONS_H

#import <AppKit/AppKit.h>
#include "jni.h"

NSString *javaToNSString(JNIEnv *env, jstring value);
jstring nsToJavaString(JNIEnv *env, NSString *value);
NSColor *javaToNSColor(JNIEnv *env, jobject value);
NSFont *javaToNSFont(JNIEnv *env, jobject value);
NSCursor *javaToNSCursor(JNIEnv *env, jobject value);
int javaToNSTextAlignment(int alignment);

#endif
