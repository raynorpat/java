/* DR2 libawt.A.dylib: graphics state 54006b48--54006f28,
 * rectangles 54007080--540071b4, screen focus 5400a028--5400a1d4. */
#import "awt_Graphics.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#import <AppKit/psops.h>

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
    [_font release];
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
        _dirtyBits |= 0x22;
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
@end

@implementation AwtScreenGraphics
- (id)initWithCanvas:(AwtCanvas *)canvas
{
    self = [super initWithSurface:canvas font:[canvas awtFont] flipped:NO];
    [canvas allocateGState];
    return self;
}
- (BOOL)_beginDrawingWithPaint:(BOOL)paint
{
    if (![_surface awtCanDraw]) return NO;
    if ([_surface awtLockFocus]) _dirtyBits |= 0x19;
    if (paint && (_dirtyBits & 1)) { [_color set]; _dirtyBits &= ~1; }
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
