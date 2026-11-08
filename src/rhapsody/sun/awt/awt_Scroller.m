/* DR2 libawt.A.dylib: AwtScroller 5400a1d8--5400ab60. */
#import "awt_Scroller.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSWindow (AWTScrollerTracking)
- (BOOL)_hasCursorRects;
@end

static NSString *trackingCancelled = @"AwtScrollerTrackingCancelled";

@implementation AwtTrackingScroller
/* DR2 tracking loops keep fetching from self->_window. Disposing a peer
 * detaches that window, so unwind the loop before it starts polling nil. */
- (void)mouseDown:(NSEvent *)event
{
    NSWindow *window = [[self window] retain];
    BOOL hadCursorRects = [window _hasCursorRects];
    [[self retain] autorelease];
    NS_DURING
        [super mouseDown:event];
    NS_HANDLER
        if ([[localException name] isEqual:trackingCancelled]) {
            [NSEvent stopPeriodicEvents];
            _hitPart = NSScrollerNoPart;
            if (hadCursorRects) [window enableCursorRects];
        } else {
            [window release];
            [localException raise];
        }
    NS_ENDHANDLER
    [window release];
}
- (void)cancelTracking
{ [NSException raise:trackingCancelled format:@"Disposed scrollbar ended native tracking"]; }
@end

@implementation AwtScroller
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor isVertical:(BOOL)vertical minValue:(int)minimum maxValue:(int)maximum value:(int)value visibleAmount:(int)visible lineIncrement:(int)line pageIncrement:(int)page
{
    _isVertical = vertical;
    if (vertical && frame.size.height <= frame.size.width) frame.size.height = frame.size.width + 1;
    if (!vertical && frame.size.width <= frame.size.height) frame.size.width = frame.size.height + 1;
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
    [self awtSetEnabled:enabled];
    if (![self awtComponentGetsBackgroundFromItsContainer]) [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground]; [self awtSetCursor:cursor];
    _lineIncrement = line; _pageIncrement = page;
    [self setValue:value minValue:minimum maxValue:maximum visibleAmount:visible];
    [self setTarget:self]; [self setAction:@selector(handleScrollerAction:)];
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
{ [_backgroundColor autorelease]; _backgroundColor = [color retain]; }
- (void)awtSetForegroundColor:(NSColor *)color
{ [_foregroundColor autorelease]; _foregroundColor = [color retain]; }
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
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)setLineIncrement:(int)increment { _lineIncrement = increment; }
- (void)setPageIncrement:(int)increment { _pageIncrement = increment; }
- (void)setValue:(int)value minValue:(int)minimum maxValue:(int)maximum visibleAmount:(int)visible
{
    int range = maximum - visible - minimum;
    _range = range < 1 ? 1 : range; _minimum = minimum;
    [self setFloatValue:(float)(value - minimum) / _range knobProportion:(float)visible / (maximum - minimum)];
}
- (void)handleScrollerAction:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jmethodID method;
    int type = 0, value;
    float delta = 0;
    if (!_peer) return;
    switch ([sender hitPart]) {
        case 0: return;
        case 1: delta = -_pageIncrement / _range; type = 3; break;
        case 2: case 6: type = 5; break;
        case 3: delta = _pageIncrement / _range; type = 4; break;
        case 4: delta = -_lineIncrement / _range; type = 2; break;
        case 5: delta = _lineIncrement / _range; type = 1; break;
    }
    [[self retain] autorelease];
    [sender setFloatValue:[sender floatValue] + delta];
    value = (int)([sender floatValue] * _range + _minimum + 0.5);
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RScrollbarPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "dispatchAdjustmentEvent", "(II)V");
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, _peer, method, type, value);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    if (!_peer) [self cancelTracking];
}
+ (float)scrollerWidth { return [super scrollerWidth] + 4; }
- (NSSize)preferredSize
{ float width = [[self class] scrollerWidth]; return _isVertical ? NSMakeSize(width, 50) : NSMakeSize(50, width); }
- (NSRect)rectForPart:(NSScrollerPart)part
{
    NSRect saved = [self bounds], result;
    _bounds = NSInsetRect(saved, 2, 2);
    result = [super rectForPart:part];
    _bounds = saved;
    return result;
}
- (void)drawRect:(NSRect)rect
{
    NSRect bounds = [self bounds];
    NSDrawDarkBezel(bounds, bounds);
    [super drawRect:NSInsetRect(rect, -1, -1)];
    [[NSColor blackColor] set]; NSFrameRect(NSInsetRect(bounds, 1, 1));
}
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RScrollbarPeer_createNativeScroller(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject background, jobject foreground, jobject cursor, jboolean vertical,
    jint minimum, jint maximum, jint value, jint visibleAmount, jint line, jint page)
{
    AwtScroller *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtScroller alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtScroller"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            backgroundColor:nativeBackground
            foregroundColor:nativeForeground cursor:nativeCursor isVertical:vertical minValue:minimum
            maxValue:maximum value:value visibleAmount:visibleAmount lineIncrement:line pageIncrement:page];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollbarPeer_setValues(JNIEnv *env, jobject peer, jint value, jint visible, jint minimum, jint maximum)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setValue:value minValue:minimum maxValue:maximum visibleAmount:visible];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollbarPeer_setLineIncrement(JNIEnv *env, jobject peer, jint increment)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setLineIncrement:increment];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollbarPeer_setPageIncrement(JNIEnv *env, jobject peer, jint increment)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setPageIncrement:increment];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
