/* DR2 libawt.A.dylib: label path of AwtTextField, 5400db48--5400e604. */
#import "awt_TextField.h"
#import "awt_Application.h"
#import "awt_Canvas.h"
#import "awt_Container.h"
#import "awt_Component.h"
#import "awt_Conversions.h"
#import "awt_Exception.h"
#import "awt_ThreadContext.h"
#include <math.h>

@interface NSCell (AWTEditingAccess)
- (void)_setControlView:(NSView *)view;
@end

@interface NSInputManager (AWTInputServerAccess)
- (void)_notifyInputServerIfNeeded;
@end
@interface NSInputManager (AWTClientLifetime)
- (void)awtForgetClient:(id)client;
@end
@implementation NSInputManager (AWTClientLifetime)
- (void)awtForgetClient:(id)client
{
    /* DR2 setEnabled: (4335d49c) cannot clear a client after its key window
     * has disappeared. 43348098 owns the retained _lastClient reference. */
    if (_currentClient == client) {
        _currentClient = nil;
        _flags &= ~0x2001U;
        _flags |= 0x100000U;
    }
    if (_lastClient == client) [self _notifyInputServerIfNeeded];
}
@end

/* 5400e8e0--5400ec10: shared native field editor. */
@implementation AwtTextView
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame
{ self = [super initWithFrame:frame]; _peer = peer; return self; }
- (jobject)peer { return _peer; }
- (void)setPeer:(jobject)peer
{
    _peer = peer;
    if (!peer) [[NSInputManager currentInputManager] awtForgetClient:self];
}
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type == 100 || type == 101) return _peer != NULL;
    if (!_peer) return NO;
    if (type == 6 || type == 7) return _wantsMouseMotionEvents && [self isAwtEnabled];
    if ((type >= 1 && type <= 4) || (type >= 10 && type <= 12)) return [self isAwtEnabled];
    return NO;
}
- (BOOL)becomeFirstResponder
{ BOOL result = [super becomeFirstResponder]; if (result) [self dispatchFocusEventGained:YES temporary:NO]; return result; }
- (BOOL)resignFirstResponder
{ BOOL result = [super resignFirstResponder]; if (result) [self dispatchFocusEventGained:NO temporary:NO]; return result; }
@end

/* 5400e7e4/5400e860: attach the owning Java peer before AppKit edits. */
@implementation AwtTextFieldCell
- (void)editWithFrame:(NSRect)frame inView:(NSView *)view editor:(NSText *)editor delegate:(id)delegate event:(NSEvent *)event
{
    [(AwtTextView *)editor setPeer:[(AwtTextField *)view peer]];
    [super editWithFrame:frame inView:view editor:editor delegate:delegate event:event];
}
- (void)selectWithFrame:(NSRect)frame inView:(NSView *)view editor:(NSText *)editor delegate:(id)delegate start:(int)start length:(int)length
{
    [(AwtTextView *)editor setPeer:[(AwtTextField *)view peer]];
    [super selectWithFrame:frame inView:view editor:editor delegate:delegate start:start length:length];
}
@end

@implementation AwtTextField
+ (Class)cellClass { return [AwtTextFieldCell class]; }
+ (id)sharedFieldEditorForWindow:(NSWindow *)window
{
    NSMutableDictionary *dictionary = [[NSThread currentThread] threadDictionary];
    NSString *key = [NSString stringWithFormat:@"AwtFieldEditorForWindow%d", (int)window];
    id editor = [dictionary objectForKey:key];
    if (!editor) {
        editor = [[AwtTextView alloc] init];
        [editor setFieldEditor:YES];
        [dictionary setObject:editor forKey:key];
        [editor release];
    }
    return editor;
}
- (id)getFieldEditor { return [AwtTextField sharedFieldEditorForWindow:[self window]]; }
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text editable:(BOOL)editable echoChar:(unsigned short)echo
{
    self = [self commonInitWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor text:text];
    [self setAlignment:NSLeftTextAlignment];
    [[self cell] setWraps:NO]; [[self cell] setScrollable:YES];
    [self setEchoChar:echo];
    [self setEditable:editable]; [self setSelectable:editable];
    [self setBordered:YES]; [self setBezeled:YES];
    [self setTarget:self]; [self setAction:@selector(returnHit:)]; [self setDelegate:self];
    _isLabel = NO;
    if (![self awtComponentGetsBackgroundFromItsContainer] || [self awtComponentUsesCustomColor])
        [self awtSetBackgroundColor:background];
    [self awtSetForegroundColor:foreground];
    return self;
}
- (void)awtSetEditable:(BOOL)editable
{ if (!editable) [self resignFirstResponderIfNecessary]; [self setEditable:editable]; [self setSelectable:editable]; }
/* 5400e208 is an empty method in the DR2 donor. */
- (void)setEchoChar:(unsigned short)echo { }
- (void)selectRange:(NSRange)range { [[self currentEditor] setSelectedRange:range]; }
- (NSRange)selectedRange
{ id editor = [self currentEditor]; return editor ? [editor selectedRange] : NSMakeRange(0, 0); }
- (NSSize)preferredSizeForNumColumns:(int)columns
{ NSSize size = [self preferredSize]; size.width = [[self font] advancementForGlyph:48].width * columns + 6; return size; }
- (void)makeComponentTakeFocus { [self selectText:nil]; }
- (void)_validateEditing:(NSText *)editor
{
    id cell = [self selectedCell];
    if ([cell allowsEditingTextAttributes] && [editor isKindOfClass:[NSTextView class]])
        [cell setAttributedStringValue:[(NSTextView *)editor textStorage]];
    else {
        id control = [cell controlView];
        [cell _setControlView:nil];
        [cell setStringValue:[editor string]];
        [cell _setControlView:control];
    }
}
- (void)validateEditing { id editor = [self currentEditor]; if (editor) [self _validateEditing:editor]; }
- (void)returnHit:(id)sender
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jmethodID method;
    jobject event;
    if (!_peer) return;
    [[self retain] autorelease];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RTextFieldPeer");
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
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text
{
    self = [super initWithFrame:frame];
    _peer = (jobject)peer;
    [self awtSetEnabled:enabled];
    [self awtSetFont:font];
    [self awtSetCursor:cursor];
    [self setStringValue:text];
    return self;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(NSFont *)font backgroundColor:(NSColor *)background foregroundColor:(NSColor *)foreground cursor:(NSCursor *)cursor text:(NSString *)text alignment:(int)alignment
{
    self = [self commonInitWithPeer:peer frame:frame enabled:enabled font:font backgroundColor:background foregroundColor:foreground cursor:cursor text:text];
    [self setAlignment:alignment];
    [self setEditable:NO];
    [self setSelectable:NO];
    [self setBordered:NO];
    [self setBezeled:NO];
    _isLabel = YES;
    [self awtSetForegroundColor:foreground];
    [self awtSetBackgroundColor:background];
    return self;
}
- (void)dealloc { [_cursor release]; [super dealloc]; }
- (jobject)peer { return _peer; }
- (void)clearPeer
{
    AwtContainer *container = [self outermostJavaContainer];
    if ([container lastJavaViewEntered] == self) [container setLastJavaViewEntered:nil];
    if (!_isLabel) {
        AwtTextView *editor = (AwtTextView *)[self currentEditor];
        [editor setPeer:NULL];
        [[self window] endEditingFor:self];
    }
    _peer = NULL;
}
- (void)setStringValue:(NSString *)text { [super setStringValue:text ? text : @""]; }
- (void)awtSetAlignment:(int)alignment { [self setAlignment:alignment]; }
- (void)awtSetBackgroundColor:(NSColor *)color
{
    if (color) [self setBackgroundColor:color];
    [self setDrawsBackground:color != nil];
}
- (void)awtSetForegroundColor:(NSColor *)color { if (color) [self setTextColor:color]; }
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
    if (!enabled) [self resignFirstResponderIfNecessary];
    [self setEnabled:enabled];
    if (_cursor) [[self window] invalidateCursorRectsForView:self];
}
- (BOOL)isAwtEnabled { return _awtEnabled && [[self superview] isAwtEnabled]; }
- (BOOL)isFlipped { return YES; }
- (void)setWantsMouseMotionEvents:(BOOL)enabled { _wantsMouseMotionEvents = enabled; }
- (BOOL)dispatchesAwtEventsOfType:(int)type
{
    if (type < 1 || type > 12) return NO;
    if (type >= 5 && type <= 7 && !_wantsMouseMotionEvents) return NO;
    return [self isAwtEnabled];
}
- (void)mouseEntered:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)mouseExited:(NSEvent *)event { [self dispatchEnterExitPair:event]; }
- (void)awtContainerChangedBackground
{
    if ([self awtComponentGetsBackgroundFromItsContainer])
        [self awtSetBackgroundColor:[(AwtView *)[self superview] awtBackgroundColor]];
}
@end

/* 5401731c/54017574/54017638: unchanged RLabelPeer bindings. */
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RLabelPeer_createNativeLabel(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring text, jint alignment)
{
    AwtTextField *volatile view = nil;
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
        view = [AwtTextField alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtTextField"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor text:nativeText alignment:javaToNSTextAlignment(alignment)];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RLabelPeer_setText(JNIEnv *env, jobject peer, jstring text)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setStringValue:javaToNSString(env, text)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RLabelPeer_setAlignment(JNIEnv *env, jobject peer, jint alignment)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetAlignment:javaToNSTextAlignment(alignment)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}

/* 5401b1e0--5401b9e8: unchanged RTextFieldPeer bindings. */
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_createNativeTextField(JNIEnv *env, jobject peer,
    jint x, jint y, jint width, jint height, jboolean enabled, jboolean visible,
    jobject font, jobject background, jobject foreground, jobject cursor, jstring text, jboolean editable, jchar echo)
{
    AwtTextField *volatile view = nil;
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
        view = [AwtTextField alloc];
        if (!view) [AwtException raise:env exceptionName:"java/lang/OutOfMemoryError" reason:"Cannot allocate AwtTextField"];
        [[view appkitProxy] initWithPeer:(int)globalPeer frame:NSMakeRect(x, y, width, height) enabled:enabled
            font:[nativeFont appkitProxy] backgroundColor:[nativeBackground appkitProxy]
            foregroundColor:[nativeForeground appkitProxy] cursor:nativeCursor text:nativeText editable:editable echoChar:echo];
    NS_HANDLER
        [[AwtView appkitProxy] discardView:(int)view];
        if (globalPeer) (*env)->DeleteGlobalRef(env, globalPeer);
        view = nil;
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return (jint)view;
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_setEchoChar(JNIEnv *env, jobject peer, jchar echo)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setEchoChar:echo];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_setEditable(JNIEnv *env, jobject peer, jboolean editable)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) awtSetEditable:editable];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_nSetText(JNIEnv *env, jobject peer, jstring text)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) setStringValue:javaToNSString(env, text)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_nSelect(JNIEnv *env, jobject peer, jint start, jint end)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [viewFromPeer(env, peer) selectRange:NSMakeRange(start, end - start)];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT jstring JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_getText(JNIEnv *env, jobject peer)
{
    jstring result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        result = nsToJavaString(env, [viewFromPeer(env, peer) stringValue]);
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_getSelectionStart(JNIEnv *env, jobject peer)
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
JNIEXPORT jint JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_getSelectionEnd(JNIEnv *env, jobject peer)
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
JNIEXPORT jobject JNICALL Java_com_apple_rhapsody_awt_RTextFieldPeer_getPreferredSize(JNIEnv *env, jobject peer, jint columns)
{
    jobject result = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        NSSize size = [viewFromPeer(env, peer) preferredSizeForNumColumns:columns];
        jclass cls = (*env)->FindClass(env, "java/awt/Dimension");
        jmethodID ctor;
        if (!cls) [AwtException raiseUsingEnv:env];
        ctor = (*env)->GetMethodID(env, cls, "<init>", "(II)V");
        if (!ctor) [AwtException raiseUsingEnv:env];
        /* 5401b454 requests upward rounding from nsSizeToJavaDimension. */
        result = (*env)->NewObject(env, cls, ctor, (int)ceil(size.width), (int)ceil(size.height));
        (*env)->DeleteLocalRef(env, cls);
        if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
