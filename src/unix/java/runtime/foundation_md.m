/* DR2 sysThreadBootstrap 5c144a38 starts Foundation's multithreaded mode. */
#import <Foundation/Foundation.h>

void rhapsodyFoundationInit(void)
{
    NSObject *object = [NSObject new];
    [NSThread detachNewThreadSelector:@selector(self) toTarget:object withObject:nil];
    /* detach retains its target; release the allocation's ownership. */
    [object release];
}

void rhapsodyFoundationExit(void)
{
    [NSThread exit];
}
