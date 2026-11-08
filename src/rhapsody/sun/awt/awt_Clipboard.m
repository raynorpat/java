/* DR2 libawt.A.dylib: 54003fc0--54004180 and 540120fc--540123c4. */
#import "awt_Clipboard.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@implementation AwtClipboard
- (id)initWithPeer:(int)peer { _peer = (jobject)peer; _changeCount = 0; return self; }
+ (void)discardClipboard:(int)address { [(AwtClipboard *)address release]; }
- (int)claimClipboardOwnership
{
    _changeCount = [[NSPasteboard generalPasteboard] declareTypes:
        [NSArray arrayWithObject:NSStringPboardType] owner:self];
    return _changeCount;
}
- (NSString *)loadExternalContents
{
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    return [[pasteboard types] containsObject:NSStringPboardType] ?
        [pasteboard stringForType:NSStringPboardType] : @"Data not readable by Java";
}
- (BOOL)javaReadableContentsAvailable
{ return [[[NSPasteboard generalPasteboard] types] containsObject:NSStringPboardType]; }
- (int)generalPasteboardCount { return [[NSPasteboard generalPasteboard] changeCount]; }
- (void)pasteboardChangedOwner:(NSPasteboard *)pasteboard
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RClipboard");
    jmethodID method;
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "lostOwnership", "()V");
    (*env)->DeleteLocalRef(env, cls);
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, _peer, method);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
}
- (void)pasteboard:(NSPasteboard *)pasteboard provideDataForType:(NSString *)type
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RClipboard");
    jmethodID method; jstring text; NSString *nativeText;
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "stringValue", "()Ljava/lang/String;");
    (*env)->DeleteLocalRef(env, cls);
    if (!method) [AwtException raiseUsingEnv:env];
    text = (*env)->CallObjectMethod(env, _peer, method);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    nativeText = javaToNSString(env, text);
    (*env)->DeleteLocalRef(env, text);
    [[NSPasteboard generalPasteboard] setString:nativeText forType:NSStringPboardType];
}
@end

static id clipboardFromPeer(JNIEnv *env, jobject peer)
{
    jclass cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RClipboard");
    jfieldID field;
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "nNativeClipboard", "I");
    (*env)->DeleteLocalRef(env, cls);
    if (!field) [AwtException raiseUsingEnv:env];
    return [(AwtClipboard *)(*env)->GetIntField(env, peer, field) appkitProxy];
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RClipboard_createNativeClipboard(JNIEnv *env, jobject peer)
{
    AwtClipboard *volatile clipboard = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        clipboard = [AwtClipboard alloc];
        if (!clipboard) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtClipboard"];
        [[clipboard appkitProxy] initWithPeer:(int)globalPeer];
        /* RToolkit caches this clipboard for the process lifetime; no dispose entry exists. */
    NS_HANDLER
        [[AwtClipboard appkitProxy] discardClipboard:(int)clipboard];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        clipboard = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)clipboard;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RClipboard_postNewContents(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = [clipboardFromPeer(env, peer) claimClipboardOwnership];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jstring JNICALL Java_com_apple_rhapsody_awt_RClipboard_loadExternalContents(JNIEnv *env, jobject peer)
{
    jstring result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = nsToJavaString(env, [clipboardFromPeer(env, peer) loadExternalContents]);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RClipboard_generalPasteboardCount(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = [clipboardFromPeer(env, peer) generalPasteboardCount];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
