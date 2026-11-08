#import <AppKit/AppKit.h>
#include "jni.h"
#include <math.h>

extern NSString *javaToNSString(JNIEnv *, jstring);
extern NSColor *javaToNSColor(JNIEnv *, jobject);

JNIEXPORT jboolean JNICALL Java_Values115_stringMatches(JNIEnv *env, jclass cls, jstring value)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSString *native = javaToNSString(env, value);
    BOOL result = native == nil;
    if (value) {
        const jchar *chars = (*env)->GetStringChars(env, value, NULL);
        jsize length = (*env)->GetStringLength(env, value), index;
        result = [native length] == length;
        for (index = 0; result && index < length; index++)
            result = [native characterAtIndex:index] == chars[index];
        (*env)->ReleaseStringChars(env, value, chars);
    }
    [pool release];
    return result;
}
JNIEXPORT jint JNICALL Java_Values115_colorRGB(JNIEnv *env, jclass cls, jobject value)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSColor *native = javaToNSColor(env, value);
    jint result = -1;
    if (native) {
        float red, green, blue, alpha;
        [native getRed:&red green:&green blue:&blue alpha:&alpha];
        if (alpha == 1.0)
            result = ((int)floor(red * 255.0 + 0.5) << 16) |
                     ((int)floor(green * 255.0 + 0.5) << 8) |
                     (int)floor(blue * 255.0 + 0.5);
    }
    [pool release];
    return result;
}
