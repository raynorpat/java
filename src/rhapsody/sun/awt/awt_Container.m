/* DR2 libawt.A.dylib: AwtContainer 54004214--540044f4. */
#import "awt_Container.h"

static void rectPath(NSRect rect)
{
    PSmoveto(rect.origin.x, rect.origin.y);
    PSrlineto(rect.size.width, 0);
    PSrlineto(0, rect.size.height);
    PSrlineto(-rect.size.width, 0);
    PSclosepath();
}

@implementation AwtContainer
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    _clipSubviews = NO;
    return [super initWithPeer:peer frame:frame enabled:enabled font:font
        backgroundColor:background foregroundColor:foreground cursor:cursor];
}
- (void)_clipAroundSubviews
{
    NSRect outer = NSInsetRect([self bounds], -1, -1);
    NSEnumerator *views = [[self subviews] objectEnumerator];
    NSView *view;
    while ((view = [views nextObject])) {
        /* 5401fdb8: two closed rectangles followed by even-odd clipping. */
        PSnewpath();
        rectPath(outer);
        rectPath([view frame]);
        PSeoclip();
    }
}
- (BOOL)awtLockFocus
{
    BOOL result = [super awtLockFocus];
    if (result && _clipSubviews) [self _clipAroundSubviews];
    return result;
}
- (void)didAddSubview:(NSView *)view { _clipSubviews = YES; }
- (void)willRemoveSubview:(NSView *)view
{
    if ([[self subviews] count] == 1) _clipSubviews = NO;
}
- (void)notifyAwtWindowOfNativeWidgetAddition:(id)window
{
    [[self subviews] makeObjectsPerformSelector:_cmd withObject:window];
}
- (void)notifyAwtWindowOfNativeWidgetRemoval:(id)window
{
    [[self subviews] makeObjectsPerformSelector:_cmd withObject:window];
}
- (NSView *)hitTest:(NSPoint)point
{
    return [self isAwtEnabled] ? [super hitTest:point] : nil;
}
- (NSView *)lastJavaViewEntered { return _lastViewEntered; }
- (void)setLastJavaViewEntered:(NSView *)view { _lastViewEntered = view; }
@end
