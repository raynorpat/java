#import <AppKit/AppKit.h>
#include <stdio.h>
#include <stdlib.h>

@interface NSApplication (HandoffTest)
- (void)setupForAwtToolkitUsingEnv:(void *)env;
- (BOOL)inAppkitThread;
@end
@interface NSObject (HandoffTest)
- (id)appkitProxy;
+ (id)appkitProxy;
+ (id)allocInAppkitThread;
- (id)windowProxy;
@end

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static NSThread *appThread;
static NSConditionLock *done;
static id target;

@interface HandoffTarget : NSObject
- (int)check:(int)value;
- (void)raiseOnAppkit;
- (void)releaseAllocation;
+ (int)checkClass:(int)value;
+ (void)worker:(id)unused;
@end
@implementation HandoffTarget
- (void)releaseAllocation { [self release]; }
+ (id)allocWithZone:(NSZone *)zone
{
    CHECK([NSThread currentThread] == appThread);
    return [super allocWithZone:zone];
}
- (void)raiseOnAppkit
{
    CHECK([NSThread currentThread] == appThread);
    [NSException raise:@"HandoffException" format:@"AppKit failure"];
}
- (int)check:(int)value
{
    CHECK([NSThread currentThread] == appThread);
    CHECK([NSApp inAppkitThread]);
    return value + 1;
}
+ (int)checkClass:(int)value
{
    CHECK([NSThread currentThread] == appThread);
    return value + 2;
}
+ (void)worker:(id)unused
{
    id pool = [[NSAutoreleasePool alloc] init];
    id proxy;
    id allocated;
    int i;
    BOOL caught = NO;
    CHECK(![NSApp inAppkitThread]);
    CHECK([target windowProxy] == target);
    proxy = [target appkitProxy];
    for (i = 0; i < 20; i++) {
        CHECK([proxy check:i] == i + 1);
        CHECK([[HandoffTarget appkitProxy] checkClass:i] == i + 2);
    }
    allocated = [[HandoffTarget allocInAppkitThread] init];
    CHECK([allocated check:40] == 41);
    /* The returned NSDistantObject is autoreleased locally. Balance the
     * remote allocation on its owning thread without releasing that proxy. */
    [allocated releaseAllocation];
    NS_DURING
        [proxy raiseOnAppkit];
    NS_HANDLER
        caught = [[localException name] isEqualToString:@"HandoffException"];
    NS_ENDHANDLER
    CHECK(caught);
    [pool release];
    [done lock];
    [done unlockWithCondition:1];
}
@end

int main(void)
{
    id pool = [[NSAutoreleasePool alloc] init];
    NSDate *deadline;
    NSString *modes[4];
    int mode;
    [NSApplication sharedApplication];
    appThread = [NSThread currentThread];
    [NSApp setupForAwtToolkitUsingEnv:NULL];
    target = [HandoffTarget new];
    CHECK([NSApp inAppkitThread]);
    CHECK([target appkitProxy] == target);
    CHECK([HandoffTarget appkitProxy] == [HandoffTarget class]);
    modes[0] = NSDefaultRunLoopMode;
    modes[1] = NSModalPanelRunLoopMode;
    modes[2] = NSEventTrackingRunLoopMode;
    modes[3] = @"AwtProcessRequestsMode";
    for (mode = 0; mode < 4; mode++) {
        done = [[NSConditionLock alloc] initWithCondition:0];
        [NSThread detachNewThreadSelector:@selector(worker:) toTarget:[HandoffTarget class] withObject:nil];
        deadline = [NSDate dateWithTimeIntervalSinceNow:20];
        while (![done tryLockWhenCondition:1]) {
            CHECK([deadline timeIntervalSinceNow] > 0);
            [[NSRunLoop currentRunLoop] runMode:modes[mode] beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
        }
        [done unlock];
        [done release];
    }
    [target release];
    [pool release];
    puts("DR2 AppKit instance and class proxies dispatch on the owning thread");
    return 0;
}
