/* DR2 libawt.A.dylib: 5400ec20, 5400ed44, 5401d308. */
#import <AppKit/AppKit.h>
#import "awt_ThreadContext.h"

/* These DR2 selectors are implemented by the system frameworks but omitted
 * from their public headers. */
@interface NSAutoreleasePool (AwtDR2)
+ (BOOL)autoreleasePoolExists;
@end
@interface NSDPSServerContext (AwtDR2)
+ (id)contextForSecondaryThread;
@end

extern void *get_awt_cookie(void *env);
extern void set_awt_cookie(void *env, void *cookie);

@implementation AwtThreadContext
- (id)init
{
    if ([NSAutoreleasePool autoreleasePoolExists]) magicPool = nil;
    else magicPool = [[AwtAutoreleasePool alloc] init];
    graphicsContext = [NSDPSContext currentContext];
    if (!graphicsContext) graphicsContext = [NSDPSServerContext contextForSecondaryThread];
    [[[NSThread currentThread] threadDictionary] setObject:self forKey:@"AwtThreadContext Key"];
    /* The thread dictionary owns this context, including for native callers. */
    [[super init] release];
    return self;
}

+ (id)currentThreadContext
{
    return [[[NSThread currentThread] threadDictionary] objectForKey:@"AwtThreadContext Key"];
}
@end

AwtAutoreleasePool *NativeMethodEnter(void *env)
{
    AwtThreadContext *context = env ? get_awt_cookie(env) : nil;
    if (!context) {
        context = [AwtThreadContext currentThreadContext];
        if (env && context) set_awt_cookie(env, context);
    }
    if (!context) {
        context = [[AwtThreadContext alloc] init];
        if (env) set_awt_cookie(env, context);
        return context->magicPool;
    }
    if (context->magicPool && !context->magicPool->inNativeMethod) {
        context->magicPool->inNativeMethod = YES;
        return context->magicPool;
    }
    return nil;
}
