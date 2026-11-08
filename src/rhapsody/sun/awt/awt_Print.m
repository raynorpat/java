/* DR2 libawt.A.dylib: 54009a4c--5400a01c, JNI 54018cc4--54019a2b. */
#import "awt_Print.h"
#import "awt_Application.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@implementation AwtPrintGraphics
- (id)initWithPrintSurface:(id)surface
{ return [super initWithSurface:surface font:nil flipped:NO]; }
/* These are the donor's unfinished print operations, not screen drawing. */
- (BOOL)_beginDrawingWithPaint:(BOOL)paint { return YES; }
- (void)_endDrawing { }
- (void)printPage { }
@end

@implementation AwtPrintJob
- (id)initWithTitle:(NSString *)title disposition:(int)disposition destination:(NSString *)destination copies:(int)copies orientation:(int)orientation paperName:(NSString *)paper
{
    NSPrintInfo *info = [[[NSPrintInfo alloc] init] autorelease];
    [info setUpPrintOperationDefaultValues];
    if (disposition) [info setJobDisposition:(NSString *)disposition];
    if (destination) {
        if ([NSPrintSpoolJob isEqual:(NSString *)disposition]) {
            NSPrinter *printer = [NSPrinter printerWithName:destination];
            if (printer) [info setPrinter:printer];
        } else if ([NSPrintSaveJob isEqual:(NSString *)disposition])
            [[info dictionary] setObject:destination forKey:NSPrintSavePath];
    }
    if (copies > 0) [[info dictionary] setObject:[NSNumber numberWithInt:copies] forKey:NSPrintCopies];
    [info setOrientation:orientation];
    if (paper) [info setPaperName:paper];
    _printOp = [[NSPrintOperation printOperationWithView:nil printInfo:info] retain];
    return [super init];
}
- (void)dealloc { [_printOp release]; [super dealloc]; }
- (int)showPrintDialog
{
    NSPrintPanel *panel;
    [NSPrintPanel printPanel];
    if ([NSPrintOperation currentOperation] != _printOp) return (int)NSPrintCancelJob;
    if (![_printOp printPanel]) [_printOp setPrintPanel:[NSPrintPanel printPanel]];
    panel = [_printOp printPanel];
    if (panel) {
        [panel setHidesOnDeactivate:NO];
        [panel updateFromPrintInfo];
        [panel runModal];
        [panel finalWritePrintInfo];
    } else [[_printOp printInfo] setJobDisposition:NSPrintCancelJob];
    return (int)[[_printOp printInfo] jobDisposition];
}
- (void)cleanUp { [[_printOp printPanel] orderOut:nil]; [_printOp cleanUpOperation]; }
- (NSPrintOperation *)printOperation { return _printOp; }
+ (void)discardPrintJob:(int)address
{
    AwtPrintJob *job = (AwtPrintJob *)address;
    NS_DURING
        [job cleanUp];
    NS_HANDLER
        [job release]; [localException raise];
    NS_ENDHANDLER
    [job release];
}
@end

@implementation AwtPrintSurface
- (NSRect)awtBounds { return NSZeroRect; }
- (NSRect)awtVisibleBounds { return NSZeroRect; }
- (BOOL)awtCanDraw { return NO; }
- (BOOL)awtLockFocus { return YES; }
- (void)awtUnlockFocus { }
- (NSColor *)awtColor { return nil; }
@end

static jfieldID jobField(JNIEnv *env, jobject peer)
{
    jclass cls = (*env)->GetObjectClass(env, peer);
    jfieldID field = (*env)->GetFieldID(env, cls, "nNativePrintJob", "I");
    (*env)->DeleteLocalRef(env, cls);
    if (!field) [AwtException raiseUsingEnv:env];
    return field;
}
static AwtPrintJob *jobFromPeer(JNIEnv *env, jobject peer)
{ return (AwtPrintJob *)(*env)->GetIntField(env, peer, jobField(env, peer)); }

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RPrintControl_nInitConstants(JNIEnv *env, jclass cls)
{
    const char *names[] = { "NSPrintSpoolJob", "NSPrintSaveJob", "NSPrintCancelJob", "NSPortraitOrientation", "NSLandscapeOrientation" };
    jint values[5]; int i;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    values[0] = (jint)NSPrintSpoolJob; values[1] = (jint)NSPrintSaveJob;
    values[2] = (jint)NSPrintCancelJob; values[3] = NSPortraitOrientation; values[4] = NSLandscapeOrientation;
    NS_DURING
        for (i = 0; i < 5; i++) {
            jfieldID field = (*env)->GetStaticFieldID(env, cls, names[i], "I");
            if (!field) [AwtException raiseUsingEnv:env];
            (*env)->SetStaticIntField(env, cls, field, values[i]);
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintGraphics_nCreateNativeGraphics(JNIEnv *env, jobject peer, jint surface)
{
    AwtPrintGraphics *volatile graphics = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        graphics = [AwtPrintGraphics alloc];
        graphics = [graphics initWithPrintSurface:(id)surface];
        if (!graphics) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtPrintGraphics"];
    NS_HANDLER
        [graphics release]; graphics = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)graphics;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RPrintGraphics_nPrintPage(JNIEnv *env, jobject peer, jint address)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtPrintGraphics *)address printPage];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nCreateNativePrintJob(JNIEnv *env, jobject peer, jstring title, jint disposition, jstring destination, jint copies, jint orientation, jstring paper)
{
    AwtPrintJob *volatile job = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        job = [AwtPrintJob alloc];
        [[job appkitProxy] initWithTitle:javaToNSString(env, title) disposition:(int)disposition
            destination:javaToNSString(env, destination) copies:copies orientation:orientation paperName:javaToNSString(env, paper)];
    NS_HANDLER
        /* The donor returns an invalid job on a native construction failure. */
        [[AwtPrintJob appkitProxy] discardPrintJob:(int)job]; job = nil;
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)job;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nDispose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        jfieldID field = jobField(env, peer);
        jint address = (*env)->GetIntField(env, peer, field);
        (*env)->SetIntField(env, peer, field, 0);
        /* Releasing a proxy target through its own retained invocation double-frees it. */
        [[AwtPrintJob appkitProxy] discardPrintJob:address];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nShowPrintDialog(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = (jint)[[jobFromPeer(env, peer) appkitProxy] showPrintDialog];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nGetDisposition(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSPrintInfo *info = [[jobFromPeer(env, peer) printOperation] printInfo];
        result = (jint)[info jobDisposition];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nGetNumCopies(JNIEnv *env, jobject peer)
{
    jint result = 1;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSPrintInfo *info = [[jobFromPeer(env, peer) printOperation] printInfo];
        result = [[[info dictionary] objectForKey:NSPrintCopies] intValue];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nGetOrientation(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSPrintInfo *info = [[jobFromPeer(env, peer) printOperation] printInfo];
        result = [info orientation];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jstring JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nGetPaperName(JNIEnv *env, jobject peer)
{
    jstring result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSPrintInfo *info = [[jobFromPeer(env, peer) printOperation] printInfo];
        result = nsToJavaString(env, [info paperName]);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jstring JNICALL Java_com_apple_rhapsody_awt_RPrintJob_nGetDestinationName(JNIEnv *env, jobject peer)
{
    jstring result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSPrintInfo *info = [[jobFromPeer(env, peer) printOperation] printInfo];
        NSString *disposition = [info jobDisposition];
        if ([NSPrintSpoolJob isEqual:disposition]) result = nsToJavaString(env, [[info printer] name]);
        else if ([NSPrintSaveJob isEqual:disposition]) result = nsToJavaString(env, [[info dictionary] objectForKey:NSPrintSavePath]);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
