/* DR2 libawt.A.dylib: offscreen images 540208dc--54022954. */
#import "awt_Image.h"
#import "awt_Graphics.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "com_apple_rhapsody_awt_RGraphics.h"
#include "java_awt_Color.h"

/* DR2 AppKit's bitmap normalization entry points, also used by the donor. */
extern BOOL _NSCanCreateEightBitMeshedBitmap(int, int, int, int, int, BOOL, BOOL, int, BOOL);
extern BOOL _NSCreateEightBitMeshedBitmap(unsigned char **, unsigned char *, int, int,
    int, int, int, BOOL, BOOL, int, int, BOOL, BOOL, BOOL);

@implementation AwtImage
- (id)initForIR:(void *)representation withCH:(void *)background
{
    Classsun_awt_image_ImageRepresentation *ir = unhand((Hsun_awt_image_ImageRepresentation *)representation);
    self = [super initWithSize:NSMakeSize(ir->width, ir->height)];
    if (!self) return nil;
    width = wide = ir->width; height = high = ir->height;
    hints = ir->hints; object = representation;
    offscreen = YES;
    color = background ? unhand((Hjava_awt_Color *)background)->value : 0;
    [self setBackgroundColor:[self awtColor]];
    [self setFlipped:YES];
    [self setCachedSeparately:YES];
    return self;
}
- (void)dealloc { rb = 0; data = NULL; [super dealloc]; }
- (NSRect)awtBounds { return NSMakeRect(0, 0, width, height); }
- (NSRect)awtVisibleBounds { return NSMakeRect(0, 0, wide, high); }
- (BOOL)awtCanDraw { return YES; }
- (BOOL)awtLockFocus { [self lockFocus]; return YES; }
- (void)awtUnlockFocus { [self unlockFocus]; }
- (NSColor *)awtColor
{
    return [NSColor colorWithDeviceRed:((color >> 16) & 255) / 255.0
        green:((color >> 8) & 255) / 255.0 blue:(color & 255) / 255.0 alpha:1];
}
- (id)drawAtPoint:(NSPoint)point
{
    point.y += height;
    [self compositeToPoint:point operation:NSCompositeSourceOver];
    return self;
}
- (id)sendPixels:(void *)source
{
    Hsun_awt_image_OffScreenImageSource *handle = source;
    ExecEnv *ee = EE();
    HObject *model;
    HArrayOfInt *pixels;
    NSBitmapImageRep *volatile capture = nil, *volatile normalized = nil;
    BOOL volatile focused = NO;
    if (!unhand(handle)->theConsumer) return nil;
    model = execute_java_constructor(ee, "java/awt/image/DirectColorModel", NULL,
        "(IIIII)", 32, 0xff0000, 0xff00, 0xff, hasAlpha ? 0xff000000 : 0);
    if (!model || exceptionOccurred(ee)) return nil;
    execute_java_dynamic_method(ee, (HObject *)unhand(handle)->theConsumer,
        "setColorModel", "(Ljava/awt/image/ColorModel;)V", model);
    if (!unhand(handle)->theConsumer || exceptionOccurred(ee)) return nil;
    execute_java_dynamic_method(ee, (HObject *)unhand(handle)->theConsumer, "setHints", "(I)V", 11);
    if (!unhand(handle)->theConsumer || exceptionOccurred(ee)) return nil;
    NS_DURING
        [self lockFocus]; focused = YES;
        capture = [[NSBitmapImageRep alloc] initWithFocusedViewRect:NSMakeRect(0, 0, wide, high)];
        [self unlockFocus]; focused = NO;
        if (_NSCanCreateEightBitMeshedBitmap(wide, high, [capture samplesPerPixel],
            [capture bitsPerSample], [capture bitsPerPixel], [capture isPlanar],
            [capture hasAlpha], [capture bytesPerRow], NO)) {
            unsigned char *planes[5] = {NULL, NULL, NULL, NULL, NULL};
            unsigned char *bytes;
            int row, stride;
            hasAlpha = [capture hasAlpha];
            normalized = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                pixelsWide:wide pixelsHigh:high bitsPerSample:8 samplesPerPixel:hasAlpha ? 4 : 3
                hasAlpha:hasAlpha isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:32];
            if (!normalized) [NSException raise:NSMallocException format:@"Cannot allocate image pixels"];
            [capture getBitmapDataPlanes:planes];
            _NSCreateEightBitMeshedBitmap(planes, [normalized bitmapData], wide, high,
                [capture samplesPerPixel], [capture bitsPerSample], [capture bitsPerPixel],
                [capture isPlanar], [capture hasAlpha], [capture bytesPerRow],
                [normalized bytesPerRow], NO, NO, NO);
            bytes = [normalized bitmapData]; stride = [normalized bytesPerRow];
            pixels = (HArrayOfInt *)ArrayAlloc(T_INT, wide);
            if (!pixels) SignalError(ee, "java/lang/OutOfMemoryError", NULL);
            else for (row = 0; row < high; row++) {
                if (!unhand(handle)->theConsumer || exceptionOccurred(ee)) break;
                /* Preserve the donor's native-endian four-byte readback. */
                memcpy(unhand(pixels)->body, bytes + row * stride, wide * 4);
                execute_java_dynamic_method(ee, (HObject *)unhand(handle)->theConsumer,
                    "setPixels", "(IIIILjava/awt/image/ColorModel;[III)V",
                    0, row, wide, 1, model, pixels, 0, wide);
            }
        }
    NS_HANDLER
        SignalError(ee, "java/lang/OutOfMemoryError", NULL);
    NS_ENDHANDLER
    if (focused) [self unlockFocus];
    [normalized release]; [capture release];
    return self;
}
@end

static void imageException(NSException *exception)
{
    SignalError(EE(), "java/lang/RuntimeException", (char *)[[exception reason] cString]);
}

void sun_awt_image_ImageRepresentation_offscreenInit(Hsun_awt_image_ImageRepresentation *self, Hjava_awt_Color *background)
{
    AwtAutoreleasePool *pool;
    AwtImage *volatile image = nil;
    if (!self || !unhand(self)) { SignalError(EE(), "java/lang/NullPointerException", NULL); return; }
    if (unhand(self)->width <= 0 || unhand(self)->height <= 0) {
        SignalError(EE(), "java/lang/IllegalAccessError", NULL); return;
    }
    pool = NativeMethodEnter(NULL);
    NS_DURING
        if (!unhand(self)->pData && (unhand(self)->availinfo & 3) == 3) {
            image = [AwtImage alloc];
            image = [image initForIR:self withCH:background];
            unhand(self)->pData = (long)image;
            image = nil;
        }
        if (unhand(self)->pData) unhand(self)->availinfo = 43;
        else SignalError(EE(), "java/lang/OutOfMemoryError", NULL);
    NS_HANDLER
        [image release];
        imageException(localException);
    NS_ENDHANDLER
    if (pool) NativeMethodExit(NULL, pool);
}

void sun_awt_image_ImageRepresentation_disposeImage(Hsun_awt_image_ImageRepresentation *self)
{
    AwtAutoreleasePool *pool;
    if (!self || !unhand(self)) { SignalError(EE(), "java/lang/NullPointerException", NULL); return; }
    pool = NativeMethodEnter(NULL);
    NS_DURING
        AwtImage *image = (AwtImage *)unhand(self)->pData;
        unhand(self)->pData = 0;
        if (image) { image->object = NULL; [image release]; }
    NS_HANDLER
        imageException(localException);
    NS_ENDHANDLER
    if (pool) NativeMethodExit(NULL, pool);
}

void sun_awt_image_ImageRepresentation_imageDraw(Hsun_awt_image_ImageRepresentation *self,
    struct Hjava_awt_Graphics *graphics, long x, long y, Hjava_awt_Color *background)
{
    AwtAutoreleasePool *pool;
    if (!self || !unhand(self) || !graphics) { SignalError(EE(), "java/lang/NullPointerException", NULL); return; }
    if ((unhand(self)->availinfo & 11) != 11) return;
    pool = NativeMethodEnter(NULL);
    NS_DURING
        AwtImage *image = (AwtImage *)unhand(self)->pData;
        AwtGraphics *target = (AwtGraphics *)unhand((Hcom_apple_rhapsody_awt_RGraphics *)graphics)->nNativeGraphics;
        [target drawImage:image atPoint:NSMakePoint(x, y)];
    NS_HANDLER
        imageException(localException);
    NS_ENDHANDLER
    if (pool) NativeMethodExit(NULL, pool);
}

void sun_awt_image_OffScreenImageSource_sendPixels(Hsun_awt_image_OffScreenImageSource *self)
{
    AwtAutoreleasePool *pool;
    AwtImage *volatile image = nil;
    Hsun_awt_image_ImageRepresentation *representation;
    if (!self || !(representation = unhand(self)->baseIR) || !unhand(representation)) {
        SignalError(EE(), "java/lang/NullPointerException", NULL); return;
    }
    if ((unhand(representation)->availinfo & 43) != 43) {
        SignalError(EE(), "java/lang/IllegalAccessError", NULL); return;
    }
    pool = NativeMethodEnter(NULL);
    NS_DURING
        image = [(AwtImage *)unhand(representation)->pData retain];
        if (image) [image sendPixels:self];
        else SignalError(EE(), "java/lang/OutOfMemoryError", NULL);
    NS_HANDLER
        imageException(localException);
    NS_ENDHANDLER
    [image release];
    if (pool) NativeMethodExit(NULL, pool);
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_ROffScreenGraphics_nCreateNativeGraphics(JNIEnv *env, jobject peer, jobject representation)
{
    AwtScreenGraphics *volatile graphics = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        jclass cls = (*env)->GetObjectClass(env, representation);
        jfieldID field = (*env)->GetFieldID(env, cls, "pData", "I");
        AwtImage *image;
        if (!field) [AwtException raiseUsingEnv:env];
        image = (AwtImage *)(*env)->GetIntField(env, representation, field);
        graphics = [AwtScreenGraphics alloc];
        graphics = [graphics initWithImage:image];
        if (!graphics) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Can't allocate AwtScreenGraphics"];
        (*env)->DeleteLocalRef(env, cls);
    NS_HANDLER
        [graphics release]; graphics = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)graphics;
}
