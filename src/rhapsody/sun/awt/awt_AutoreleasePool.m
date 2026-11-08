/* DR2 libawt.A.dylib: 54003310, 54003354, 5401d3e0. */
#import <Foundation/Foundation.h>
#import "awt_AutoreleasePool.h"

@implementation AwtAutoreleasePool
- (id)init
{
    realPool = nil;
    inNativeMethod = YES;
    deferredReleaseCount = 0;
    return [super init];
}

- (void)addObject:(id)object
{
    /* This child becomes the current pool, so subsequent autoreleases go
     * directly to it until NativeMethodExit drains the batch. */
    realPool = [[NSAutoreleasePool alloc] init];
    NSAutoreleaseObject(object);
}
@end

void NativeMethodExit(void *env, AwtAutoreleasePool *pool)
{
    pool->inNativeMethod = NO;
    if (pool->realPool && ++pool->deferredReleaseCount > 10) {
        [pool->realPool release];
        pool->realPool = nil;
        pool->deferredReleaseCount = 0;
    }
}
