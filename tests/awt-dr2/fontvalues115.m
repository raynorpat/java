#import <AppKit/AppKit.h>
#include "jni.h"
extern NSFont *javaToNSFont(JNIEnv *, jobject);

JNIEXPORT jstring JNICALL Java_FontValues115_describe(JNIEnv *env, jclass cls, jobject value)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSFont *font = javaToNSFont(env, value);
    jstring result = NULL;
    if (font) {
        NSString *description = [NSString stringWithFormat:@"%@:%g", [font fontName], [font pointSize]];
        result = (*env)->NewStringUTF(env, [description cString]);
    }
    [pool release];
    return result;
}
