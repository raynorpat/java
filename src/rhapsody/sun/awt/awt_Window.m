/* DR2 libawt.A.dylib: AwtWindow construction at 5401008c--5401037c. */
#import "awt_Window.h"
#import "awt_View.h"
#import "awt_Application.h"
#import "awt_Event.h"
#import "awt_Exception.h"
#import "awt_TextField.h"

@interface NSWindow (AWTBorderAccess)
- (id)_borderView;
@end
@interface NSView (AWTBorderResize)
- (void)setIsResizable:(BOOL)resizable;
@end

/* 54010eb8: DR2 changes the border view's resize control, not the style mask. */
@implementation NSWindow (AWTResize)
/* 54010bf4: donor uses a fixed title when computing the native minimum. */
- (NSSize)minimumSize
{
    unsigned int style = [self styleMask];
    NSRect frame = [NSWindow frameRectForContentRect:NSMakeRect(0, 0, 0, 0) styleMask:style];
    int width = (int)[NSWindow minFrameWidthWithTitle:@"Hi!" styleMask:style];
    int height = (int)frame.size.height;
    return NSMakeSize(width ? width : 10, height ? height : 10);
}
- (void)setResizable:(BOOL)resizable
{
    id border = [self _borderView];
    [border setIsResizable:resizable];
    [border setNeedsDisplay:YES];
}
@end

@implementation AwtWindow
/* 54010f08: Java controls supply their shared AWT field editor. */
- (NSText *)fieldEditor:(BOOL)create forObject:(id)object
{
    if ([object respondsToSelector:@selector(getFieldEditor)]) return [object getFieldEditor];
    return [super fieldEditor:create forObject:object];
}
- (id)commonInitWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled styleMask:(int)style contentView:(NSView *)view
{
    NSRect content = [NSWindow contentRectForFrameRect:frame styleMask:style];
    self = [super initWithContentRect:content styleMask:style backing:NSBackingStoreBuffered defer:YES];
    _peer = (jobject)peer;
    [self setContentView:view];
    [view setBoundsOrigin:NSMakePoint(content.origin.x - frame.origin.x, NSMaxY(frame) - NSMaxY(content))];
    [self setDelegate:self];
    /* The donor assigns directly, avoiding focus callbacks during peer creation. */
    _firstResponder = [self contentView];
    [self setAcceptsMouseMovedEvents:YES];
    _isRealWindow = YES;
    return self;
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view
{
    return [self commonInitWithPeer:peer frame:frame enabled:enabled styleMask:0 contentView:view];
}
- (id)initWithPeer:(int)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view resizable:(BOOL)resizable title:(NSString *)title
{
    self = [self commonInitWithPeer:peer frame:frame enabled:enabled
        styleMask:resizable ? 15 : 7 contentView:view];
    [self setTitle:title];
    return self;
}
- (void)dealloc
{
    [[[NSThread currentThread] threadDictionary] removeObjectForKey:
        [NSString stringWithFormat:@"AwtFieldEditorForWindow%d", (int)self]];
    [_menuBar release];
    [super dealloc];
}
- (jobject)peer { return _peer; }
/* Focus portion of 540104f4/54010598; menu activation is handled separately. */
- (void)becomeKeyWindow
{
    id responder = [self firstResponder];
    [super becomeKeyWindow];
    if ([responder dispatchesAwtEventsOfType:101])
        [responder dispatchFocusEventGained:YES temporary:YES];
}
- (void)resignKeyWindow
{
    id responder = [self firstResponder];
    [super resignKeyWindow];
    if ([responder dispatchesAwtEventsOfType:100])
        [responder dispatchFocusEventGained:NO temporary:YES];
}
/* 54010720: programmatic resizing must not feed back through the delegate. */
- (void)awtSetFrame:(NSRect)frame display:(BOOL)display
{
    [self setDelegate:nil];
    [super setFrame:frame display:display];
    [self setDelegate:self];
}
/* 540107dc: enable Component events before reflecting window-manager geometry. */
- (void)notifyJavaFrameChanged
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jfieldID field;
    jmethodID method;
    jobject source;
    jlong mask;
    NSRect frame = [self frame];
    NSRect screen = [[self screen] visibleFrame];
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) [AwtException raiseUsingEnv:env];
    source = (*env)->GetObjectField(env, _peer, field);
    (*env)->DeleteLocalRef(env, cls);
    cls = (*env)->FindClass(env, "java/awt/Component");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "eventMask", "J");
    if (!field) [AwtException raiseUsingEnv:env];
    mask = (*env)->GetLongField(env, source, field);
    (*env)->SetLongField(env, source, field, mask | 1);
    method = (*env)->GetMethodID(env, cls, "setBounds", "(IIII)V");
    if (!method) [AwtException raiseUsingEnv:env];
    (*env)->CallVoidMethod(env, source, method, (int)frame.origin.x,
        (int)(screen.size.height - NSMaxY(frame)), (int)frame.size.width, (int)frame.size.height);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, source);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
}
- (void)windowDidMove:(NSNotification *)notification { [self notifyJavaFrameChanged]; }
- (void)windowDidResize:(NSNotification *)notification { [self notifyJavaFrameChanged]; }
- (void)postWindowEvent:(int)type
{
    [NSApp postEvent:makeWrappedWindowEvent([NSApp appkitEnv], self, type) atStart:YES];
}
- (void)windowDidMiniaturize:(NSNotification *)notification { [self postWindowEvent:203]; }
- (void)windowDidDeminiaturize:(NSNotification *)notification { [self postWindowEvent:204]; }
- (void)windowDidBecomeKey:(NSNotification *)notification { [self postWindowEvent:205]; }
- (void)windowDidResignKey:(NSNotification *)notification { [self postWindowEvent:206]; }
- (BOOL)windowShouldClose:(id)sender { [self postWindowEvent:201]; return NO; }
+ (void)discardWindow:(AwtWindow *)window view:(int)view
{
    [window setDelegate:nil];
    [(AwtView *)view clearPeer];
    if ([window contentView]) {
        [window setReleasedWhenClosed:NO];
        [window orderOut:nil];
        [window close];
    }
    if (window) window->_peer = NULL;
    /* This runs on AppKit, including rollback of a partial construction. */
    [(NSView *)view release];
    [window release];
}
- (void)disposeAwtWindow
{
    [AwtWindow discardWindow:self view:(int)[self contentView]];
}
@end
