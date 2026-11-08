/* DR2 libawt.A.dylib: AwtCanvas 54003d6c--54003f64. */
#import "awt_Canvas.h"
#import "awt_Application.h"
#import "awt_Event.h"
#import "awt_Exception.h"

/* 5400ff44: inspect the Java field, not getBackground's inherited value. */
@implementation NSView (AWTBackground)
- (BOOL)awtComponentGetsBackgroundFromItsContainer
{
    JNIEnv *env = [NSApp appkitEnv];
    jclass cls;
    jfieldID field;
    jobject source, color;
    BOOL inherited;
    cls = (*env)->FindClass(env, "com/apple/rhapsody/awt/RComponentPeer");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) [AwtException raiseUsingEnv:env];
    source = (*env)->GetObjectField(env, [(AwtView *)self peer], field);
    (*env)->DeleteLocalRef(env, cls);
    cls = (*env)->FindClass(env, "java/awt/Component");
    if (!cls) [AwtException raiseUsingEnv:env];
    field = (*env)->GetFieldID(env, cls, "background", "Ljava/awt/Color;");
    if (!field) [AwtException raiseUsingEnv:env];
    color = (*env)->GetObjectField(env, source, field);
    inherited = color == NULL;
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, source);
    (*env)->DeleteLocalRef(env, color);
    if ((*env)->ExceptionOccurred(env)) [AwtException raiseUsingEnv:env];
    return inherited;
}
@end

@implementation AwtCanvas
- (void)drawRect:(NSRect)rect
{
    /* AppKit may drain an invalidated rectangle while disposal services callbacks. */
    if (![self peer]) return;
    [[self awtBackgroundColor] set];
    NSRectFill(rect);
    PSFlush();
    [NSApp postEvent:makeWrappedPaintEvent([NSApp appkitEnv], self, 800, rect) atStart:YES];
}
- (NSRect)awtBounds { return [self bounds]; }
- (NSRect)awtVisibleBounds { return [self visibleRect]; }
- (BOOL)awtCanDraw { return [[self window] windowNumber] > 0; }
- (BOOL)awtLockFocus { [self lockFocus]; return YES; }
- (void)awtUnlockFocus { [self unlockFocus]; }
- (NSColor *)awtColor { return [self awtBackgroundColor]; }
- (void)awtContainerChangedBackground
{
    if ([self awtComponentGetsBackgroundFromItsContainer])
        [self awtSetBackgroundColor:[(AwtView *)[self superview] awtBackgroundColor]];
}
@end
