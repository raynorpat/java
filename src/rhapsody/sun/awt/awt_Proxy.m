/* DR2 libawt.A.dylib: 54004c9c--54004dac. */
#import "awt_Application.h"

@implementation NSObject (AWTProxy)
- (id)appkitProxy
{
    id application;
    if ([NSApp inAppkitThread]) return self;
    application = [NSConnection rootProxyForConnectionWithRegisteredName:[NSApp portName] host:nil];
    return [application proxyForObjectByAddress:(int)self];
}
- (id)windowProxy { return self; }
+ (id)appkitProxy
{
    id application;
    if ([NSApp inAppkitThread]) return self;
    application = [NSConnection rootProxyForConnectionWithRegisteredName:[NSApp portName] host:nil];
    return [application proxyForClassByName:NSStringFromClass([self class])];
}
+ (id)allocInAppkitThread { return [[self appkitProxy] alloc]; }
@end

/* 54004de4: classes must cross the connection by reference too. */
@implementation NSObject (DOHacks)
+ (id)replacementObjectForPortCoder:(NSPortCoder *)coder
{
    return [NSDistantObject proxyWithLocal:self connection:[coder connection]];
}
@end
