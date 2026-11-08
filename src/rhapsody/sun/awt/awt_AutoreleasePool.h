#ifndef RHAPSODY_AWT_AUTORELEASE_POOL_H
#define RHAPSODY_AWT_AUTORELEASE_POOL_H

#import <Foundation/NSAutoreleasePool.h>

@interface AwtAutoreleasePool : NSAutoreleasePool {
@public
    NSAutoreleasePool *realPool;
    BOOL inNativeMethod;
    unsigned int deferredReleaseCount;
}
@end

void NativeMethodExit(void *env, AwtAutoreleasePool *pool);

#endif
