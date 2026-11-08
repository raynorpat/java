/* DR2 libawt.A.dylib: AwtScrollView 5400cb58--5400d6f4. */
#import "awt_ScrollView.h"
#import "awt_Scroller.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"

@interface NSScrollView (AWTScrollerAction)
- (void)_doScroller:(id)sender;
@end

@implementation AwtScrollView
/* DR2 NSScrollView creates scrollers through _newScroll: (43372b8c).
 * Preserve its geometry and setup while supplying disposal-safe tracking. */
- (id)_newScroll:(BOOL)vertical
{
    NSRect frame = vertical ? NSMakeRect(0, 0, 0, 100) : NSMakeRect(0, 0, 100, 0);
    AwtTrackingScroller *scroller = [[AwtTrackingScroller allocWithZone:[self zone]] initWithFrame:frame];
    if (vertical) [self setVerticalScroller:scroller]; else [self setHorizontalScroller:scroller];
    [scroller setArrowsPosition:NSScrollerArrowsMaxEnd];
    [scroller release];
    return self;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor scrollerPolicy:(int)policy verticalLineIncrement:(int)vertical horizontalLineIncrement:(int)horizontal
{
    self = [super initWithFrame:frame];
    _peer = (jobject)peer; _showScrollers = policy;
    if (policy == 1) { [self setHasVerticalScroller:YES]; [self setHasHorizontalScroller:YES]; }
    [self setBorderType:NSBezelBorder];
    [self setHorizontalLineScroll:horizontal]; [self setVerticalLineScroll:vertical];
    [self awtSetBackgroundColor:background]; [self awtSetForegroundColor:foreground];
    [self awtSetEnabled:enabled]; [self awtSetCursor:cursor];
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
    if (_cursor && [self isAwtEnabled]) [self addCursorRect:[self convertRect:[[self contentView] bounds] fromView:[self contentView]] cursor:_cursor];
    if (_trackingRectTag) [self removeTrackingRect:_trackingRectTag];
    _trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
- (void)awtSetEnabled:(BOOL)enabled
{
    _awtEnabled = enabled;
    [[self horizontalScroller] setEnabled:enabled];
    [[self verticalScroller] setEnabled:enabled];
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
- (NSView *)hitTest:(NSPoint)point { return [self isAwtEnabled] ? [super hitTest:point] : nil; }
- (NSSize)preferredSize { return NSMakeSize(100, 100); }
- (void)getInsetsTop:(out int *)top left:(out int *)left bottom:(out int *)bottom right:(out int *)right
{
    NSSize content = [[self class] contentSizeForFrameSize:NSMakeSize(100, 100)
        hasHorizontalScroller:NO hasVerticalScroller:NO borderType:[self borderType]];
    float y = (100 - content.height) / 2, x = (100 - content.width) / 2;
    *top = y; *left = x;
    *bottom = y + ([self hasHorizontalScroller] ? [[self class] horizontalScrollerHeight] : 0);
    *right = x + ([self hasVerticalScroller] ? [[self class] verticalScrollerWidth] : 0);
}
- (void)reflectScrolledClipView:(NSClipView *)clip
{
    NSSize content, document;
    [super reflectScrolledClipView:clip];
    content = [self contentSize]; document = [[self documentView] bounds].size;
    if (_showScrollers == 0) {
        [self setHasHorizontalScroller:document.width > content.width];
        [self setHasVerticalScroller:document.height > content.height];
        [[self horizontalScroller] setEnabled:_awtEnabled];
        [[self verticalScroller] setEnabled:_awtEnabled];
    }
}
- (void)_doScroller:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls; jmethodID method;
    [[self retain] autorelease];
    [super _doScroller:sender];
    if (!_peer) return;
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RScrollPanePeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "dispatchAdjustmentEvent", "(ZF)V");
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, _peer, method, sender == [self verticalScroller], (double)[sender floatValue]);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    if (!_peer) [(AwtTrackingScroller *)sender cancelTracking];
}
- (void)scrollTo:(NSPoint)point
{
    NSClipView *clip = [self contentView];
    [clip scrollToPoint:point]; [self reflectScrolledClipView:clip];
}
- (void)setValue:(int)value isVertical:(BOOL)vertical
{
    NSPoint point = [[self contentView] bounds].origin;
    if (vertical) point.y = value; else point.x = value;
    [self scrollTo:point];
}
- (void)setLineIncrement:(int)value isVertical:(BOOL)vertical
{ if (vertical) [self setVerticalLineScroll:value]; else [self setHorizontalLineScroll:value]; }
+ (int)horizontalScrollerHeight
{
    NSSize with = [self contentSizeForFrameSize:NSMakeSize(100, 100) hasHorizontalScroller:YES hasVerticalScroller:NO borderType:NSBezelBorder];
    NSSize without = [self contentSizeForFrameSize:NSMakeSize(100, 100) hasHorizontalScroller:NO hasVerticalScroller:NO borderType:NSBezelBorder];
    return without.height - with.height;
}
+ (int)verticalScrollerWidth
{
    NSSize with = [self contentSizeForFrameSize:NSMakeSize(100, 100) hasHorizontalScroller:NO hasVerticalScroller:YES borderType:NSBezelBorder];
    NSSize without = [self contentSizeForFrameSize:NSMakeSize(100, 100) hasHorizontalScroller:NO hasVerticalScroller:NO borderType:NSBezelBorder];
    return without.width - with.width;
}
@end

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_createNativeScrollView(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject background, jobject foreground, jobject cursor, jint policy, jint vertical, jint horizontal)
{
    AwtScrollView *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtScrollView alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtScrollView"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            backgroundColor:nativeBackground
            foregroundColor:nativeForeground cursor:nativeCursor scrollerPolicy:policy verticalLineIncrement:vertical horizontalLineIncrement:horizontal];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_setChildComponentVisible(JNIEnv *env, jobject peer, jobject child, jboolean visible)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setDocumentView:visible ? viewFromPeer(env, child) : nil];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_setScrollPosition(JNIEnv *env, jobject peer, jint x, jint y)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) scrollTo:NSMakePoint(x, y)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_setUnitIncrementForOrientation(JNIEnv *env, jobject peer, jint value, jboolean vertical)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setLineIncrement:value isVertical:vertical];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_setValueForOrientation(JNIEnv *env, jobject peer, jint value, jboolean vertical)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setValue:value isVertical:vertical];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_childResized(JNIEnv *env, jobject peer, jint width, jint height)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        /* The donor relies on native document-view frame notifications. */
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_getHScrollbarHeight(JNIEnv *env, jobject peer)
{
    int result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = [[AwtScrollView appkitProxy] horizontalScrollerHeight];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_getVScrollbarWidth(JNIEnv *env, jobject peer)
{
    int result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = [[AwtScrollView appkitProxy] verticalScrollerWidth];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}

JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RScrollPanePeer_getInsets(JNIEnv *env, jobject peer)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        int top, left, bottom, right;
        jclass cls; jmethodID method;
        [viewFromPeer(env, peer) getInsetsTop:&top left:&left bottom:&bottom right:&right];
        cls = (*env)->FindClass(env, "java/awt/Insets");
        if (!cls) [AwtException raiseUsingEnv:env];
        method = (*env)->GetMethodID(env, cls, "<init>", "(IIII)V");
        if (!method) [AwtException raiseUsingEnv:env];
        result = (*env)->NewObject(env, cls, method, top, left, bottom, right);
        (*env)->DeleteLocalRef(env, cls);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
