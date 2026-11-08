/* DR2 libawt.A.dylib: AwtPopUpButton 540093a0--540099f0. */
#import "awt_PopUpButton.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSButtonCell (AWTPopUpBackground)
- (void)_setBackgroundColor:(NSColor *)color;
@end

@implementation AwtPopUpButton
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor
{
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
    [self awtSetEnabled:enabled];
    if (![self awtComponentGetsBackgroundFromItsContainer]) [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground];
    [self awtSetFont:font];
    [self awtSetCursor:cursor];
    [self setTarget:self]; [self setAction:@selector(popUpClicked:)];
    return self;
}
- (void)dealloc
{
    [_backgroundColor release];
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
- (void)awtSetBackgroundColor:(NSColor *)color
{
    [_backgroundColor autorelease]; _backgroundColor = [color retain];
    [[self cell] _setBackgroundColor:color];
}
- (void)awtSetForegroundColor:(NSColor *)color
{ [_foregroundColor autorelease]; _foregroundColor = [color retain]; }
- (void)awtSetFont:(NSFont *)font { if (font) [self setFont:font]; }
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
/* Insert an empty title first so NSPopUpButton preserves duplicate labels. */
- (void)safeInsertItemWithTitle:(NSString *)title atIndex:(int)index
{ [self insertItemWithTitle:@"" atIndex:index]; [[self itemAtIndex:index] setTitle:title]; }
- (void)popUpClicked:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jmethodID method;
    jobject event;
    if (!_peer) return;
    [[self retain] autorelease];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RChoicePeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "makeItemEvent", "(I)Ljava/awt/event/ItemEvent;");
    if (!method) [AwtException raiseUsingEnv:env];
    event = (*env)->CallObjectMethod(env, _peer, method, [self indexOfSelectedItem]);
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

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RChoicePeer_createNativeChoice(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor)
{
    AwtPopUpButton *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        NSFont *nativeFont = javaToNSFont(env, font);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtPopUpButton alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtPopUpButton"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RChoicePeer_add(JNIEnv *env, jobject peer, jstring text, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) safeInsertItemWithTitle:javaToNSString(env, text) atIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RChoicePeer_remove(JNIEnv *env, jobject peer, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) removeItemAtIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RChoicePeer_select(JNIEnv *env, jobject peer, jint index)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) selectItemAtIndex:index];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
