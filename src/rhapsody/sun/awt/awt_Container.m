/* DR2 libawt.A.dylib: AwtContainer 54004214--540044f4. */
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_ThreadContext.h"
#import "awt_Exception.h"

static void rectPath(NSRect rect)
{
    PSmoveto(rect.origin.x, rect.origin.y);
    PSrlineto(rect.size.width, 0);
    PSrlineto(0, rect.size.height);
    PSrlineto(-rect.size.width, 0);
    PSclosepath();
}

@implementation AwtContainer
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
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
    AwtContainer *container = [self outermostJavaContainer];
    if ([[container lastJavaViewEntered] isDescendantOf:view])
        [container setLastJavaViewEntered:nil];
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

/* 54013578--54013648: suppress redraws while Java lays out a container. */
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RContainerPeer_beginValidate(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [[viewFromPeer(env, peer) window] setAutodisplay:NO];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RContainerPeer_endValidate(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSWindow *window = [viewFromPeer(env, peer) window];
        [window setAutodisplay:YES];
        [window setViewsNeedDisplay:YES];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
