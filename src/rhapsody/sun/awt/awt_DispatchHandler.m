/* DR2 libawt.A.dylib: 54004504--54004a14. */
#import "awt_DispatchHandler.h"
#import "awt_Dispatcher.h"
#import "awt_Application.h"
#import "awt_Exception.h"

@implementation AwtDispatchHandler
- (id)init
{
    self = [super init];
    _eventLock = [[NSConditionLock alloc] initWithCondition:1];
    _lockedForDispatch = NO;
    _running = NO;
    _env = NULL;
    _selector = NULL;
    _javaEvent = NULL;
    _consumed = NO;
    _peer = NULL;
    _paintRect = NSZeroRect;
    _selfProxy = nil;
    [NSThread detachNewThreadSelector:@selector(_run) toTarget:self withObject:nil];
    return self;
}
- (void)dealloc
{
    if (_running) {
        [self lockForDispatch:YES];
        _selector = @selector(_terminateDispatching);
        [self _dispatchAndUnlock];
    }
    [_eventLock release];
    [super dealloc];
}
- (void)_run
{
    JavaVM *vm = NULL;
    jsize count;
    id pool;
    JNI_GetCreatedJavaVMs(&vm, 1, &count);
    if (!vm) return;
    pool = [[NSAutoreleasePool alloc] init];
    (*vm)->AttachCurrentThread(vm, &_env, NULL);
    _selfProxy = [[self appkitProxy] retain];
    _running = YES;
    [pool release];
    while (_running) {
        [_eventLock lockWhenCondition:2];
        pool = [[NSAutoreleasePool alloc] init];
        [self performSelector:_selector];
        /* DR2 batches released remote references into the next DO message.
         * Drain callback proxies before the tickle, while AppKit still services
         * requests, so an idle worker cannot retain a disposed native control. */
        [pool release];
        pool = [[NSAutoreleasePool alloc] init];
        [_eventLock unlockWithCondition:1];
        [_selfProxy _tickleAppKitRunLoop];
        [pool release];
    }
    pool = [[NSAutoreleasePool alloc] init];
    [_selfProxy release];
    [pool release];
    (*vm)->DetachCurrentThread(vm);
}
- (BOOL)lockForDispatch:(BOOL)wait
{
    if (wait) {
        [_eventLock lockWhenCondition:1];
        _lockedForDispatch = YES;
    } else {
        _lockedForDispatch = [_eventLock tryLockWhenCondition:1];
    }
    return _lockedForDispatch;
}
- (BOOL)_dispatchAndUnlock
{
    BOOL consumed;
    [_eventLock unlockWithCondition:2];
    while (![_eventLock tryLockWhenCondition:1]) [self _acceptDORequests];
    consumed = _consumed;
    _selector = NULL;
    _javaEvent = NULL;
    _consumed = NO;
    _peer = NULL;
    _paintRect = NSZeroRect;
    _lockedForDispatch = NO;
    [_eventLock unlockWithCondition:1];
    return consumed;
}
- (void)_acceptDORequests
{
    [[NSRunLoop currentRunLoop] acceptInputForMode:AwtProcessRequestsMode beforeDate:[NSDate distantFuture]];
}
- (void)_tickleAppKitRunLoop { }
- (BOOL)dispatchJavaEvent:(jobject)event
{
    _selector = @selector(_realDispatchJavaEvent);
    _javaEvent = event;
    return [self _dispatchAndUnlock];
}
- (void)_realDispatchJavaEvent
{
    jclass cls = (*_env)->FindClass(_env, "com/apple/rhapsody/awt/RToolkit");
    jmethodID method;
    if (!cls) [AwtException raiseUsingEnv:_env];
    method = (*_env)->GetStaticMethodID(_env, cls, "dispatchEvent", "(Ljava/awt/AWTEvent;)Z");
    if (!method) [AwtException raiseUsingEnv:_env];
    _consumed = (*_env)->CallStaticBooleanMethod(_env, cls, method, _javaEvent);
    (*_env)->DeleteLocalRef(_env, cls);
    if ((*_env)->ExceptionOccurred(_env)) [AwtException raiseUsingEnv:_env];
}
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer
{
    _selector = @selector(_realDispatchActionEventToMenuItemPeer);
    _peer = peer;
    [self _dispatchAndUnlock];
}
- (void)_realDispatchActionEventToMenuItemPeer
{
    jclass cls = (*_env)->FindClass(_env, "com/apple/rhapsody/awt/RMenuItemPeer");
    jmethodID method;
    if (!cls) [AwtException raiseUsingEnv:_env];
    method = (*_env)->GetMethodID(_env, cls, "dispatchActionEvent", "()V");
    if (!method) [AwtException raiseUsingEnv:_env];
    (*_env)->CallVoidMethod(_env, _peer, method);
    (*_env)->DeleteLocalRef(_env, cls);
    if ((*_env)->ExceptionOccurred(_env)) [AwtException raiseUsingEnv:_env];
}
- (void)_terminateDispatching { _running = NO; }
@end
