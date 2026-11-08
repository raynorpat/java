#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach/cthreads.h>
extern void *NSLoadVirtualMachine(NSString *);
extern void JAVAAttachCurrentThread(void *);
extern void JAVADetachCurrentThread(void *);
extern id NSJavaLookupHandle(void *);
@interface NSObject (FullBootstrapFixture)
- (int)add:(int)value;
- (NSString *)greeting:(NSString *)name;
- (int)collect;
- (void *)javaHandle;
@end
static id retainedProxy;
static void *loadedVM;
static any_t worker(any_t ignored)
{
    NSAutoreleasePool *pool;
    int valid;
    JAVAAttachCurrentThread(loadedVM);
    pool = [NSAutoreleasePool new];
    valid = [retainedProxy collect] == 7 && [retainedProxy add:5] == 12;
    [pool release];
    JAVADetachCurrentThread(loadedVM);
    return (any_t)valid;
}
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    Class clazz;
    id object;
    unsigned i;
    void *handle;
    cthread_t thread;
    if (argc != 2) return 2;
    loadedVM = NSLoadVirtualMachine([NSString stringWithCString:argv[1]]);
    if (!loadedVM) return 1;
    clazz = NSClassFromString(@"ProxyFixture");
    if (!clazz) return 1;
    object = [[clazz alloc] init];
    printf("startup=%d\n", [object add:5]);
    printf("string=%s\n", [[object greeting:@"private framework"] cString]);
    retainedProxy = object;
    handle = [object javaHandle];
    if ([object collect] != 7 || NSJavaLookupHandle(handle) != object) return 2;
    thread = cthread_fork(worker, NULL);
    if (!(int)cthread_join(thread) || NSJavaLookupHandle(handle) != object) return 3;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "/System/Library/Frameworks/JavaVM.framework") || strstr(path, "/usr/lib/java/")) {
            fprintf(stderr, "Unexpected donor image: %s\n", path); return 1;
        }
    }
    [object release];
    [object release];
    if (NSJavaLookupHandle(handle)) return 4;
    puts("ownership-gc-thread=1");
    [pool release];
    return 0;
}
