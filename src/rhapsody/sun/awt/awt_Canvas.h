#ifndef RHAPSODY_AWT_CANVAS_H
#define RHAPSODY_AWT_CANVAS_H

#import "awt_View.h"

@interface NSView (AWTBackground)
- (BOOL)awtComponentGetsBackgroundFromItsContainer;
- (BOOL)awtComponentUsesCustomColor;
@end

@interface AwtCanvas : AwtView
- (NSRect)awtBounds;
- (NSRect)awtVisibleBounds;
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (NSColor *)awtColor;
@end

#endif
