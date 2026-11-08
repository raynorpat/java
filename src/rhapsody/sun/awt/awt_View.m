/* DR2 libawt.A.dylib: AwtView state at 5400ed94--5400f51c. */
#import "awt_View.h"
#import "awt_Event.h"
#import "awt_Application.h"
#import "awt_Exception.h"
#import "awt_Container.h"

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
/* 5400f754--5400f850: common geometry and responder operations. */
- (NSSize)preferredSize { return [self bounds].size; }
- (NSPoint)upperLeftInScreenCoords
{
    NSRect bounds = [self bounds];
    NSPoint point = bounds.origin;
    if (![self isFlipped]) point.y += bounds.size.height;
    return [[self window] convertBaseToScreen:[self convertPoint:point toView:nil]];
}
- (BOOL)componentIsFocusTraversable { return [self acceptsFirstResponder]; }
- (void)makeComponentTakeFocus { [[self window] makeFirstResponder:self]; }
/* 5400f63c: scroll documents stay at the clip origin; invalidate old/new areas. */
- (void)setFrame:(NSRect)frame display:(BOOL)display
{
    if ([[[self superview] superview] isKindOfClass:[NSScrollView class]])
        frame.origin = NSMakePoint(0, 0);
    if (display) [[self superview] setNeedsDisplayInRect:[self frame]];
    [self setFrame:frame];
    if (display) [self setNeedsDisplay:YES];
}
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
/* 5400fd10: old-event components always request motion; new-event
 * components need an enabled mask, listener, or lightweight dispatcher. */
- (void)updateMouseMotionEventStatus
{
    JNIEnv *env = [NSApp appkitEnv];
    jobject source = javaComponentForView(env, self), listener = NULL;
    jclass cls = (*env)->FindClass(env, "java/awt/Component");
    jfieldID field;
    BOOL wants = YES;
    jlong mask;
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "newEventsOnly", "Z");
    if (!field) [AwtException raiseUsingEnv:env];
    if ((*env)->GetBooleanField(env, source, field)) {
        field = (*env)->GetFieldID(env, cls, "eventMask", "J");
        if (!field) [AwtException raiseUsingEnv:env];
        mask = (*env)->GetLongField(env, source, field);
        field = (*env)->GetFieldID(env, cls, "mouseMotionListener", "Ljava/awt/event/MouseMotionListener;");
        if (!field) [AwtException raiseUsingEnv:env];
        listener = (*env)->GetObjectField(env, source, field);
        (*env)->DeleteLocalRef(env, cls);
        cls = (*env)->FindClass(env, "java/awt/Container");
        if (!cls) [AwtException raiseUsingEnv:env];
        if (!listener && (*env)->IsInstanceOf(env, source, cls)) {
            field = (*env)->GetFieldID(env, cls, "dispatcher", "Ljava/awt/LightweightDispatcher;");
            if (!field) [AwtException raiseUsingEnv:env];
            listener = (*env)->GetObjectField(env, source, field);
        }
        wants = (mask & 0x20) != 0 || listener != NULL;
    }
    (*env)->DeleteLocalRef(env, listener);
    (*env)->DeleteLocalRef(env, source);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    [(AwtView *)self setWantsMouseMotionEvents:wants];
    [[self window] setAcceptsMouseMovedEvents:wants];
}
/* 5400fbe8: derived events follow non-consumed native input. */
- (void)sendAdditionalJavaEventsFor:(NSEvent *)event
{
    int type = [event type];
    if (type == NSKeyDown || (type == NSLeftMouseUp && [event clickCount])) {
        JNIEnv *env = [NSApp appkitEnv];
        jobject derived = type == NSKeyDown ? makeKeyTypedEvent(env, event, self) : makeMouseClickedEvent(env, event, self);
        if (!derived) return;
        NS_DURING
            [NSApp dispatchJavaEvent:derived isLocalRef:YES];
        NS_HANDLER
            (*env)->DeleteLocalRef(env, derived);
            [localException raise];
        NS_ENDHANDLER
        (*env)->DeleteLocalRef(env, derived);
    } else if (type == NSLeftMouseDown || type == NSRightMouseDown)
        [self updateMouseMotionEventStatus];
}
- (void)awtContainerChangedBackground { }
/* 5400f9a8: convert and dispatch input on behalf of a native view. */
- (void)dispatchNSEventToPeer:(NSEvent *)event
{
    JNIEnv *env = [NSApp appkitEnv];
    jobject source, javaEvent;
    if (![(AwtView *)self peer]) return;
    source = javaComponentForView(env, self);
    javaEvent = [event type] >= NSKeyDown ? nsKeyEventToJavaEvent(env, event, source) :
        nsMouseEventToJavaEvent(env, event, source, self);
    (*env)->DeleteLocalRef(env, source);
    if (javaEvent) {
        NS_DURING
            [NSApp dispatchJavaEvent:javaEvent isLocalRef:YES];
        NS_HANDLER
            (*env)->DeleteLocalRef(env, javaEvent);
            [localException raise];
        NS_ENDHANDLER
        (*env)->DeleteLocalRef(env, javaEvent);
    }
}
/* 5400fa6c: stale tracking notifications describe the CURRENT pointer hit. */
- (void)dispatchEnterExitPair:(NSEvent *)event
{
    NSWindow *window = [self window];
    NSPoint point = [window mouseLocationOutsideOfEventStream];
    AwtContainer *container = [self outermostJavaContainer];
    AwtView *next = (AwtView *)[window deepestViewUnderLocation:point dispatchingEventsOfType:NSMouseEntered];
    AwtView *previous = (AwtView *)[container lastJavaViewEntered];
    NSCursor *cursor;
    if (next == previous) return;
    /* Java listeners may dispose either target during the paired callbacks. */
    [[container retain] autorelease];
    [[previous retain] autorelease];
    [[next retain] autorelease];
    [event updateLocationTo:point];
    [previous dispatchNSEventToPeer:event];
    cursor = [[[NSCursor currentCursor] retain] autorelease];
    [next dispatchNSEventToPeer:event];
    if (![next peer] || [next window] != window) next = nil;
    [container setLastJavaViewEntered:next];
    [next updateMouseMotionEventStatus];
    if ([next isKindOfClass:[AwtView class]] && cursor == [NSCursor currentCursor])
        [[next cursor] set];
}
/* 5400fcb8: append to the native queue, preserving responder event order. */
- (void)dispatchFocusEventGained:(BOOL)gained temporary:(BOOL)temporary
{
    JNIEnv *env = [NSApp appkitEnv];
    if (env && [(AwtView *)self peer])
        [NSApp postEvent:makeWrappedFocusEvent(env, self, gained, temporary) atStart:NO];
}
- (BOOL)dispatchesAwtEventsOfType:(int)type { return NO; }
@end

@implementation AwtView
+ (void)discardView:(int)address
{
    AwtView *view = (AwtView *)address;
    [view clearPeer];
    [view resignFirstResponderIfNecessary];
    [view removeFromSuperview];
    [view release];
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
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
- (void)clearPeer
{
    AwtContainer *container = [self outermostJavaContainer];
    if ([container lastJavaViewEntered] == self) [container setLastJavaViewEntered:nil];
    _peer = NULL;
}
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
/* 5400f5d4/5400f5f8: either native tracking notification refreshes the pair. */
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
/* 5400f51c, 5400f578: only successful native transitions notify Java. */
- (BOOL)becomeFirstResponder
{
    BOOL result = [super becomeFirstResponder];
    if (result) [self dispatchFocusEventGained:YES temporary:NO];
    return result;
}
- (BOOL)resignFirstResponder
{
    BOOL result = [super resignFirstResponder];
    if (result) [self dispatchFocusEventGained:NO temporary:NO];
    return result;
}
@end
