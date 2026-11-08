/* DR2 libawt.A.dylib: AwtView state at 5400ed94--5400f51c. */
#import "awt_View.h"

/* 54010b20: hitTest accepts coordinates in the content view's superview. */
@implementation NSWindow (AWTViewTracking)
- (NSView *)deepestViewUnderLocation:(NSPoint)location dispatchingEventsOfType:(int)type
{
    NSView *content = [self contentView];
    NSView *view = [content hitTest:[[content superview] convertPoint:location fromView:nil]];
    while (view && ![view dispatchesAwtEventsOfType:type]) view = [view superview];
    return view;
}
@end

@implementation NSView (AWTViewState)
- (BOOL)isAwtEnabled
{
    return ![self superview] || [[self superview] isAwtEnabled];
}
- (id)outermostJavaContainer
{
    NSView *view;
    id result = nil;
    Class container = NSClassFromString(@"AwtContainer");
    for (view = self; view; view = [view superview])
        if ([view isKindOfClass:container]) result = view;
    return result;
}
- (void)resignFirstResponderIfNecessary
{
    NSWindow *window = [self window];
    id responder = [window firstResponder];
    if ([responder isKindOfClass:[NSView class]] && [responder isDescendantOf:self])
        [window makeFirstResponder:[window contentView]];
}
- (void)awtContainerChangedBackground { }
- (BOOL)dispatchesAwtEventsOfType:(int)type { return NO; }
@end

@implementation AwtView
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithFrame:frame];
    _peer = peer;
    _awtEnabled = enabled;
    _font = [font retain];
    _backgroundColor = [background retain];
    _foregroundColor = [foreground retain];
    [self awtSetCursor:cursor];
    return self;
}
- (void)dealloc
{
    [_backgroundColor release];
    [_foregroundColor release];
    [_font release];
    [_cursor release];
    [super dealloc];
}
- (jobject)peer { return _peer; }
- (NSColor *)awtBackgroundColor
{
    if (!_backgroundColor && [[self superview] isKindOfClass:[AwtView class]])
        return [(AwtView *)[self superview] awtBackgroundColor];
    return _backgroundColor;
}
- (NSColor *)awtForegroundColor
{
    if (!_foregroundColor && [[self superview] isKindOfClass:[AwtView class]])
        return [(AwtView *)[self superview] awtForegroundColor];
    return _foregroundColor;
}
- (NSFont *)awtFont
{
    if (!_font && [[self superview] isKindOfClass:[AwtView class]])
        return [(AwtView *)[self superview] awtFont];
    return _font;
}
- (void)awtSetBackgroundColor:(NSColor *)color
{
    [_backgroundColor autorelease];
    _backgroundColor = [color retain];
    [[self subviews] makeObjectsPerformSelector:@selector(awtContainerChangedBackground)];
}
- (void)awtSetForegroundColor:(NSColor *)color
{
    [_foregroundColor autorelease];
    _foregroundColor = [color retain];
}
- (void)awtSetFont:(NSFont *)font
{
    [_font autorelease];
    _font = [font retain];
}
- (BOOL)awtViewIsFrame { return self == [self outermostJavaContainer]; }
- (NSCursor *)cursor { return _cursor; }
- (void)awtSetCursor:(NSCursor *)cursor
{
    NSWindow *window;
    NSView *view;
    [_cursor autorelease];
    _cursor = [cursor retain];
    window = [self window];
    view = [window deepestViewUnderLocation:[window mouseLocationOutsideOfEventStream] dispatchingEventsOfType:8];
    if ([view isKindOfClass:[AwtView class]] && window == [view window]) [_cursor set];
}
- (void)resetCursorRects
{
    if (_cursor && [self isAwtEnabled] && [self awtViewIsFrame])
        [self addCursorRect:[self bounds] cursor:_cursor];
    if (_trackingRectTag) [self removeTrackingRect:_trackingRectTag];
    _trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
- (void)awtSetEnabled:(BOOL)enabled
{
    _awtEnabled = enabled;
    if (_cursor && [self awtViewIsFrame]) [[self window] invalidateCursorRectsForView:self];
    [self resetCursorRects];
    if (!enabled) [self resignFirstResponderIfNecessary];
}
- (BOOL)isAwtEnabled
{
    if (self == [[self window] contentView]) return _awtEnabled;
    return _awtEnabled && [[self superview] isAwtEnabled];
}
- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque { return YES; }
- (void)notifyAwtWindowOfNativeWidgetAddition:(id)window { }
- (void)notifyAwtWindowOfNativeWidgetRemoval:(id)window { }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type >= 100 && type <= 101) return YES;
    if (type < 1 || type > 12) return NO;
    if (type >= 5 && type <= 7 && !_wantsMouseMotionEvents) return NO;
    return [self isAwtEnabled];
}
- (BOOL)acceptsFirstResponder { return [self isAwtEnabled]; }
@end
