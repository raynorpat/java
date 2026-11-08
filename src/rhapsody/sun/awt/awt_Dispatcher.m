/* DR2 libawt.A.dylib: dispatcher setup 54004a14--54004b38. */
#import "awt_Dispatcher.h"

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
@end
