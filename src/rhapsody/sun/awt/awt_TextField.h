#ifndef RHAPSODY_AWT_TEXTFIELD_H
#define RHAPSODY_AWT_TEXTFIELD_H

#import "awt_View.h"

/* DR2 fragile ABI: NSTextField is 100 bytes. */
@interface AwtTextField : NSTextField {
    jobject _peer;
    NSCursor *_cursor;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
    BOOL _isLabel;
}
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text;
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text alignment:(int)alignment;
- (jobject)peer;
- (void)clearPeer;
- (void)awtSetAlignment:(int)alignment;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetFont:(NSFont *)font;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
@end

#endif
