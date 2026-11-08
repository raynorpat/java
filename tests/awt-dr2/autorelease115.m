#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>

extern void NativeMethodExit(void *, void *);
static int released;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

@interface ReleaseProbe : NSObject
@end
@implementation ReleaseProbe
- (void)dealloc { ++released; [super dealloc]; }
@end

int main(void)
{
    id pool = [[NSClassFromString(@"AwtAutoreleasePool") alloc] init];
    int i;
    CHECK(pool != nil);
    /* Empty exits must not advance the deferred release counter. */
    for (i = 0; i < 15; ++i) NativeMethodExit(NULL, pool);
    [[[ReleaseProbe alloc] init] autorelease];
    for (i = 0; i < 10; ++i) NativeMethodExit(NULL, pool);
    CHECK(released == 0);
    NativeMethodExit(NULL, pool);
    CHECK(released == 1);
    [[[ReleaseProbe alloc] init] autorelease];
    for (i = 0; i < 10; ++i) NativeMethodExit(NULL, pool);
    CHECK(released == 1);
    NativeMethodExit(NULL, pool);
    CHECK(released == 2);
    /* Explicit nested Foundation pools preserve their own release boundary. */
    {
        id nested = [[NSAutoreleasePool alloc] init];
        [[[ReleaseProbe alloc] init] autorelease];
        CHECK(released == 2);
        [nested release];
        CHECK(released == 3);
    }
    [pool release];
    CHECK(released == 3);
    puts("DR2 deferred autorelease and nested pools passed");
    return 0;
}
