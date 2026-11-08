/* DR2 libawt.A.dylib: AwtWindow construction at 5401008c--5401037c. */
#import "awt_Window.h"
#import "awt_Application.h"
#import "awt_Event.h"
#import "awt_Exception.h"

@implementation AwtWindow
- (id)commonInitWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled styleMask:(unsigned int)style contentView:(NSView *)view
{
    NSRect content = [NSWindow contentRectForFrameRect:frame styleMask:style];
    self = [super initWithContentRect:content styleMask:style backing:NSBackingStoreBuffered defer:YES];
    _peer = peer;
    [self setContentView:view];
    [view setBoundsOrigin:NSMakePoint(content.origin.x - frame.origin.x, NSMaxY(frame) - NSMaxY(content))];
    [self setDelegate:self];
    /* The donor assigns directly, avoiding focus callbacks during peer creation. */
    _firstResponder = [self contentView];
    [self setAcceptsMouseMovedEvents:YES];
    _isRealWindow = YES;
    return self;
}
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view
{
    return [self commonInitWithPeer:peer frame:frame enabled:enabled styleMask:0 contentView:view];
}
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled contentView:(NSView *)view resizable:(BOOL)resizable title:(NSString *)title
{
    self = [self commonInitWithPeer:peer frame:frame enabled:enabled
        styleMask:resizable ? 15 : 7 contentView:view];
    [self setTitle:title];
    return self;
}
- (void)dealloc
{
    [_menuBar release];
    [super dealloc];
}
- (jobject)peer { return _peer; }
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
@end
