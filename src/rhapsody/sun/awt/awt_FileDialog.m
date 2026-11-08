/* DR2 libawt.A.dylib: 54004e24--5400532c and 5401445c--54014928. */
#import "awt_FileDialog.h"
#import "awt_Component.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSSavePanel (AWTFilePackages)
- (BOOL)_isFilePackage:(NSString *)extension;
@end

@implementation AwtDummyFileDialogView
- (id)initWithPeer:(int)peer isSavePanel:(BOOL)save title:(NSString *)title file:(NSString *)file directory:(NSString *)directory wantsToFilter:(BOOL)filter
{
    self = [super init];
    _peer = (jobject)peer; _isSave = save; _wantsToFilterFiles = filter;
    _title = [title retain]; _file = [file retain]; _directory = [directory retain];
    return self;
}
- (void)dealloc { [_title release]; [_file release]; [_directory release]; [super dealloc]; }
- (void)setFile:(NSString *)file { [_file autorelease]; _file = [file retain]; }
- (void)setDirectory:(NSString *)directory { [_directory autorelease]; _directory = [directory retain]; }
- (void)setTitle:(NSString *)title { [_title autorelease]; _title = [title retain]; }
- (void)setIsSavePanel:(BOOL)save { _isSave = save; }
- (void)setWantsToFilterFiles:(BOOL)filter { _wantsToFilterFiles = filter; }
- (void)showPanel
{
    NSSavePanel *panel = _isSave ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];
    int result;
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls; jmethodID method; jstring directory, file;
    [[self retain] autorelease];
    [panel setTitle:_title];
    if (_wantsToFilterFiles) [panel setDelegate:self];
    NS_DURING
        result = [panel runModalForDirectory:_directory file:_file];
    NS_HANDLER
        [panel setDelegate:nil];
        [localException raise];
    NS_ENDHANDLER
    [panel setDelegate:nil];
    if (!_peer) return;
    directory = nsToJavaString(env, [panel directory]);
    file = nsToJavaString(env, result == 1 ? [[panel filename] lastPathComponent] : nil);
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RFileDialogPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "fileDialogDone", "(Ljava/lang/String;Ljava/lang/String;)V");
    (*env)->DeleteLocalRef(env, cls);
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, _peer, method, directory, file);
    (*env)->DeleteLocalRef(env, directory); (*env)->DeleteLocalRef(env, file);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
}
- (BOOL)_pathIsDirectoryOrAutomount:(NSString *)path
{
    NSFileManager *manager = [NSFileManager defaultManager];
    NSDictionary *attributes = [manager fileAttributesAtPath:path traverseLink:NO];
    NSString *type = [attributes fileType];
    if ([type isEqualToString:NSFileTypeSymbolicLink]) {
        if ((unsigned short)[attributes filePosixPermissions] == 0xffff) return YES;
        type = [[manager fileAttributesAtPath:path traverseLink:YES] fileType];
    }
    return [type isEqualToString:NSFileTypeDirectory];
}
- (BOOL)filterAcceptsFilename:(NSString *)path forPanel:(NSSavePanel *)panel
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls; jmethodID method; jstring directory, file; BOOL result;
    if (!_wantsToFilterFiles) return YES;
    if ([self _pathIsDirectoryOrAutomount:path] && ![panel _isFilePackage:[path pathExtension]]) return YES;
    if (!_peer) return NO;
    directory = nsToJavaString(env, [path stringByDeletingLastPathComponent]);
    file = nsToJavaString(env, [path lastPathComponent]);
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RFileDialogPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "filterAccepts", "(Ljava/lang/String;Ljava/lang/String;)Z");
    (*env)->DeleteLocalRef(env, cls);
    if (!method) [AwtException raiseUsingEnv:env];
    result = (*env)->CallBooleanMethod(env, _peer, method, directory, file);
    (*env)->DeleteLocalRef(env, directory); (*env)->DeleteLocalRef(env, file);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return result;
}
- (BOOL)panel:(id)panel isValidFilename:(NSString *)path { return [self filterAcceptsFilename:path forPanel:panel]; }
- (BOOL)panel:(id)panel shouldShowFilename:(NSString *)path { return [self filterAcceptsFilename:path forPanel:panel]; }
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_createNativeFileDialog(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring title,
    jboolean resizable, jboolean modal, jstring file, jstring directory, jboolean save, jboolean filter)
{
    AwtDummyFileDialogView *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeTitle = javaToNSString(env, title);
        NSString *nativeFile = javaToNSString(env, file);
        NSString *nativeDirectory = javaToNSString(env, directory);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtDummyFileDialogView alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtFileDialogView"];
        [[view appkitProxy] initWithPeer:(int)globalPeer isSavePanel:save title:nativeTitle file:nativeFile directory:nativeDirectory wantsToFilter:filter];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_setTitle(JNIEnv *env, jobject peer, jstring value)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setTitle:javaToNSString(env, value)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_setFile(JNIEnv *env, jobject peer, jstring value)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setFile:javaToNSString(env, value)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_setDirectory(JNIEnv *env, jobject peer, jstring value)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setDirectory:javaToNSString(env, value)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_setWantsToFilterFiles(JNIEnv *env, jobject peer, jboolean value)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setWantsToFilterFiles:value];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RFileDialogPeer_setVisible(JNIEnv *env, jobject peer, jboolean value)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        if (value) [viewFromPeer(env, peer) showPanel];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
