#ifndef RHAPSODY_AWT_DISPATCHER_H
#define RHAPSODY_AWT_DISPATCHER_H

#import <Foundation/Foundation.h>

extern NSString *AwtProcessRequestsMode;
@interface AwtDispatcher : NSObject {
    NSMutableArray *_handlers;
}
+ (id)defaultDispatcher;
+ (void)setDefaultDispatcher:(id)dispatcher;
- (id)initForConnection:(NSConnection *)connection;
- (void)addPort:(NSPort *)port;
@end

#endif
