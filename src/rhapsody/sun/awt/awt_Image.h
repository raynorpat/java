#ifndef RHAPSODY_AWT_IMAGE_H
#define RHAPSODY_AWT_IMAGE_H
#import <AppKit/AppKit.h>

@interface AwtImage : NSImage {
@public
    int width, height, wide, high, hints, rb;
    void *object;
    unsigned char *data;
    int color;
    unsigned int hasAlpha:1, offscreen:1, reserved:30;
    BOOL newDataReceived;
}
- (id)initForIR:(void *)representation withCH:(void *)background;
- (id)sendPixels:(void *)source;
- (id)drawAtPoint:(NSPoint)point;
- (NSRect)awtBounds;
- (NSRect)awtVisibleBounds;
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (NSColor *)awtColor;
@end
#endif
