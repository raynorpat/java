#ifndef RHAPSODY_AWT_DISPATCH_HANDLER_H
#define RHAPSODY_AWT_DISPATCH_HANDLER_H

#import <Foundation/Foundation.h>
#include "jni.h"

@interface AwtDispatchHandler : NSObject {
    NSConditionLock *_eventLock;
    BOOL _lockedForDispatch;
    BOOL _running;
    JNIEnv *_env;
    SEL _selector;
    jobject _javaEvent;
    BOOL _consumed;
    jobject _peer;
    NSRect _paintRect;
    AwtDispatchHandler *_selfProxy;
}
- (BOOL)lockForDispatch:(BOOL)wait;
- (BOOL)dispatchJavaEvent:(jobject)event;
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer;
- (void)_run;
- (BOOL)_dispatchAndUnlock;
- (void)_acceptDORequests;
- (void)_tickleAppKitRunLoop;
- (void)_realDispatchJavaEvent;
- (void)_realDispatchActionEventToMenuItemPeer;
- (void)_terminateDispatching;
@end

#endif
