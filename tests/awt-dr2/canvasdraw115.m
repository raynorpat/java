#import <AppKit/AppKit.h>
#include "jni.h"
#include <stdio.h>
#include <stdlib.h>

@interface NSObject (CanvasDrawTest)
+ (id)appkitProxy;
- (JNIEnv *)appkitEnv;
- (id)initWithPeer:(jobject)peer frame:(NSRect)frame enabled:(BOOL)enabled font:(id)font backgroundColor:(id)background foregroundColor:(id)foreground cursor:(id)cursor;
- (BOOL)awtCanDraw;
- (BOOL)awtLockFocus;
- (void)awtUnlockFocus;
- (NSRect)awtBounds;
- (NSRect)awtVisibleBounds;
- (id)awtColor;
- (id)awtBackgroundColor;
- (void)awtSetBackgroundColor:(id)color;
- (BOOL)awtComponentGetsBackgroundFromItsContainer;
- (void)awtSetEnabled:(BOOL)enabled;
- (id)lastJavaViewEntered;
- (void)setLastJavaViewEntered:(id)view;
- (void)notifyAwtWindowOfNativeWidgetAddition:(id)window;
- (void)notifyAwtWindowOfNativeWidgetRemoval:(id)window;
- (BOOL)isJavaEvent;
- (jobject)javaEvent;
- (void)releaseJavaEvent:(JNIEnv *)env;
@end
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static int added, removed;
@interface ContainerChildProbe : NSView
@end
@implementation ContainerChildProbe
- (void)notifyAwtWindowOfNativeWidgetAddition:(id)window { CHECK(window == (id)123); added++; }
- (void)notifyAwtWindowOfNativeWidgetRemoval:(id)window { CHECK(window == (id)123); removed++; }
@end

@interface CanvasDrawProbe : NSObject
+ (int)drawWithPeer:(int)peer inherited:(BOOL)inherited container:(BOOL)container;
@end
@implementation CanvasDrawProbe
+ (int)drawWithPeer:(int)peer inherited:(BOOL)inherited container:(BOOL)container
{
    JNIEnv *env = [NSApp appkitEnv];
    Class canvasClass = NSClassFromString(container ? @"AwtContainer" : @"AwtCanvas");
    NSRect bounds = NSMakeRect(0, 0, 80, 60);
    NSRect dirty = NSMakeRect(4, 5, 20, 21);
    id canvas, parent, child;
    NSWindow *window;
    NSEvent *cover;
    jobject result;
    NSColor *pixel;
    float red, green, blue, alpha;
    CHECK(canvasClass != Nil);
    parent = [[NSClassFromString(@"AwtView") alloc] initWithPeer:(jobject)peer frame:bounds enabled:YES
        font:nil backgroundColor:[NSColor greenColor] foregroundColor:nil cursor:nil];
    child = [[canvasClass alloc] initWithPeer:(jobject)peer frame:bounds enabled:YES
        font:nil backgroundColor:[NSColor redColor] foregroundColor:nil cursor:nil];
    [parent addSubview:child];
    CHECK([child awtComponentGetsBackgroundFromItsContainer] == inherited);
    [parent awtSetBackgroundColor:[NSColor blueColor]];
    CHECK([child awtBackgroundColor] == (inherited ? [NSColor blueColor] : [NSColor redColor]));
    [child removeFromSuperview];
    [child release];
    [parent release];
    canvas = [[canvasClass alloc] initWithPeer:(jobject)peer frame:bounds enabled:YES
        font:nil backgroundColor:[NSColor redColor] foregroundColor:[NSColor blackColor] cursor:nil];
    CHECK(![canvas awtCanDraw]);
    window = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 80, 60)
        styleMask:0 backing:NSBackingStoreBuffered defer:NO];
    [window setContentView:canvas];
    CHECK([canvas awtCanDraw]);
    CHECK(NSEqualRects([canvas awtBounds], bounds));
    CHECK(NSEqualRects([canvas awtVisibleBounds], [canvas visibleRect]));
    CHECK([canvas awtColor] == [NSColor redColor]);
    CHECK([canvas awtLockFocus]);
    while ((cover = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:YES]))
        if ([cover isJavaEvent]) [cover releaseJavaEvent:env];
    [canvas drawRect:dirty];
    [canvas setNeedsDisplay:NO];
    cover = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:NO];
    CHECK(cover && [cover isJavaEvent]);
    result = (*env)->NewGlobalRef(env, [cover javaEvent]);
    pixel = [NSReadPixel(NSMakePoint(10, 10)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
    CHECK(red > 0.99 && green < 0.01 && blue < 0.01);
    [canvas awtUnlockFocus];
    /* Donor dequeue dispatches covers itself; a plain AppKit queue returns them. */
    while ((cover = [NSApp nextEventMatchingMask:NSApplicationDefinedMask untilDate:[NSDate date]
        inMode:NSDefaultRunLoopMode dequeue:YES]))
        if ([cover isJavaEvent]) [cover releaseJavaEvent:env];
    if (container) {
        NSPoint hit;
        id second;
        child = [[ContainerChildProbe alloc] initWithFrame:NSMakeRect(10, 10, 20, 20)];
        second = [[ContainerChildProbe alloc] initWithFrame:NSMakeRect(20, 20, 20, 20)];
        [canvas addSubview:child];
        [canvas addSubview:second];
        hit = [[canvas superview] convertPoint:NSMakePoint(15, 15) fromView:canvas];
        CHECK([canvas hitTest:hit] == child);
        [canvas awtSetEnabled:NO];
        CHECK([canvas hitTest:hit] == nil);
        [canvas awtSetEnabled:YES];
        [canvas setLastJavaViewEntered:child];
        CHECK([canvas lastJavaViewEntered] == child);
        [canvas setLastJavaViewEntered:nil];
        added = removed = 0;
        [canvas notifyAwtWindowOfNativeWidgetAddition:(id)123];
        [canvas notifyAwtWindowOfNativeWidgetRemoval:(id)123];
        CHECK(added == 2 && removed == 2);
        CHECK([canvas awtLockFocus]);
        [[NSColor blueColor] set];
        NSRectFill(bounds);
        PSFlush();
        pixel = [NSReadPixel(NSMakePoint(15, 15)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red > 0.99 && green < 0.01 && blue < 0.01);
        pixel = [NSReadPixel(NSMakePoint(25, 25)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red > 0.99 && green < 0.01 && blue < 0.01);
        pixel = [NSReadPixel(NSMakePoint(5, 5)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red < 0.01 && green < 0.01 && blue > 0.99);
        [canvas awtUnlockFocus];
        [child removeFromSuperview];
        [child release];
        CHECK([canvas awtLockFocus]);
        [[NSColor greenColor] set];
        NSRectFill(bounds);
        PSFlush();
        pixel = [NSReadPixel(NSMakePoint(15, 15)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red < 0.01 && green > 0.99 && blue < 0.01);
        pixel = [NSReadPixel(NSMakePoint(35, 35)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red > 0.99 && green < 0.01 && blue < 0.01);
        [canvas awtUnlockFocus];
        [second removeFromSuperview];
        [second release];
        CHECK([canvas awtLockFocus]);
        [[NSColor blueColor] set];
        NSRectFill(bounds);
        PSFlush();
        pixel = [NSReadPixel(NSMakePoint(35, 35)) colorUsingColorSpaceName:NSDeviceRGBColorSpace];
        [pixel getRed:&red green:&green blue:&blue alpha:&alpha];
        CHECK(red < 0.01 && green < 0.01 && blue > 0.99);
        [canvas awtUnlockFocus];
    }
    [window release];
    [canvas release];
    return (int)result;
}
@end

JNIEXPORT jobject JNICALL Java_CanvasDraw115_draw(JNIEnv *env, jclass cls, jobject source, jboolean inherited, jboolean container)
{
    id pool = [[NSAutoreleasePool alloc] init];
    jclass peerClass = (*env)->FindClass(env, "com/apple/rhapsody/awt/RCanvasPeer");
    jobject peer, globalPeer, event, result;
    jfieldID field;
    jobjectArray root;
    if (!peerClass) { [pool release]; return NULL; }
    peer = (*env)->AllocObject(env, peerClass);
    field = (*env)->GetFieldID(env, peerClass, "fJavaCounterpart", "Ljava/awt/Component;");
    if (!field) { [pool release]; return NULL; }
    (*env)->SetObjectField(env, peer, field, source);
    globalPeer = (*env)->NewGlobalRef(env, peer);
    event = (jobject)[[CanvasDrawProbe appkitProxy] drawWithPeer:(int)globalPeer inherited:inherited container:container];
    root = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/Object"), event);
    result = (*env)->GetObjectArrayElement(env, root, 0);
    (*env)->DeleteGlobalRef(env, event);
    (*env)->DeleteGlobalRef(env, globalPeer);
    [pool release];
    return result;
}
