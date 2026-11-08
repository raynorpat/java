/* DR2 libawt.A.dylib: 54002828, 54002858, 54002f6c--54003080. */
#import "awt_Application.h"
#import "awt_Dispatcher.h"
#import "awt_Exception.h"
#import "awt_Event.h"
#import "awt_View.h"
#import <objc/objc-class.h>
#import <objc/objc-runtime.h>

@interface NSWindow (AWTInputTracking)
- (NSView *)_lastLeftHit;
- (NSView *)_lastRightHit;
@end

static JNIEnv *appkitEnv;
static NSThread *appkitThread;
static NSConnection *kitConnection;
static NSString *portName;
/* The blocking RToolkit.run JNI frame keeps this local reference alive. */
static jobject awtToolkit;
static unsigned int previousEventFlags;

@implementation AwtApplication
+ (id)sharedApplicationUsingEnv:(JNIEnv *)env awtToolkit:(jobject)toolkit
{
    id application = [self sharedApplication];
    appkitEnv = env;
    awtToolkit = toolkit;
    return application;
}
- (void)finishLaunching
{
    jclass cls;
    jmethodID notify;
    [super finishLaunching];
    [self commonAwtInitialization];
    cls = (*appkitEnv)->FindClass(appkitEnv, "com/apple/rhapsody/awt/RToolkit");
    if (!cls) [AwtException raiseUsingEnv:appkitEnv];
    notify = (*appkitEnv)->GetMethodID(appkitEnv, cls, "notifyRunning", "()V");
    if (!notify) [AwtException raiseUsingEnv:appkitEnv];
    (*appkitEnv)->CallVoidMethod(appkitEnv, awtToolkit, notify);
    (*appkitEnv)->DeleteLocalRef(appkitEnv, cls);
    if ((*appkitEnv)->ExceptionOccurred(appkitEnv)) [AwtException raiseUsingEnv:appkitEnv];
}
@end

/* 54003094: categories precede the original class's method list in DR2. */
static IMP originalApplicationMethod(SEL selector)
{
    void *iterator = NULL;
    struct objc_method_list *methods;
    IMP result = NULL;
    Class cls = objc_lookUpClass("NSApplication");
    while ((methods = class_nextMethodList(cls, &iterator))) {
        int index;
        for (index = 0; index < methods->method_count; index++)
            if (methods->method_list[index].method_name == selector)
                result = methods->method_list[index].method_imp;
    }
    return result;
}

@implementation NSApplication (AWTAdditions)
- (unsigned int)previousEventFlags { return previousEventFlags; }
/* 54003110: peeking must not dispatch; dequeue consumes Java covers. */
- (NSEvent *)nextEventMatchingMask:(unsigned int)mask untilDate:(NSDate *)expiration inMode:(NSString *)mode dequeue:(BOOL)dequeue
{
    typedef NSEvent *(*NextEvent)(id, SEL, unsigned int, NSDate *, NSString *, BOOL);
    static NextEvent original;
    NSEvent *event;
    if (!original) original = (NextEvent)originalApplicationMethod(_cmd);
    if (!dequeue) return original(self, _cmd, mask, expiration, mode, NO);
    previousEventFlags = [[self currentEvent] modifierFlags];
    do {
        event = original(self, _cmd, mask, expiration, mode, YES);
    } while (event && ![self sendNSEventThroughJava:event]);
    return event;
}
/* 54002adc: dispatcher workers require a global reference. */
- (BOOL)dispatchJavaEvent:(jobject)event isLocalRef:(BOOL)local
{
    jobject global = local ? (*appkitEnv)->NewGlobalRef(appkitEnv, event) : event;
    BOOL consumed = NO;
    NS_DURING
        consumed = [[AwtDispatcher defaultDispatcher] dispatchJavaEvent:global];
    NS_HANDLER
        if (local) (*appkitEnv)->DeleteGlobalRef(appkitEnv, global);
        [localException raise];
    NS_ENDHANDLER
    if (local) (*appkitEnv)->DeleteGlobalRef(appkitEnv, global);
    return consumed;
}
/* 54002c58: a queued Java cover never reaches native sendEvent:. */
- (NSEvent *)sendNSEventThroughJava:(NSEvent *)event
{
    if (![event isJavaEvent]) {
        int type = [event type];
        NSView *target;
        jobject source, converted;
        BOOL consumed = NO;
        if (type < NSLeftMouseDown || type > NSFlagsChanged ||
            type == NSMouseEntered || type == NSMouseExited) return event;
        target = [self findAwtTargetForEvent:event];
        if (!target || ![target dispatchesAwtEventsOfType:type] || ![(AwtView *)target peer]) return event;
        source = javaComponentForView(appkitEnv, target);
        converted = type >= NSKeyDown ? nsKeyEventToJavaEvent(appkitEnv, event, source) :
            nsMouseEventToJavaEvent(appkitEnv, event, source, target);
        (*appkitEnv)->DeleteLocalRef(appkitEnv, source);
        if (!converted) return event;
        [target retain];
        NS_DURING
            consumed = [self dispatchJavaEvent:converted isLocalRef:YES];
            /* 54002c58: native tracking also receives consumed key releases. */
            if (type == NSLeftMouseDown || type == NSRightMouseDown || type == NSKeyUp) consumed = NO;
            if (![(AwtView *)target peer] || ![target window]) consumed = YES;
            if (!consumed) {
                [event updateToMatchJavaEvent:converted];
                [target sendAdditionalJavaEventsFor:event];
            }
        NS_HANDLER
            (*appkitEnv)->DeleteLocalRef(appkitEnv, converted);
            [target release];
            [localException raise];
        NS_ENDHANDLER
        (*appkitEnv)->DeleteLocalRef(appkitEnv, converted);
        [target release];
        return consumed ? nil : event;
    }
    NS_DURING
        [self dispatchJavaEvent:[event javaEvent] isLocalRef:NO];
    NS_HANDLER
        [event releaseJavaEvent:appkitEnv];
        [localException raise];
    NS_ENDHANDLER
    [event releaseJavaEvent:appkitEnv];
    return nil;
}

/* 54002998: mouse releases and drags follow AppKit's captured down target. */
- (id)findAwtTargetForEvent:(NSEvent *)event
{
    NSWindow *modal = [self modalWindow];
    if (modal && [event window] != modal) return nil;
    switch ([event type]) {
        case NSLeftMouseDown: case NSRightMouseDown: case NSMouseMoved:
            return [[event window] deepestViewUnderLocation:[event locationInWindow]
                dispatchingEventsOfType:[event type]];
        case NSLeftMouseUp: case NSLeftMouseDragged: return [[event window] _lastLeftHit];
        case NSRightMouseUp: case NSRightMouseDragged: return [[event window] _lastRightHit];
        case NSKeyDown: case NSKeyUp: case NSFlagsChanged: return [[self keyWindow] firstResponder];
        default: return nil;
    }
}
- (id)proxyForClassByName:(NSString *)name
{
    return objc_getClass([name cString]);
}
- (id)proxyForObjectByAddress:(int)address
{
    return (id)address;
}
- (void)buildConnectionForDO
{
    int index;
    NSString *registeredName = nil;
    kitConnection = [[NSConnection alloc] init];
    [kitConnection setRootObject:self];
    for (index = 0; index < 10; index++) {
        NSString *name = [NSString stringWithFormat:@"Super-secret backdoor hack port #%d", index];
        if ([kitConnection registerName:name]) {
            registeredName = [name retain];
            break;
        }
    }
    if (!registeredName)
        [NSException raise:@"DO error" format:@"Can't register AppKit connection (all names in use)."];
    portName = registeredName;
    [kitConnection addRequestMode:NSModalPanelRunLoopMode];
    [kitConnection addRequestMode:NSEventTrackingRunLoopMode];
}
- (void)setupForAwtToolkitUsingEnv:(JNIEnv *)env
{
    appkitEnv = env;
    [self commonAwtInitialization];
}
- (void)commonAwtInitialization
{
    appkitThread = [NSThread currentThread];
    [self buildConnectionForDO];
    [AwtDispatcher setDefaultDispatcher:[[AwtDispatcher alloc] initForConnection:kitConnection]];
}
- (NSString *)portName { return portName; }
- (BOOL)inAppkitThread { return appkitThread == [NSThread currentThread]; }
- (JNIEnv *)appkitEnv { return appkitEnv; }
@end
