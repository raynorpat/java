#ifndef RHAPSODY_AWT_THREAD_CONTEXT_H
#define RHAPSODY_AWT_THREAD_CONTEXT_H

#import <AppKit/NSDPSContext.h>
#import "awt_AutoreleasePool.h"

@interface AwtThreadContext : NSObject {
@public
    NSDPSContext *graphicsContext;
    AwtAutoreleasePool *magicPool;
}
+ (id)currentThreadContext;
@end

AwtAutoreleasePool *NativeMethodEnter(void *env);

#endif
