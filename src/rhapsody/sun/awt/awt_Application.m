/* DR2 libawt.A.dylib: 54002828, 54002858, 54002f6c--54003080. */
#import "awt_Application.h"
#import "awt_Dispatcher.h"
#import "awt_Exception.h"
#import "awt_Event.h"
#import <objc/objc-class.h>
#import <objc/objc-runtime.h>

static JNIEnv *appkitEnv;
static NSThread *appkitThread;
static NSConnection *kitConnection;
static NSString *portName;
/* The blocking RToolkit.run JNI frame keeps this local reference alive. */
static jobject awtToolkit;

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
/* 54003110: peeking must not dispatch; dequeue consumes Java covers. */
- (NSEvent *)nextEventMatchingMask:(unsigned int)mask untilDate:(NSDate *)expiration inMode:(NSString *)mode dequeue:(BOOL)dequeue
{
    typedef NSEvent *(*NextEvent)(id, SEL, unsigned int, NSDate *, NSString *, BOOL);
    static NextEvent original;
    NSEvent *event;
    if (!original) original = (NextEvent)originalApplicationMethod(_cmd);
    if (!dequeue) return original(self, _cmd, mask, expiration, mode, NO);
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
    if (![event isJavaEvent]) return event;
    NS_DURING
        [self dispatchJavaEvent:[event javaEvent] isLocalRef:NO];
    NS_HANDLER
        [event releaseJavaEvent:appkitEnv];
        [localException raise];
    NS_ENDHANDLER
    [event releaseJavaEvent:appkitEnv];
    return nil;
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
