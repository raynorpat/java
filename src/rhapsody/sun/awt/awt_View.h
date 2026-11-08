#ifndef RHAPSODY_AWT_VIEW_H
#define RHAPSODY_AWT_VIEW_H

#import <AppKit/AppKit.h>
#include "jni.h"

@interface NSView (AWTViewState)
- (BOOL)isAwtEnabled;
- (id)outermostJavaContainer;
- (void)resignFirstResponderIfNecessary;
- (void)awtContainerChangedBackground;
- (BOOL)dispatchesAwtEventsOfType:(int)type;
@end

@interface NSWindow (AWTViewTracking)
- (NSView *)deepestViewUnderLocation:(NSPoint)location dispatchingEventsOfType:(int)type;
@end

@interface AwtView : NSView {
    jobject _peer;
    NSColor *_backgroundColor;
    NSColor *_foregroundColor;
    NSFont *_font;
    NSCursor *_cursor;
    BOOL _awtEnabled;
    int _trackingRectTag;
    BOOL _wantsMouseMotionEvents;
}
/* NSConnection must transmit the 32-bit JNI handle as a value, not a pointer. */
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor;
- (jobject)peer;
- (NSColor *)awtBackgroundColor;
- (NSColor *)awtForegroundColor;
- (NSFont *)awtFont;
- (void)awtSetBackgroundColor:(NSColor *)color;
- (void)awtSetForegroundColor:(NSColor *)color;
- (void)awtSetFont:(NSFont *)font;
- (NSCursor *)cursor;
- (void)awtSetCursor:(NSCursor *)cursor;
- (void)awtSetEnabled:(BOOL)enabled;
- (BOOL)awtViewIsFrame;
- (void)setWantsMouseMotionEvents:(BOOL)enabled;
- (BOOL)dispatchesAwtEventsOfType:(int)type;
- (void)notifyAwtWindowOfNativeWidgetAddition:(id)window;
- (void)notifyAwtWindowOfNativeWidgetRemoval:(id)window;
@end

#endif
