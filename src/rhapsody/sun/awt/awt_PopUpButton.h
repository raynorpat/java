#ifndef RHAPSODY_AWT_POPUPBUTTON_H
#define RHAPSODY_AWT_POPUPBUTTON_H

#import "awt_View.h"

/* DR2 fragile ABI: NSPopUpButton is 96 bytes. */
@interface AwtPopUpButton : NSPopUpButton {
    jobject _peer;
    NSColor *_backgroundColor;
    NSColor *_foregroundColor;
    NSCursor *_cursor;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetFont:(NSFont *)font;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (void)safeInsertItemWithTitle:(NSString *)title atIndex:(int)index;
- (void)popUpClicked:(id)sender;
@end

#endif
