/* DR2 libawt.A.dylib: 540149f0--540157c4. */
#import "awt_Conversions.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"

static NSFont *metricsFont(JNIEnv *env, jobject metrics)
{
    jclass cls = (*env)->GetObjectClass(env, metrics);
    jfieldID field = (*env)->GetFieldID(env, cls, "font", "Ljava/awt/Font;");
    jobject value;
    NSFont *font;
    if (!field) [AwtException raiseUsingEnv:env];
    value = (*env)->GetObjectField(env, metrics, field);
    font = javaToNSFont(env, value);
    (*env)->DeleteLocalRef(env, value);
    (*env)->DeleteLocalRef(env, cls);
    return font;
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFontMetrics_init(JNIEnv *env, jobject metrics)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *font = metricsFont(env, metrics);
        NSFont *screen = [font screenFont];
        NSRect bounds;
        int descent, index;
        jint widths[256], values[8];
        const char *names[] = {"ascent", "maxAscent", "descent", "maxDescent",
                               "height", "maxHeight", "maxAdvance", "leading"};
        jclass cls = (*env)->GetObjectClass(env, metrics);
        jintArray array;
        jfieldID field;
        if (!screen) screen = font;
        bounds = [screen boundingRectForFont];
        descent = (int)([font descender] - 0.5);
        if (descent < 0) descent = -descent;
        values[0] = values[1] = (int)bounds.size.height - 1 - descent;
        values[2] = values[3] = descent;
        values[4] = values[5] = (int)bounds.size.height;
        values[6] = (int)bounds.size.width;
        values[7] = 1;
        for (index = 0; index < 256; index++)
            widths[index] = (int)([screen advancementForGlyph:index].width + 0.5);
        for (index = 0; index < 8; index++) {
            field = (*env)->GetFieldID(env, cls, names[index], "I");
            if (!field) [AwtException raiseUsingEnv:env];
            (*env)->SetIntField(env, metrics, field, values[index]);
        }
        array = (*env)->NewIntArray(env, 256);
        if (!array) [AwtException raiseUsingEnv:env];
        (*env)->SetIntArrayRegion(env, array, 0, 256, widths);
        field = (*env)->GetFieldID(env, cls, "widths", "[I");
        if (!field) [AwtException raiseUsingEnv:env];
        (*env)->SetObjectField(env, metrics, field, array);
        (*env)->DeleteLocalRef(env, array);
        (*env)->DeleteLocalRef(env, cls);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFontMetrics_stringWidth(JNIEnv *env, jobject metrics, jstring text)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *font = metricsFont(env, metrics);
        result = (jint)[font widthOfString:javaToNSString(env, text)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

static jint metricsArrayWidth(JNIEnv *env, jobject metrics, jarray text,
                              jint offset, jint length, BOOL characters)
{
    jint result = 0;
    void *volatile buffer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSFont *font = metricsFont(env, metrics);
        jsize count;
        NSString *string;
        if (!text) [AwtException raise:env exceptionName:"java/lang/NullPointerException" reason:"Null text array"];
        count = (*env)->GetArrayLength(env, text);
        if (offset < 0 || length < 0 || offset > count || length > count - offset)
            [AwtException raise:env exceptionName:"java/lang/ArrayIndexOutOfBoundsException" reason:"Text array range"];
        if (length) {
            if (characters) buffer = (*env)->GetCharArrayElements(env, (jcharArray)text, NULL);
            else buffer = (*env)->GetByteArrayElements(env, (jbyteArray)text, NULL);
            if (!buffer) [AwtException raiseUsingEnv:env];
            /* The donor measures length bytes of UTF-16 storage for charsWidth.
               Preserve that result without its undersized temporary buffer. */
            string = [NSString stringWithCString:(const char *)buffer + offset * (characters ? sizeof(jchar) : 1) length:length];
        } else string = @"";
        result = (jint)[font widthOfString:string];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (buffer) {
        if (characters) (*env)->ReleaseCharArrayElements(env, (jcharArray)text, (jchar *)buffer, JNI_ABORT);
        else (*env)->ReleaseByteArrayElements(env, (jbyteArray)text, (jbyte *)buffer, JNI_ABORT);
    }
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFontMetrics_charsWidth(JNIEnv *env, jobject metrics, jcharArray text, jint offset, jint length)
{
    return metricsArrayWidth(env, metrics, text, offset, length, YES);
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFontMetrics_bytesWidth(JNIEnv *env, jobject metrics, jbyteArray text, jint offset, jint length)
{
    return metricsArrayWidth(env, metrics, text, offset, length, NO);
}

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
