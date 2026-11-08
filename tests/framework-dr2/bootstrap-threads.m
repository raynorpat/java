#import <Foundation/Foundation.h>
#include <mach/cthreads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DONOR
#include <mach-o/dyld.h>
static void (*safeRelease)(id), (*purge)(void), (*enable)(BOOL), (*exitStub)(int), (*threadSafe)(void);
static int (*enterStub)(void);
static NSRecursiveLock **bridgeLock;
static cthread_t *firstThread;
static int *safeEnabled;
/* DR2 local symbols are not all dyld exports; addresses are donor IDA evidence. */
static void *donorAddress(unsigned address)
{
    unsigned i;
    for (i = 0; i < _dyld_image_count(); ++i)
        if (strstr(_dyld_get_image_name(i), "libObjCJava.A.dylib"))
            return (void *)(address + _dyld_get_image_vmaddr_slide(i));
    abort();
    return NULL;
}
#else
#import "../../src/rhapsody/java/bridge/bootstrap.h"
#define safeRelease _NSJavaSafeRelease
#define purge _NSJavaSafePurge
#define enable _NSJavaEnableSafeDeallocation
#define exitStub _NSJavaStubExit
#define enterStub _NSJavaStubEnter
#define threadSafe GoThreadSafe
#define bridgeLock (&_NSBridgeLock)
#define firstThread (&_NSFirstThread)
#define safeEnabled (&_NSJavaSafeDeallocation)
#endif
static int released;
@interface BootstrapReleaseProbe : NSObject { int number; }
- initWithNumber:(int)value;
@end
@implementation BootstrapReleaseProbe
- initWithNumber:(int)value { [super init]; number = value; return self; }
- (void)dealloc { released++; printf("release %d\n", number); [super dealloc]; }
@end
static void *worker(void *object)
{
    safeRelease((id)object);
    exitStub(enterStub());
    return NULL;
}
static void require(int condition) { if (!condition) exit(2); }
int main(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    cthread_t thread;
#ifdef DONOR
    require(NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib"));
    safeRelease = donorAddress(0x53d06b38);
    purge = donorAddress(0x53d06c04);
    enable = donorAddress(0x53d06b08);
    enterStub = donorAddress(0x53d06c98);
    exitStub = donorAddress(0x53d06ca4);
    threadSafe = donorAddress(0x53d05d38);
    bridgeLock = donorAddress(0x53d10034);
    firstThread = donorAddress(0x53d10038);
    safeEnabled = donorAddress(0x53d10048);
#endif
    *bridgeLock = [NSRecursiveLock new];
    *firstThread = cthread_self();
    threadSafe();
    require(*safeEnabled == 1 && enterStub() == 0);
    safeRelease([[BootstrapReleaseProbe alloc] initWithNumber:1]);
    safeRelease([[BootstrapReleaseProbe alloc] initWithNumber:2]);
    require(released == 0);
    purge();
    require(released == 2);
    enable(NO);
    require(*safeEnabled == 0);
    safeRelease([[BootstrapReleaseProbe alloc] initWithNumber:3]);
    require(released == 3);
    enable(YES);
    thread = cthread_fork((cthread_fn_t)worker, [[BootstrapReleaseProbe alloc] initWithNumber:4]);
    cthread_join(thread);
    require(released == 3);
    exitStub(enterStub());
    require(released == 4);
    safeRelease([[BootstrapReleaseProbe alloc] initWithNumber:5]);
    enable(NO);
    require(released == 4);
    enable(YES);
    purge();
    require(released == 5);
    puts("PASS main-thread deferred releases");
    [pool release];
    return 0;
}
