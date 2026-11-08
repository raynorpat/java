#ifndef RHAPSODY_AWT_GRAPHICS_H
#define RHAPSODY_AWT_GRAPHICS_H
#import "awt_Canvas.h"

@interface AwtGraphics : NSObject {
    id _surface;
    NSRect _surfaceBounds;
    NSColor *_color;
    NSColor *_xorColor;
    int _xorWindowNum;
    BOOL _inXorDrawingMode;
    NSFont *_font;
    float _fontPointSize;
    NSDictionary *_textAttributes;
    BOOL _translated;
    NSPoint _origin;
    BOOL _clipped;
    BOOL _flipped;
    NSRect _clip;
    unsigned int _dirtyBits;
}
- (id)initWithSurface:(id)surface font:(NSFont *)font flipped:(BOOL)flipped;
- (BOOL)_beginDrawingWithPaint:(BOOL)paint;
- (void)_endDrawing;
- (void)setOrigin:(NSPoint)origin;
- (void)setColor:(NSColor *)color;
- (void)setPaintMode;
- (void)setXORMode:(NSColor *)color;
- (void)setFont:(NSFont *)font;
- (void)setClip:(NSRect)clip;
- (void)unsetClip;
- (void)drawRect:(NSRect)rect;
- (void)fillRect:(NSRect)rect;
- (void)clearRect:(NSRect)rect;
- (void)copyArea:(NSRect)rect offset:(NSPoint)offset;
- (void)drawLineFromPoint:(NSPoint)start toPoint:(NSPoint)end;
- (void)strokeRoundRect:(NSRect)rect arcWidth:(float)width arcHeight:(float)height andFill:(BOOL)fill;
- (void)strokeOval:(NSRect)rect andFill:(BOOL)fill;
- (void)strokeArc:(NSRect)rect startAngle:(float)start arcAngle:(float)angle andFill:(BOOL)fill;
- (void)strokePolyLine:(NSData *)points count:(int)count andClose:(BOOL)close andFill:(BOOL)fill;
- (void)drawString:(NSString *)string atPoint:(NSPoint)point;
- (void)drawImage:(id)image atPoint:(NSPoint)point;
- (void)drawImage:(id)image fromRect:(NSRect)source toRect:(NSRect)destination;
@end
@interface AwtScreenGraphics : AwtGraphics {
    unsigned int _flags;
    BOOL _surfaceIsCanvas;
}
- (id)initWithCanvas:(AwtCanvas *)canvas;
- (id)initWithImage:(id)image;
@end
#endif
