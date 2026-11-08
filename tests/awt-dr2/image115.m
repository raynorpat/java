#import "../../src/rhapsody/sun/awt/awt_Image.h"
#import "../../src/rhapsody/sun/awt/awt_Graphics.h"
#include "jni.h"
static int liveImages, failNext, drawingBalance;
@interface ImageGraphicsProbe : AwtScreenGraphics
@end
@implementation ImageGraphicsProbe
- (BOOL)_beginDrawingWithPaint:(BOOL)paint
{ BOOL result = [super _beginDrawingWithPaint:paint]; if (result) drawingBalance++; return result; }
- (void)_endDrawing { drawingBalance--; [super _endDrawing]; }
- (void)_abortDrawing { drawingBalance--; [super _abortDrawing]; }
@end
@interface ImageLifetimeProbe : AwtImage
@end
@implementation ImageLifetimeProbe
+ (id)allocWithZone:(NSZone *)zone { id value = [super allocWithZone:zone]; if (value) liveImages++; return value; }
- (void)dealloc { liveImages--; [super dealloc]; }
- (id)initForIR:(void *)representation withCH:(void *)background
{
    self = [super initForIR:representation withCH:background];
    if (failNext) { failNext = 0; [NSException raise:NSInvalidArgumentException format:@"image constructor failure"]; }
    return self;
}
- (id)initForIR:(void *)representation withCM:(void *)model
{
    self = [super initForIR:representation withCM:model];
    if (failNext) { failNext = 0; [NSException raise:NSInvalidArgumentException format:@"image constructor failure"]; }
    return self;
}
- (id)drawSubrect:(NSRect)source inRect:(NSRect)destination
{
    if (failNext) { failNext = 0; [NSException raise:NSInvalidArgumentException format:@"image drawing failure"]; }
    return [super drawSubrect:source inRect:destination];
}
@end
JNIEXPORT void JNICALL Java_Image115_track(JNIEnv *env, jclass cls)
{ [ImageLifetimeProbe poseAsClass:[AwtImage class]]; [ImageGraphicsProbe poseAsClass:[AwtScreenGraphics class]]; }
JNIEXPORT jint JNICALL Java_Image115_live(JNIEnv *env, jclass cls) { return liveImages; }
JNIEXPORT void JNICALL Java_Image115_failNext(JNIEnv *env, jclass cls) { failNext = 1; }
JNIEXPORT jboolean JNICALL Java_Image115_hasFocus(JNIEnv *env, jclass cls) { return drawingBalance != 0 || [NSView focusView] != nil; }
JNIEXPORT void JNICALL Java_Image115_release(JNIEnv *env, jclass cls, jobject image)
{
    jclass imageClass = (*env)->FindClass(env, "sun/awt/image/Image");
    jfieldID field = (*env)->GetFieldID(env, imageClass, "imagerep", "Lsun/awt/image/ImageRepresentation;");
    jobject representation = (*env)->GetObjectField(env, image, field);
    jclass representationClass = (*env)->GetObjectClass(env, representation);
    jmethodID dispose = (*env)->GetMethodID(env, representationClass, "disposeImage", "()V");
    (*env)->CallVoidMethod(env, representation, dispose);
}
JNIEXPORT jboolean JNICALL Java_Image115_upload(JNIEnv *env, jclass cls, jobject image, jboolean bytes,
    jint x, jint y, jint width, jint height, jobject model, jobject pixels, jint offset, jint scan)
{
    jclass imageClass = (*env)->FindClass(env, "sun/awt/image/Image");
    jmethodID getRepresentation = (*env)->GetMethodID(env, imageClass, "getImageRep", "()Lsun/awt/image/ImageRepresentation;");
    jobject representation = (*env)->CallObjectMethod(env, image, getRepresentation);
    jclass representationClass = (*env)->GetObjectClass(env, representation);
    jmethodID upload = (*env)->GetMethodID(env, representationClass, bytes ? "setBytePixels" : "setIntPixels",
        bytes ? "(IIIILjava/awt/image/ColorModel;[BII)Z" : "(IIIILjava/awt/image/ColorModel;[III)Z");
    return (*env)->CallBooleanMethod(env, representation, upload, x, y, width, height, model, pixels, offset, scan);
}

/* Inject at the actual DPS boundary, restoring the current thread's table
 * before raising. The production flush/setup paths remain under test. */
#import <AppKit/dpsclient.h>
#import "../../src/rhapsody/sun/awt/awt_ThreadContext.h"
static const DPSProcsRec *savedProcs;
static DPSProcsRec failingProcs;
static int failDrawingStage, writesUntilFailure, surfaceFocus;
static void failWrite(DPSContext context, const void *bytes, unsigned int count)
{
    if (--writesUntilFailure == 0) {
        context->procs = savedProcs;
        [NSException raise:NSInvalidArgumentException format:@"injected drawing failure"];
    }
    savedProcs->BinObjSeqWrite(context, bytes, count);
}
static void failFlush(DPSContext context)
{
    context->procs = savedProcs;
    [NSException raise:NSInvalidArgumentException format:@"injected drawing failure"];
}
@interface DrawingFailureColor : NSObject
- (void)set;
@end
@implementation DrawingFailureColor
- (void)set
{
    DPSContext context;
    [[NSColor redColor] set];
    if (failDrawingStage == 0)
        [NSException raise:NSInvalidArgumentException format:@"injected drawing failure"];
    context = DPSGetCurrentContext(); savedProcs = context->procs;
    failingProcs = *savedProcs;
    if (failDrawingStage == 3) failingProcs.FlushContext = failFlush;
    else {
        writesUntilFailure = failDrawingStage;
        failingProcs.BinObjSeqWrite = failWrite;
    }
    context->procs = &failingProcs;
}
@end
@interface FocusSurfaceProbe : NSImage
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (NSColor *)awtColor;
@end
@implementation FocusSurfaceProbe
- (BOOL)awtCanDraw { return YES; }
- (BOOL)awtLockFocus { [self lockFocus]; surfaceFocus++; return YES; }
- (void)awtUnlockFocus { surfaceFocus--; [self unlockFocus]; }
- (NSColor *)awtColor { return [NSColor whiteColor]; }
@end
JNIEXPORT jint JNICALL Java_Image115_focusExceptions(JNIEnv *env, jclass cls)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    int stage, failures = 0;
    for (stage = 0; stage < 4; stage++) {
        FocusSurfaceProbe *surface = [[FocusSurfaceProbe alloc] initWithSize:NSMakeSize(32, 32)];
        AwtScreenGraphics *graphics = [[AwtScreenGraphics alloc] initWithImage:surface];
        DrawingFailureColor *color = [[DrawingFailureColor alloc] init];
        BOOL volatile caught = NO;
        failDrawingStage = stage;
        [graphics setColor:(NSColor *)color];
        NS_DURING
            [graphics fillRect:NSMakeRect(0, 0, 16, 16)];
        NS_HANDLER
            caught = [[localException reason] isEqual:@"injected drawing failure"];
        NS_ENDHANDLER
        if (DPSGetCurrentContext()->procs == &failingProcs)
            DPSGetCurrentContext()->procs = savedProcs;
        if (!caught || surfaceFocus || [NSView focusView]) failures |= 1 << stage;
        /* Permit all stages to report independently on the unfixed library. */
        while (surfaceFocus) [surface awtUnlockFocus];
        [graphics setColor:[NSColor blueColor]];
        [graphics fillRect:NSMakeRect(0, 0, 16, 16)];
        if (surfaceFocus || [NSView focusView]) failures |= 1 << stage;
        [graphics release]; [color release]; [surface release];
    }
    if (pool) NativeMethodExit(env, pool);
    return failures;
}
