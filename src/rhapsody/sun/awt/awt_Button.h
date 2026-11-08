#ifndef RHAPSODY_AWT_BUTTON_H
#define RHAPSODY_AWT_BUTTON_H

#import "awt_View.h"

/* DR2 fragile ABI: NSButton is 92 bytes; peer/color/cursor start at 92/96/100. */
@interface AwtButton : NSButton {
    jobject _peer;
    NSColor *_foregroundColor;
    NSCursor *_cursor;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
    BOOL _isCheckBoxButton;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title;
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title;
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title state:(BOOL)state inGroup:(BOOL)inGroup;
- (void)awtSetState:(BOOL)state;
- (void)awtSetInGroup:(BOOL)inGroup;
- (void)checkboxClicked:(id)sender;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetTitle:(NSString *)title;
- (void)awtSetFont:(NSFont *)font;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (void)buttonClicked:(id)sender;
@end

#endif
