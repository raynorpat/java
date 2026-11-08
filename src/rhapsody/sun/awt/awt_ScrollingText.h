#ifndef RHAPSODY_AWT_SCROLLINGTEXT_H
#define RHAPSODY_AWT_SCROLLINGTEXT_H

#import "awt_TextField.h"

/* DR2 fragile ABI: NSScrollView is 120 bytes. */
@interface AwtScrollingText : NSScrollView {
    jobject _peer;
    AwtTextView *_text;
    BOOL _awtEditable;
    BOOL _awtEnabled;
    NSCursor *_cursor;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable wantsScrollers:(int)scrollers selection:(NSRange)selection;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)awtSetEditable:(BOOL)editable;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetFont:(NSFont *)font;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (NSSize)preferredSizeForNumRows:(int)rows numColumns:(int)columns;
- (void)setString:(NSString *)text;
- (NSString *)string;
- (void)insertString:(NSString *)text atPosition:(int)position;
- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)text;
- (NSRange)selectedRange;
- (void)setSelectedRange:(NSRange)range;
@end

#endif
