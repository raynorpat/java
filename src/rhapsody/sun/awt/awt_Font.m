/* DR2 libawt.A.dylib: 540149f0--54014cf4. */
#import "awt_Conversions.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"

@interface NSFontManager (AwtDR2)
- (NSDictionary *)_defaultFontSet;
@end

static NSString *fontNames[][2] = {
    {@"TimesRoman", @"Times"}, {@"Helvetica", @"Helvetica"},
    {@"Courier", @"Courier"}, {@"ZapfDingbats", @"TimesRoman"},
    {@"Serif", @"Times"}, {@"SansSerif", @"Helvetica"},
    {@"Monospaced", @"Courier"}, {@"Dialog", @"Helvetica"},
    {@"DialogInput", @"Helvetica"}, {nil, nil}
};

NSFont *javaToNSFont(JNIEnv *env, jobject value)
{
    jclass cls, peerClass;
    jmethodID sizeMethod, peerMethod;
    jfieldID nativeField;
    jint size;
    jobject peer;
    NSFont *font;
    if (!value) return nil;
    cls = (*env)->FindClass(env, "java/awt/Font");
    if (!cls) [AwtException raiseUsingEnv:env];
    sizeMethod = (*env)->GetMethodID(env, cls, "getSize", "()I");
    peerMethod = (*env)->GetMethodID(env, cls, "getPeer", "()Ljava/awt/peer/FontPeer;");
    if (!sizeMethod || !peerMethod) [AwtException raiseUsingEnv:env];
    size = (*env)->CallIntMethod(env, value, sizeMethod);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    peer = (*env)->CallObjectMethod(env, value, peerMethod);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RFontPeer");
    if (!peerClass) [AwtException raiseUsingEnv:env];
    nativeField = (*env)->GetFieldID(env, peerClass, "nNativeFont", "I");
    if (!nativeField) [AwtException raiseUsingEnv:env];
    font = (NSFont *)(*env)->GetIntField(env, peer, nativeField);
    (*env)->DeleteLocalRef(env, peerClass);
    (*env)->DeleteLocalRef(env, peer);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return [[NSFontManager sharedFontManager] convertFont:font toSize:(float)size];
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFontPeer_createNativeFont(JNIEnv *env, jobject peer, jstring name, jint style)
{
    NSFont *font = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeName = javaToNSString(env, name);
        NSFontManager *manager;
        int index;
        for (index = 0; fontNames[index][0]; index++) {
            if ([nativeName isEqualToString:fontNames[index][0]]) {
                nativeName = fontNames[index][1];
                break;
            }
        }
        font = [NSFont fontWithName:nativeName size:12.0];
        if (!font) font = [NSFont systemFontOfSize:12.0];
        manager = [NSFontManager sharedFontManager];
        if (style & 1) font = [manager convertFont:font toHaveTrait:NSBoldFontMask];
        if (style & 2) font = [manager convertFont:font toHaveTrait:NSItalicFontMask];
        [font retain];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)font;
}

JNIEXPORT jobjectArray JNICALL Java_com_apple_rhapsody_awt_RFontPeer_getFontList(JNIEnv *env, jclass peer)
{
    jobjectArray result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSArray *families = [[[[NSFontManager sharedFontManager] _defaultFontSet] objectForKey:@"FontFamilies"] allKeys];
        unsigned int index, count = [families count];
        jclass stringClass = (*env)->FindClass(env, "java/lang/String");
        if (!stringClass) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObjectArray(env, count, stringClass, NULL);
        (*env)->DeleteLocalRef(env, stringClass);
        if (!result) [AwtException raiseUsingEnv:env];
        for (index = 0; index < count; index++) {
            jstring name = nsToJavaString(env, [families objectAtIndex:index]);
            if (!name) [AwtException raiseUsingEnv:env];
            (*env)->SetObjectArrayElement(env, result, index, name);
            (*env)->DeleteLocalRef(env, name);
            if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
