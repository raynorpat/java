#import <AppKit/AppKit.h>
#include <mach/cthreads.h>
#include <stdio.h>
#include <stdlib.h>

extern void *NativeMethodEnter(void *);
extern void NativeMethodExit(void *, void *);
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static id mainContext;

@interface Warmup : NSObject
+ (void)run:(id)unused;
@end
@implementation Warmup
+ (void)run:(id)unused { }
@end

static void *worker(void *unused)
{
    void *outer = NativeMethodEnter(NULL);
    id context = [[[NSThread currentThread] threadDictionary] objectForKey:@"AwtThreadContext Key"];
    CHECK(outer != NULL && context != nil && context != mainContext);
    CHECK([NSDPSContext currentContext] != nil);
    CHECK(NativeMethodEnter(NULL) == NULL);
    NativeMethodExit(NULL, outer);
    CHECK(NativeMethodEnter(NULL) == outer);
    CHECK([[[NSThread currentThread] threadDictionary] objectForKey:@"AwtThreadContext Key"] == context);
    NativeMethodExit(NULL, outer);
    return NULL;
}

int main(void)
{
    id pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    /* Existing application-owned pools require no bridge exit token. */
    CHECK(NativeMethodEnter(NULL) == NULL);
    mainContext = [[[NSThread currentThread] threadDictionary] objectForKey:@"AwtThreadContext Key"];
    CHECK(mainContext != nil);
    /* Raw cthread_fork does not switch Foundation out of its single-thread
     * fast path. Enter that mode through the public Foundation thread API. */
    [NSThread detachNewThreadSelector:@selector(run:) toTarget:[Warmup class] withObject:nil];
    cthread_join(cthread_fork(worker, NULL));
    [pool release];
    puts("DR2 per-thread native entry and reentrancy passed");
    return 0;
}
