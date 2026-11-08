/* DR2 libawt.A.dylib: offscreen images 540208dc--54022954. */
#import "awt_Image.h"
#import "awt_Graphics.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "com_apple_rhapsody_awt_RGraphics.h"
#include "java_awt_Color.h"
#include "java_awt_image_IndexColorModel.h"
#include "java_awt_image_DirectColorModel.h"

static int colorModelKind(void *model)
{
    const char *name;
    if (!model) return 0;
    name = cbName(obj_classblock((HObject *)model));
    if (!strcmp(name, "java/awt/image/IndexColorModel")) return 1;
    if (!strcmp(name, "java/awt/image/DirectColorModel")) return 2;
    return 0;
}

/* DR2 AppKit's bitmap normalization entry points, also used by the donor. */
extern BOOL _NSCanCreateEightBitMeshedBitmap(int, int, int, int, int, BOOL, BOOL, int, BOOL);
extern BOOL _NSCreateEightBitMeshedBitmap(unsigned char **, unsigned char *, int, int,
    int, int, int, BOOL, BOOL, int, int, BOOL, BOOL, BOOL);

@implementation AwtImage
- (id)initForIR:(void *)representation withCM:(void *)model
{
    Classsun_awt_image_ImageRepresentation *ir = unhand((Hsun_awt_image_ImageRepresentation *)representation);
    self = [super initWithSize:NSMakeSize(ir->width, ir->height)];
    if (!self) return nil;
    width = ir->width; height = ir->height;
    wide = ir->srcW; high = ir->srcH;
    hints = ir->hints; object = representation;
    if (colorModelKind(model) == 1) hasAlpha = !unhand((Hjava_awt_image_IndexColorModel *)model)->opaque;
    else if (colorModelKind(model) == 2) hasAlpha = unhand((Hjava_awt_image_DirectColorModel *)model)->alpha_mask != 0;
    [self setCachedSeparately:YES];
    return self;
}
- (void *)getDataAndRowBytes:(int *)rowBytes
{
    if (!data) {
        NSBitmapImageRep *bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
            pixelsWide:wide pixelsHigh:high bitsPerSample:8 samplesPerPixel:hasAlpha ? 4 : 3
            hasAlpha:hasAlpha isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:32];
        if (!bitmap) [NSException raise:NSMallocException format:@"Cannot allocate image pixels"];
        data = [bitmap bitmapData]; rb = [bitmap bytesPerRow];
        [self addRepresentation:bitmap];
        [self setDataRetained:YES];
        [bitmap release];
    }
    *rowBytes = rb;
    return data;
}
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
    if (!offscreen && object) {
        Classsun_awt_image_ImageRepresentation *ir = unhand((Hsun_awt_image_ImageRepresentation *)object);
        if (data && !(ir->hints & 16) && (ir->availinfo & 43) == 43) {
            [self setDataRetained:NO];
            newDataReceived = NO; [self recache];
            rb = 0; data = NULL;
        } else if ((data && !(ir->hints & 16) && (ir->availinfo & 27) == 27) ||
                   (newDataReceived && (!data || (ir->hints & 16)))) {
            newDataReceived = NO; [self recache];
        }
    }
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

static AwtImage *imageForRepresentation(Hsun_awt_image_ImageRepresentation *self, void *model)
{
    AwtImage *volatile image = (AwtImage *)unhand(self)->pData;
    if (!image && (unhand(self)->availinfo & 3) == 3) {
        NS_DURING
            image = [AwtImage alloc];
            image = [image initForIR:self withCM:model];
            unhand(self)->pData = (long)image;
        NS_HANDLER
            [image release];
            [localException raise];
        NS_ENDHANDLER
    }
    return image;
}

static unsigned int rgbaWord(unsigned int argb)
{
    return ((argb >> 16) & 255) | (argb & 0xff00) | ((argb & 255) << 16) | (argb & 0xff000000);
}

static long setImagePixels(Hsun_awt_image_ImageRepresentation *self, long x, long y, long width, long height,
    struct Hjava_awt_image_ColorModel *model, HArrayOfByte *pixels, long offset, long scan, BOOL bytes)
{
    AwtAutoreleasePool *pool;
    AwtImage *volatile image = nil;
    long result = 0;
    long long last;
    if (!self || !unhand(self) || !model || !pixels) {
        SignalError(EE(), "java/lang/NullPointerException", NULL); return 0;
    }
    if (!width || !height) return 0;
    if (width < 0 || height < 0) { SignalError(EE(), "java/lang/IllegalArgumentException", NULL); return 0; }
    last = (long long)offset + (long long)(height - 1) * scan;
    if (offset < 0 || last < 0 || (long long)offset + width > obj_length(pixels) || last + width > obj_length(pixels)) {
        SignalError(EE(), "java/lang/ArrayIndexOutOfBoundsException", NULL); return 0;
    }
    pool = NativeMethodEnter(NULL);
    NS_DURING
        int stride, row, column, kind = colorModelKind(model);
        unsigned char *data;
        image = [imageForRepresentation(self, model) retain];
        if (!image) SignalError(EE(), "java/lang/OutOfMemoryError", NULL);
        else if (x < 0 || y < 0 || x > image->wide || y > image->high ||
                 width > image->wide - x || height > image->high - y)
            SignalError(EE(), "java/lang/ArrayIndexOutOfBoundsException", NULL);
        else {
            data = [image getDataAndRowBytes:&stride];
            for (row = 0; row < height && !exceptionOccurred(EE()); row++) {
                unsigned int *target = (unsigned int *)(data + (y + row) * stride) + x;
                for (column = 0; column < width; column++) {
                    long index = offset + row * scan + column;
                    unsigned int value = bytes ? (unsigned char)unhand(pixels)->body[index] :
                        (unsigned int)unhand((HArrayOfInt *)pixels)->body[index];
                    if (kind == 1) {
                        Classjava_awt_image_IndexColorModel *cm = unhand((Hjava_awt_image_IndexColorModel *)model);
                        unsigned int rgb = 0;
                        if (value < obj_length(cm->rgb) && value != (unsigned int)cm->transparent_index)
                            rgb = unhand(cm->rgb)->body[value];
                        target[column] = rgbaWord(rgb);
                    } else if (kind == 2) {
                        Classjava_awt_image_DirectColorModel *cm = unhand((Hjava_awt_image_DirectColorModel *)model);
                        if (!bytes && cm->red_mask == 0xff0000 && cm->green_mask == 0xff00 && cm->blue_mask == 255) {
                            if (!cm->alpha_mask) target[column] = rgbaWord(value & 0xffffff);
                            else if ((unsigned int)cm->alpha_mask == 0xff000000)
                                target[column] = value & 0xff000000 ? rgbaWord(value) : 0;
                        } else {
                            unsigned int red = (value & cm->red_mask) >> cm->red_offset;
                            unsigned int green = (value & cm->green_mask) >> cm->green_offset;
                            unsigned int blue = (value & cm->blue_mask) >> cm->blue_offset;
                            unsigned int alpha = (value & cm->alpha_mask) >> cm->alpha_offset;
                            if (cm->red_scale) red = 255 * red / cm->red_scale;
                            if (cm->green_scale) green = 255 * green / cm->green_scale;
                            if (cm->blue_scale) blue = 255 * blue / cm->blue_scale;
                            if (cm->alpha_scale) alpha = 255 * alpha / cm->alpha_scale;
                            target[column] = alpha | (blue << 8) | (green << 16) | (red << 24);
                        }
                    } else {
                        target[column] = do_execute_java_method(EE(), model, "getRGB", "(I)I", NULL, FALSE, value);
                        if (exceptionOccurred(EE())) break;
                    }
                }
            }
            image->newDataReceived = YES;
            if (!exceptionOccurred(EE())) result = width * height;
        }
    NS_HANDLER
        imageException(localException);
    NS_ENDHANDLER
    [image release];
    if (pool) NativeMethodExit(NULL, pool);
    return result;
}

long sun_awt_image_ImageRepresentation_setBytePixels(Hsun_awt_image_ImageRepresentation *self,
    long x, long y, long width, long height, struct Hjava_awt_image_ColorModel *model, HArrayOfByte *pixels, long offset, long scan)
{
    return setImagePixels(self, x, y, width, height, model, pixels, offset, scan, YES);
}
long sun_awt_image_ImageRepresentation_setIntPixels(Hsun_awt_image_ImageRepresentation *self,
    long x, long y, long width, long height, struct Hjava_awt_image_ColorModel *model, HArrayOfInt *pixels, long offset, long scan)
{
    return setImagePixels(self, x, y, width, height, model, (HArrayOfByte *)pixels, offset, scan, NO);
}
long sun_awt_image_ImageRepresentation_finish(Hsun_awt_image_ImageRepresentation *self, long force)
{
    AwtAutoreleasePool *pool;
    if (!self || !unhand(self)) { SignalError(EE(), "java/lang/NullPointerException", NULL); return 0; }
    pool = NativeMethodEnter(NULL);
    NS_DURING
        /* Donor finish: only obtains the image; no further processing. */
        imageForRepresentation(self, NULL);
    NS_HANDLER
        imageException(localException);
    NS_ENDHANDLER
    if (pool) NativeMethodExit(NULL, pool);
    return 0;
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
