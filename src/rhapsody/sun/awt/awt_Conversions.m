/* DR2 libawt.A.dylib: 5401f3c8, 5401f5f8. */
#import "awt_Conversions.h"
#import "awt_Exception.h"

NSString *javaToNSString(JNIEnv *env, jstring value)
{
    const jchar *characters;
    NSString *result;
    if (!value) return nil;
    characters = (*env)->GetStringChars(env, value, NULL);
    if (!characters) [AwtException raiseUsingEnv:env];
    result = [NSString stringWithCharacters:characters length:(*env)->GetStringLength(env, value)];
    (*env)->ReleaseStringChars(env, value, characters);
    return result;
}

NSColor *javaToNSColor(JNIEnv *env, jobject value)
{
    jclass cls;
    jmethodID method;
    jint rgb;
    if (!value) return nil;
    cls = (*env)->FindClass(env, "java/awt/Color");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "getRGB", "()I");
    if (!method) [AwtException raiseUsingEnv:env];
    rgb = (*env)->CallIntMethod(env, value, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return [NSColor colorWithDeviceRed:((rgb >> 16) & 255) / 255.0
                                green:((rgb >> 8) & 255) / 255.0
                                 blue:(rgb & 255) / 255.0 alpha:1.0];
}
