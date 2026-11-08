#ifndef RHAPSODY_AWT_SCROLLVIEW_H
#define RHAPSODY_AWT_SCROLLVIEW_H
#import "awt_View.h"

/* DR2 fragile ABI: NSScrollView is 120 bytes. */
@interface AwtScrollView : NSScrollView {
    jobject _peer;
    NSColor *_backgroundColor;
    NSColor *_foregroundColor;
    NSCursor *_cursor;
    int _showScrollers;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor scrollerPolicy:(int)policy verticalLineIncrement:(int)vertical horizontalLineIncrement:(int)horizontal;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (void)getInsetsTop:(out int *)top left:(out int *)left bottom:(out int *)bottom right:(out int *)right;
- (void)scrollTo:(NSPoint)point;
- (void)setValue:(int)value isVertical:(BOOL)vertical;
- (void)setLineIncrement:(int)value isVertical:(BOOL)vertical;
+ (int)horizontalScrollerHeight;
+ (int)verticalScrollerWidth;
@end
#endif
