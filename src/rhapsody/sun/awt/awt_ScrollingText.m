/* DR2 libawt.A.dylib: AwtScrollingText 5400bdf8--5400cafc. */
#import "awt_ScrollingText.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#include <math.h>

@implementation AwtScrollingText
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable wantsScrollers:(int)scrollers selection:(NSRange)selection
{
    BOOL horizontal = scrollers == 0 || scrollers == 2;
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
    if (frame.size.width < 1) frame.size.width = 1;
    _text = [[AwtTextView alloc] initWithPeer:_peer frame:frame];
    [self setHasHorizontalScroller:horizontal];
    [_text setHorizontallyResizable:horizontal];
    [[_text textContainer] setWidthTracksTextView:!horizontal];
    [self setHasVerticalScroller:scrollers == 0 || scrollers == 1];
    [_text setVerticallyResizable:YES];
    [[_text textContainer] setHeightTracksTextView:NO];
    [_text setMaxSize:NSMakeSize(10000000, 10000000)];
    [[_text textContainer] setContainerSize:NSMakeSize(10000000, 10000000)];
    [self setBorderType:NSBezelBorder];
    [self setDocumentView:_text];
    [_text setAutoresizingMask:18];
    if (![self awtComponentGetsBackgroundFromItsContainer] || [self awtComponentUsesCustomColor])
        [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground];
    [self awtSetFont:font]; [self awtSetCursor:cursor];
    [self awtSetEnabled:enabled]; [self awtSetEditable:editable];
    [_text setString:text]; [_text setSelectedRange:selection];
    [_text setDelegate:self];
    _sFlags.hScrollerStatus = 1; _sFlags.vScrollerStatus = 1;
    [self reflectScrolledClipView:[self contentView]];
    return self;
}
- (void)dealloc { [_text release]; [_cursor release]; [super dealloc]; }
- (jobject)peer { return _peer; }
- (void)clearPeer
{
    AwtContainer *container = [self outermostJavaContainer];
    if ([container lastJavaViewEntered] == self || [container lastJavaViewEntered] == _text)
        [container setLastJavaViewEntered:nil];
    [_text setPeer:NULL]; [_text setDelegate:nil]; _peer = NULL;
}
- (BOOL)componentIsFocusTraversable { return [_text acceptsFirstResponder]; }
- (void)makeComponentTakeFocus { [[self window] makeFirstResponder:_text]; }
- (void)awtSetEnabled:(BOOL)enabled
{
    _awtEnabled = enabled;
    [self awtSetEditable:_awtEditable];
    [[self horizontalScroller] setEnabled:enabled]; [[self verticalScroller] setEnabled:enabled];
    if (_cursor) [[self window] invalidateCursorRectsForView:self];
    if (!enabled) [self resignFirstResponderIfNecessary];
}
- (BOOL)isAwtEnabled { return _awtEnabled && [[self superview] isAwtEnabled]; }
- (void)awtSetEditable:(BOOL)editable
{
    _awtEditable = editable;
    [_text setEditable:_awtEditable && _awtEnabled];
    [_text setSelectable:_awtEditable && _awtEnabled];
    if (!_awtEditable || !_awtEnabled) [self resignFirstResponderIfNecessary];
}
- (void)awtSetBackgroundColor:(NSColor *)color
{
    if (color) [_text setBackgroundColor:color];
    [_text setDrawsBackground:color != nil]; [_text setNeedsDisplay:YES];
}
- (void)awtSetForegroundColor:(NSColor *)color { [_text setTextColor:color ? color : [NSColor blackColor]]; }
- (void)awtSetFont:(NSFont *)font { if (font) [_text setFont:font]; }
- (void)awtSetCursor:(NSCursor *)cursor
{
    [_cursor autorelease]; _cursor = [cursor retain];
    [_cursor setOnMouseEntered:YES]; [[self window] invalidateCursorRectsForView:self];
}
- (void)resetCursorRects
{
    if (_cursor && [self isAwtEnabled])
        [self addCursorRect:[self convertRect:[[self contentView] bounds] fromView:[self contentView]] cursor:_cursor];
    if (_trackingRectTag) [self removeTrackingRect:_trackingRectTag];
    _trackingRectTag = [self addTrackingRect:[self bounds] owner:self userData:NULL assumeInside:NO];
}
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type >= 100 && type <= 101) return YES;
    if (type < 1 || type > 12) return NO;
    if (type >= 5 && type <= 7 && !_wantsMouseMotionEvents) return NO;
    return [self isAwtEnabled];
}
- (NSView *)hitTest:(NSPoint)point { return [self isAwtEnabled] ? [super hitTest:point] : nil; }
- (BOOL)isFlipped { return YES; }
- (NSSize)preferredSizeForNumRows:(int)rows numColumns:(int)columns
{
    NSFont *font = [_text font];
    NSSize size;
    NSRect line;
    BOOL empty;
    if ([[_text layoutManager] usesScreenFonts] && [font screenFont]) font = [font screenFont];
    size.width = [font advancementForGlyph:48].width * columns + 2 * [[_text textContainer] lineFragmentPadding];
    empty = [[_text textStorage] length] == 0;
    if (empty) [_text setString:@" "];
    line = [[_text layoutManager] lineFragmentRectForGlyphAtIndex:0 effectiveRange:NULL];
    size.height = line.size.height * rows;
    if (empty) [_text setString:@""];
    return [[self class] frameSizeForContentSize:size hasHorizontalScroller:[self hasHorizontalScroller]
        hasVerticalScroller:[self hasVerticalScroller] borderType:[self borderType]];
}
- (void)setString:(NSString *)text { [_text setString:text]; [_text setNeedsDisplay:YES]; }
- (NSString *)string { return [_text string]; }
- (void)insertString:(NSString *)text atPosition:(int)position
{
    [_text replaceCharactersInRange:NSMakeRange(position, 0) withString:text];
    [_text scrollRangeToVisible:NSMakeRange(position, [text length])]; [_text setNeedsDisplay:YES];
}
- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)text
{ [_text replaceCharactersInRange:range withString:text]; [_text setNeedsDisplay:YES]; }
- (NSRange)selectedRange { return [_text selectedRange]; }
- (void)setSelectedRange:(NSRange)range { [_text setSelectedRange:range]; }
- (void)textDidChange:(NSNotification *)notification
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jmethodID method;
    if (!_peer) return;
    [[self retain] autorelease];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RTextComponentPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    method = (*env)->GetMethodID(env, cls, "dispatchTextChangedEvent", "()V");
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, _peer, method);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
}
- (void)awtContainerChangedBackground
{
    if ([self awtComponentGetsBackgroundFromItsContainer])
        [self awtSetBackgroundColor:[(AwtView *)[self superview] awtBackgroundColor]];
}
@end

/* 5401a838--5401b10c: unchanged RTextAreaPeer bindings. */
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_createNativeTextArea(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring text, jboolean editable, jint scrollers, jint start, jint end)
{
    AwtScrollingText *volatile view = nil;
    jobject volatile globalPeer = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSString *nativeText = javaToNSString(env, text);
        NSCursor *nativeCursor = javaToNSCursor(env, cursor);
        NSColor *nativeForeground = javaToNSColor(env, foreground);
        NSColor *nativeBackground = javaToNSColor(env, background);
        NSFont *nativeFont = javaToNSFont(env, font);
        globalPeer = (*env)->NewGlobalRef(env, peer);
        if (!globalPeer) [AwtException raiseUsingEnv:env];
        view = [AwtScrollingText alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtScrollingText"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor text:nativeText editable:editable wantsScrollers:scrollers selection:NSMakeRange(start, end - start)];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_setEditable(JNIEnv *env, jobject peer, jboolean editable)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetEditable:editable];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_setText(JNIEnv *env, jobject peer, jstring text)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setString:javaToNSString(env, text)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_nSelect(JNIEnv *env, jobject peer, jint start, jint end)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setSelectedRange:NSMakeRange(start, end - start)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_nInsert(JNIEnv *env, jobject peer, jstring text, jint position)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) insertString:javaToNSString(env, text) atPosition:position];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_nReplaceRange(JNIEnv *env, jobject peer, jstring text, jint start, jint end)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) replaceCharactersInRange:NSMakeRange(start, end - start) withString:javaToNSString(env, text)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jstring JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_getText(JNIEnv *env, jobject peer)
{
    jstring result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = nsToJavaString(env, [viewFromPeer(env, peer) string]);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_getSelectionStart(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSRange range = [viewFromPeer(env, peer) selectedRange];
        result = range.location;
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_getSelectionEnd(JNIEnv *env, jobject peer)
{
    jint result = 0;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSRange range = [viewFromPeer(env, peer) selectedRange];
        result = NSMaxRange(range);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RTextAreaPeer_getPreferredSize(JNIEnv *env, jobject peer, jint rows, jint columns)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSSize size = [viewFromPeer(env, peer) preferredSizeForNumRows:rows numColumns:columns];
        jclass cls = (*env)->FindClass(env, "java/awt/Dimension");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        /* 5401aa9c requests upward rounding from nsSizeToJavaDimension. */
        result = (*env)->NewObject(env, cls, ctor, (int)ceil(size.width), (int)ceil(size.height));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
