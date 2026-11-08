#ifndef RHAPSODY_AWT_SCROLLER_H
#define RHAPSODY_AWT_SCROLLER_H

#import "awt_View.h"

/* Native tracking must unwind when a Java callback disposes its control. */
@interface AwtTrackingScroller : NSScroller
- (void)cancelTracking;
@end

/* DR2 fragile ABI: NSScroller is 120 bytes. */
@interface AwtScroller : AwtTrackingScroller {
    jobject _peer;
    NSColor *_backgroundColor;
    NSColor *_foregroundColor;
    NSCursor *_cursor;
    float _range;
    BOOL _isVertical;
    int _pageIncrement;
    int _lineIncrement;
    int _minimum;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor isVertical:(BOOL)vertical minValue:(int)minimum maxValue:(int)maximum value:(int)value visibleAmount:(int)visible lineIncrement:(int)line pageIncrement:(int)page;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (void)setLineIncrement:(int)increment;
- (void)setPageIncrement:(int)increment;
- (void)setValue:(int)value minValue:(int)minimum maxValue:(int)maximum visibleAmount:(int)visible;
- (void)handleScrollerAction:(id)sender;
@end

#endif
