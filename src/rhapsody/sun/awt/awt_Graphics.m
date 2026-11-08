/* DR2 libawt.A.dylib: state/primitives/text 54006b48--540078e4,
 * screen focus 5400a028--5400a1d4; JNI 54016144--54017270. */
#import "awt_Graphics.h"
#import "awt_Image.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#import <AppKit/psops.h>
#import <AppKit/dpsclient.h>

@implementation AwtGraphics
- (id)initWithSurface:(id)surface font:(NSFont *)font flipped:(BOOL)flipped
{
    self = [super init];
    _surface = [surface retain];
    [self setColor:[surface awtColor]];
    [self setFont:font];
    _flipped = flipped;
    return self;
}
- (void)dealloc
{
    [_surface release];
    [_color release];
    [_xorColor release];
    [_font release];
    [_textAttributes release];
    [super dealloc];
}
/* The donor base is abstract; screen/print subclasses acquire drawing state. */
- (BOOL)_beginDrawingWithPaint:(BOOL)paint { return NO; }
- (void)_endDrawing { }
- (void)setOrigin:(NSPoint)origin
{
    if (!NSEqualPoints(_origin, origin)) {
        _origin = origin;
        _dirtyBits |= 8;
        _translated = origin.x != 0 || origin.y != 0;
    }
}
- (void)setColor:(NSColor *)color
{
    if (!color) color = [NSColor blackColor];
    if (_color != color) {
        [_color release];
        _color = [color retain];
        _dirtyBits |= 0x21;
    }
}
- (void)setFont:(NSFont *)font
{
    if (!font) font = [NSFont systemFontOfSize:12];
    if (_font != font) {
        [_font release];
        _font = [font retain];
        _fontPointSize = [font pointSize];
        _dirtyBits |= 0x22;
    }
}
- (void)setPaintMode
{
    if (_xorColor) {
        [_xorColor release];
        _xorColor = nil;
        _dirtyBits |= 0x24;
    }
}
- (void)setXORMode:(NSColor *)color
{
    if (_xorColor != color) {
        [_xorColor release];
        _xorColor = [color retain];
        _dirtyBits |= 0x24;
    }
}
- (void)setClip:(NSRect)clip
{
    if (!_clipped || !NSEqualRects(_clip, clip)) {
        _clip = clip;
        _clipped = YES;
        _dirtyBits |= 0x10;
    }
}
- (void)unsetClip
{
    if (_clipped) { _clipped = NO; _dirtyBits |= 0x10; }
}
- (void)drawRect:(NSRect)rect
{
    if ([self _beginDrawingWithPaint:YES]) {
        PSrectstroke(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        [self _endDrawing];
    }
}
- (void)fillRect:(NSRect)rect
{
    if ([self _beginDrawingWithPaint:YES]) {
        PSrectfill(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        [self _endDrawing];
    }
}
- (void)clearRect:(NSRect)rect
{
    if ([self _beginDrawingWithPaint:YES]) {
        NSColor *background = [_surface awtColor];
        if (![background isEqual:_color]) { [background set]; _dirtyBits |= 1; }
        PSrectfill(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        [self _endDrawing];
    }
}
- (void)copyArea:(NSRect)rect offset:(NSPoint)offset
{
    rect = NSIntersectionRect(rect, NSOffsetRect([_surface awtVisibleBounds], -_origin.x, -_origin.y));
    if ([self _beginDrawingWithPaint:NO]) {
        PScomposite(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height,
                    1, rect.origin.x + offset.x, rect.origin.y + offset.y, 1);
        [self _endDrawing];
    }
}
- (void)drawLineFromPoint:(NSPoint)start toPoint:(NSPoint)end
{
    if ([self _beginDrawingWithPaint:YES]) {
        PSnewpath();
        PSmoveto(start.x, start.y);
        PSlineto(end.x, end.y);
        PSstroke();
        [self _endDrawing];
    }
}
- (void)strokeRoundRect:(NSRect)rect arcWidth:(float)width arcHeight:(float)height andFill:(BOOL)fill
{
    if (width > rect.size.width || height > rect.size.height) return;
    if ([self _beginDrawingWithPaint:YES]) {
        if (width > 0 && height > 0) {
            float x, y;
            if (fill) { rect.size.width--; rect.size.height--; }
            x = 2 * rect.size.width / width;
            y = 2 * rect.size.height / height;
            PSgsave();
            PStranslate(rect.origin.x + 0.5, rect.origin.y + 0.5);
            PSscale(width / 2, height / 2);
            PSnewpath();
            PSmoveto(x / 2, 0);
            PSarct(x, 0, x, y, 1);
            PSarct(x, y, 0, y, 1);
            PSarct(0, y, 0, 0, 1);
            PSarct(0, 0, x, 0, 1);
            PSclosepath();
            PSscale(2 / width, 2 / height);
            if (fill) PSfill(); else PSstroke();
            PSgrestore();
        } else if (fill) [self fillRect:rect];
        else [self drawRect:rect];
        [self _endDrawing];
    }
}
- (void)strokeOval:(NSRect)rect andFill:(BOOL)fill
{
    [self strokeArc:rect startAngle:0 arcAngle:360 andFill:fill];
}
- (void)strokeArc:(NSRect)rect startAngle:(float)start arcAngle:(float)angle andFill:(BOOL)fill
{
    if (angle != 0 && [self _beginDrawingWithPaint:YES]) {
        float x, y;
        if (start < 0) start += 360;
        if (angle < 0) {
            start += angle;
            if (start < 0) start += 360;
            angle = -angle;
        }
        if (fill) { rect.size.width--; rect.size.height--; }
        x = rect.size.width / 2; y = rect.size.height / 2;
        PSgsave();
        PStranslate(rect.origin.x + x + 0.5, rect.origin.y + y + 0.5);
        PSscale(x > 0 ? x : 0.25, y > 0 ? y : 0.25);
        PSnewpath();
        if (fill) PSmoveto(0, 0);
        PSarcn(0, 0, 1, -start, -start - angle);
        if (fill) PSclosepath();
        PSscale(x > 0 ? 1 / x : 4, y > 0 ? 1 / y : 4);
        if (fill) PSfill(); else PSstroke();
        PSgrestore();
        [self _endDrawing];
    }
}
- (void)strokePolyLine:(NSData *)points count:(int)count andClose:(BOOL)close andFill:(BOOL)fill
{
    if (count > 0 && [self _beginDrawingWithPaint:YES]) {
        const NSPoint *point = [points bytes];
        int index;
        PSgsave();
        PStranslate(0.5, 0.5);
        PSnewpath();
        PSmoveto(point[0].x, point[0].y);
        for (index = 0; index < count; index++) PSlineto(point[index].x, point[index].y);
        if (close) PSclosepath();
        if (fill) PSeofill(); else PSstroke();
        PSgrestore();
        [self _endDrawing];
    }
}
- (void)drawString:(NSString *)string atPoint:(NSPoint)point
{
    if (string && [self _beginDrawingWithPaint:YES]) {
        if (_dirtyBits & 0x20) {
            [_textAttributes release];
            _textAttributes = [[NSDictionary dictionaryWithObjectsAndKeys:_font, NSFontAttributeName,
                _xorColor ? _xorColor : _color, NSForegroundColorAttributeName, nil] retain];
            _dirtyBits &= ~0x20;
        }
        if (_flipped) { PSscale(1, -1); point.y = -point.y; }
        point.y -= _fontPointSize;
        [string drawAtPoint:point withAttributes:_textAttributes];
        if (_flipped) PSscale(1, -1);
        [self _endDrawing];
    }
}
- (void)drawImage:(id)image atPoint:(NSPoint)point
{
    if (image && [self _beginDrawingWithPaint:YES]) {
        [(AwtImage *)image drawAtPoint:point];
        [self _endDrawing];
    }
}
@end

@implementation AwtScreenGraphics
- (id)initWithImage:(id)image
{
    return [super initWithSurface:image font:nil flipped:NO];
}
- (id)initWithCanvas:(AwtCanvas *)canvas
{
    self = [super initWithSurface:canvas font:[canvas awtFont] flipped:NO];
    _surfaceIsCanvas = YES;
    [canvas allocateGState];
    return self;
}
- (BOOL)_beginDrawingWithPaint:(BOOL)paint
{
    if (![_surface awtCanDraw]) return NO;
    if ([_surface awtLockFocus]) _dirtyBits |= 0x19;
    if (paint && (_dirtyBits & 1)) {
        if (_xorColor) {
            DPSPrintf(DPSGetCurrentContext(), "1 setrasterop\n");
            [_xorColor set];
        } else [_color set];
        _dirtyBits &= ~1;
    }
    if (_translated && (_dirtyBits & 8)) {
        PStranslate(_origin.x, _origin.y);
        _dirtyBits &= ~8;
    }
    if (_clipped && (_dirtyBits & 0x10)) { NSRectClip(_clip); _dirtyBits &= ~0x10; }
    return YES;
}
- (void)_endDrawing
{
    PSflushgraphics();
    PSFlush();
    [_surface awtUnlockFocus];
}
@end

/* 54019a38: the Java peer passes its raw native view address. */
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RScreenGraphics_nCreateNativeGraphics(JNIEnv *env, jobject peer, jint address)
{
    AwtScreenGraphics *graphics = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        AwtCanvas *canvas = (AwtCanvas *)address;
        if ([canvas isKindOfClass:[AwtCanvas class]]) {
            graphics = [[AwtScreenGraphics alloc] initWithCanvas:canvas];
            if (!graphics) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Can't allocate AwtScreenGraphics"];
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)graphics;
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetOrigin(JNIEnv *env, jobject peer, jint address, jint x, jint y)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setOrigin:NSMakePoint(x, y)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetColor(JNIEnv *env, jobject peer, jint address, jobject color)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setColor:javaToNSColor(env, color)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetFont(JNIEnv *env, jobject peer, jint address, jobject font)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setFont:javaToNSFont(env, font)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetPaintMode(JNIEnv *env, jobject peer, jint address)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setPaintMode];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetXORMode(JNIEnv *env, jobject peer, jint address, jobject color)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setXORMode:javaToNSColor(env, color)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nSetClip(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address setClip:NSMakeRect(x, y, width, height)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nRemoveClip(JNIEnv *env, jobject peer, jint address)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address unsetClip];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nDrawRect(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address drawRect:NSMakeRect(x, y, width, height)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nFillRect(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address fillRect:NSMakeRect(x, y, width, height)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nClearRect(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address clearRect:NSMakeRect(x, y, width, height)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nDispose(JNIEnv *env, jobject peer, jint address)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address release];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nCopyArea(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height, jint dx, jint dy)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address copyArea:NSMakeRect(x, y, width, height) offset:NSMakePoint(dx, dy)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nDrawLine(JNIEnv *env, jobject peer, jint address, jint x1, jint y1, jint x2, jint y2)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address drawLineFromPoint:NSMakePoint(x1, y1) toPoint:NSMakePoint(x2, y2)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nStrokeRoundRect(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height, jint arcWidth, jint arcHeight, jboolean fill)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address strokeRoundRect:NSMakeRect(x, y, width, height) arcWidth:arcWidth arcHeight:arcHeight andFill:fill];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nStrokeOval(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height, jboolean fill)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address strokeOval:NSMakeRect(x, y, width, height) andFill:fill];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nStrokeArc(JNIEnv *env, jobject peer, jint address, jint x, jint y, jint width, jint height, jint start, jint angle, jboolean fill)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address strokeArc:NSMakeRect(x, y, width, height) startAngle:start arcAngle:angle andFill:fill];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nDrawString(JNIEnv *env, jobject peer, jint address, jstring text, jint x, jint y)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [(AwtGraphics *)address drawString:javaToNSString(env, text) atPoint:NSMakePoint(x, y)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RGraphics_nStrokePoly(JNIEnv *env, jobject peer, jint address, jintArray xs, jintArray ys, jint count, jboolean close, jboolean fill)
{
    jint *volatile x = NULL, *volatile y = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSMutableData *data;
        NSPoint *points;
        int index;
        if (!xs || !ys)
            [AwtException raise:env exceptionName:"java/lang/NullPointerException" reason:"Null polygon array"];
        if (count < 0 || count > (*env)->GetArrayLength(env, xs) || count > (*env)->GetArrayLength(env, ys))
            [AwtException raise:env exceptionName:"java/lang/ArrayIndexOutOfBoundsException" reason:"Polygon array range"];
        data = [NSMutableData dataWithLength:count * sizeof(NSPoint)];
        if (!data) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate polygon points"];
        if (count) {
            x = (*env)->GetIntArrayElements(env, xs, NULL);
            if (!x) [AwtException raiseUsingEnv:env];
            y = (*env)->GetIntArrayElements(env, ys, NULL);
            if (!y) [AwtException raiseUsingEnv:env];
            points = [data mutableBytes];
            for (index = 0; index < count; index++) points[index] = NSMakePoint(x[index], y[index]);
            (*env)->ReleaseIntArrayElements(env, xs, x, JNI_ABORT); x = NULL;
            (*env)->ReleaseIntArrayElements(env, ys, y, JNI_ABORT); y = NULL;
        }
        [(AwtGraphics *)address strokePolyLine:data count:count andClose:close andFill:fill];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (x) (*env)->ReleaseIntArrayElements(env, xs, x, JNI_ABORT);
    if (y) (*env)->ReleaseIntArrayElements(env, ys, y, JNI_ABORT);
    if (pool) NativeMethodExit(env, pool);
}
