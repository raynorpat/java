/* DR2 libawt.A.dylib: AwtButton 540033a0--54003d08; peer 54010fd4/5401121c. */
#import "awt_Button.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSButtonCell (AWTBackgroundColor)
- (void)_setBackgroundColor:(NSColor *)color;
@end

/* 54010024: controls derive preferred geometry from their cell. */
@implementation NSControl (AWTPreferredSize)
- (NSSize)preferredSize { return [self cell] ? [[self cell] cellSize] : [super preferredSize]; }
@end

@implementation AwtButton
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor title:(NSString *)title
{
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
    [self awtSetEnabled:enabled];
    [self awtSetFont:font];
    [self awtSetCursor:cursor];
    [self awtSetTitle:title];
    [self setTarget:self];
    [self setBezelStyle:1];
    [self setAction:@selector(buttonClicked:)];
    _isCheckBoxButton = NO;
    if (![self awtComponentGetsBackgroundFromItsContainer]) [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground];
    return self;
}
- (void)dealloc
{
    [_foregroundColor release];
    [_cursor release];
    [super dealloc];
}
- (jobject)peer { return _peer; }
- (void)clearPeer
{
    AwtContainer *container = [self outermostJavaContainer];
    if ([container lastJavaViewEntered] == self) [container setLastJavaViewEntered:nil];
    _peer = NULL;
    [self setTarget:nil];
}
- (void)awtSetBackgroundColor:(NSColor *)color { [[self cell] _setBackgroundColor:color]; }
- (void)awtSetForegroundColor:(NSColor *)color
{
    [_foregroundColor autorelease];
    _foregroundColor = [color retain];
    [self awtSetTitle:[self title]];
}
- (void)awtSetTitle:(NSString *)title
{
    [self setTitle:title];
    if (_foregroundColor) {
        NSMutableAttributedString *attributed = [[[self attributedTitle] mutableCopy] autorelease];
        [attributed addAttribute:NSForegroundColorAttributeName value:_foregroundColor range:NSMakeRange(0, [title length])];
        [self setAttributedTitle:attributed];
    }
}
- (void)awtSetFont:(NSFont *)font
{
    if (font) [self setFont:font];
    [self awtSetTitle:[self title]];
}
- (void)awtSetCursor:(NSCursor *)cursor
{
    [_cursor autorelease];
    _cursor = [cursor retain];
    [_cursor setOnMouseEntered:YES];
    [[self window] invalidateCursorRectsForView:self];
}
- (void)resetCursorRects
{
    if (_cursor && [self isAwtEnabled]) [self addCursorRect:[self bounds] cursor:_cursor];
    if (_trackingRectTag) [self removeTrackingRect:_trackingRectTag];
    _trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
- (void)awtSetEnabled:(BOOL)enabled
{
    _awtEnabled = enabled;
    [self setEnabled:enabled];
    if (_cursor) [[self window] invalidateCursorRectsForView:self];
    if (!enabled) [self resignFirstResponderIfNecessary];
}
- (BOOL)isAwtEnabled { return _awtEnabled && [[self superview] isAwtEnabled]; }
- (BOOL)isFlipped { return YES; }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type >= 100 && type <= 101) return YES;
    if (type < 1 || type > 12) return NO;
    if (type >= 5 && type <= 7 && !_wantsMouseMotionEvents) return NO;
    return [self isAwtEnabled];
}
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
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)awtContainerChangedBackground
{
    if (_isCheckBoxButton && [self awtComponentGetsBackgroundFromItsContainer])
        [self awtSetBackgroundColor:[(AwtView *)[self superview] awtBackgroundColor]];
}
- (void)buttonClicked:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jmethodID method;
    jobject event;
    if (!_peer) return;
    [[self retain] autorelease];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RButtonPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "makeActionEvent", "()Ljava/awt/event/ActionEvent;");
    if (!method) [AwtException raiseUsingEnv:env];
    event = (*env)->CallObjectMethod(env, _peer, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_DURING
        [NSApp dispatchJavaEvent:event isLocalRef:YES];
    NS_HANDLER
        (*env)->DeleteLocalRef(env, event);
        [localException raise];
    NS_ENDHANDLER
    (*env)->DeleteLocalRef(env, event);
}
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RButtonPeer_createNativeButton(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring title)
{
    AwtButton *volatile button = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeTitle = javaToNSString(env, title);
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        NSFont *nativeFont = javaToNSFont(env, font);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        button = [AwtButton alloc];
        if (!button) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtButton"];
        [[button appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor title:nativeTitle];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)button];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        button = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)button;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RButtonPeer_setLabel(JNIEnv *env, jobject peer, jstring title)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetTitle:javaToNSString(env, title)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
