#ifndef RHAPSODY_AWT_GRAPHICS_H
#define RHAPSODY_AWT_GRAPHICS_H
#import "awt_Canvas.h"

@interface AwtGraphics : NSObject {
    id _surface;
    NSColor *_color;
    NSFont *_font;
    NSPoint _origin;
    NSRect _clip;
    BOOL _flipped;
    BOOL _translated;
    BOOL _clipped;
    unsigned int _dirtyBits;
}
- (id)initWithSurface:(id)surface font:(NSFont *)font flipped:(BOOL)flipped;
- (BOOL)_beginDrawingWithPaint:(BOOL)paint;
- (void)_endDrawing;
- (void)setOrigin:(NSPoint)origin;
- (void)setColor:(NSColor *)color;
- (void)setFont:(NSFont *)font;
- (void)setClip:(NSRect)clip;
- (void)unsetClip;
- (void)drawRect:(NSRect)rect;
- (void)fillRect:(NSRect)rect;
- (void)clearRect:(NSRect)rect;
@end
@interface AwtScreenGraphics : AwtGraphics
- (id)initWithCanvas:(AwtCanvas *)canvas;
@end
#endif
