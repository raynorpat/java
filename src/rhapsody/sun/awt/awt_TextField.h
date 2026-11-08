#ifndef RHAPSODY_AWT_TEXTFIELD_H
#define RHAPSODY_AWT_TEXTFIELD_H

#import "awt_View.h"

/* DR2 NSTextView is 84 bytes; the editor borrows its control's peer. */
@interface AwtTextView : NSTextView {
    jobject _peer;
    BOOL _wantsMouseMotionEvents;
}
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame;
- (jobject)peer;
- (void)setPeer:(jobject)peer;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
@end

@interface AwtTextFieldCell : NSTextFieldCell
@end

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
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable echoChar:(unsigned short)echo;
+ (id)sharedFieldEditorForWindow:(NSWindow *)window;
- (id)getFieldEditor;
- (void)awtSetEditable:(BOOL)editable;
- (void)setEchoChar:(unsigned short)echo;
- (void)selectRange:(NSRange)range;
- (NSRange)selectedRange;
- (NSSize)preferredSizeForNumColumns:(int)columns;
- (void)returnHit:(id)sender;
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
