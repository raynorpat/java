/* DR2 libawt.A.dylib: dispatcher setup 54004a14--54004b38. */
#import "awt_Dispatcher.h"
#import "awt_DispatchHandler.h"

NSString *AwtProcessRequestsMode = @"AwtProcessRequestsMode";
static AwtDispatcher *defaultDispatcher;

@implementation AwtDispatcher
+ (id)defaultDispatcher { return defaultDispatcher; }
+ (void)setDefaultDispatcher:(id)dispatcher { defaultDispatcher = dispatcher; }
- (id)initForConnection:(NSConnection *)connection
{
    self = [super init];
    _handlers = [[NSMutableArray alloc] initWithCapacity:0];
    [connection addRequestMode:AwtProcessRequestsMode];
    return self;
}
- (void)dealloc
{
    [_handlers release];
    _handlers = nil;
    [super dealloc];
}
- (void)addPort:(NSPort *)port
{
    [[NSRunLoop currentRunLoop] addPort:port forMode:AwtProcessRequestsMode];
}
- (id)_acquireDispatchHandler
{
    unsigned int index;
    AwtDispatchHandler *handler;
    for (index = 0; index < [_handlers count]; index++) {
        handler = [_handlers objectAtIndex:index];
        if ([handler lockForDispatch:NO]) return handler;
    }
    handler = [[AwtDispatchHandler alloc] init];
    [handler lockForDispatch:YES];
    [_handlers addObject:handler];
    [handler release];
    return handler;
}
- (BOOL)dispatchJavaEvent:(jobject)event
{
    return [[self _acquireDispatchHandler] dispatchJavaEvent:event];
}
- (void)dispatchActionEventToMenuItemPeer:(jobject)peer
{
    [[self _acquireDispatchHandler] dispatchActionEventToMenuItemPeer:peer];
}
@end
