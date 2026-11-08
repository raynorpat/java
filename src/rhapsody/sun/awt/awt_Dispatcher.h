#ifndef RHAPSODY_AWT_DISPATCHER_H
#define RHAPSODY_AWT_DISPATCHER_H

#import <Foundation/Foundation.h>
#include "jni.h"

extern NSString *AwtProcessRequestsMode;
@interface AwtDispatcher : NSObject {
    NSMutableArray *_handlers;
}
+ (id)defaultDispatcher;
+ (void)setDefaultDispatcher:(id)dispatcher;
- (id)initForConnection:(NSConnection *)connection;
- (void)addPort:(NSPort *)port;
- (id)_acquireDispatchHandler;
- (BOOL)dispatchJavaEvent:(jobject)event;
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer;
@end

#endif
