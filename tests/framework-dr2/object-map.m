#import <Foundation/Foundation.h>
#include <stdio.h>
#include <mach/cthreads.h>
#ifdef DONOR
#include <mach-o/dyld.h>
#import <JavaVM/JavaVM.h>
static void (*mapObjects)(void *, id, BOOL);
static void (*removeMapping)(void *, id, BOOL);
static void *(*lookupId)(id);
static id (*lookupHandle)(void *);
#else
#import "object-map.h"
#define mapObjects NSJavaMapObjects
#define removeMapping NSJavaRemoveMapping
#define lookupId NSJavaLookupId
#define lookupHandle NSJavaLookupHandle
#endif

struct Worker { id object; int handle; unsigned failures; };
static any_t exerciseMap(any_t data)
{
    struct Worker *worker = data;
    unsigned i;
    for (i = 0; i < 2000; ++i) {
        mapObjects(&worker->handle, worker->object, NO);
        if (lookupId(worker->object) != &worker->handle ||
            lookupHandle(&worker->handle) != worker->object) ++worker->failures;
        removeMapping(&worker->handle, worker->object, NO);
        if (lookupId(worker->object) || lookupHandle(&worker->handle)) ++worker->failures;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    id first = [NSObject new], second = [NSObject new];
    int handles[3];
    unsigned count = [first retainCount];
    struct Worker workers[2];
    cthread_t threads[2];
    if (argc != 2) return 2;
#ifdef DONOR
    [[[NSJavaVirtualMachine alloc] initWithClassPath:[NSString stringWithCString:argv[1]]] autorelease];
    mapObjects = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaMapObjects"));
    removeMapping = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaRemoveMapping"));
    lookupId = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaLookupId"));
    lookupHandle = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSJavaLookupHandle"));
#else
    bridgeInitializeObjectMap();
#endif
    printf("missing=%d,%d\n", lookupId(first) == NULL, lookupHandle(handles) == nil);
    mapObjects(handles, first, NO);
    printf("identity=%d,%d\n", lookupId(first) == handles, lookupHandle(handles) == first);
    printf("nonowning=%d\n", [first retainCount] == count);
    mapObjects(handles, first, YES);
    printf("retain=%d\n", [first retainCount] == count + 1);
    removeMapping(handles, first, YES);
    printf("remove=%d,%d,%d\n", lookupId(first) == NULL, lookupHandle(handles) == nil, [first retainCount] == count);
    mapObjects(handles, first, NO);
    mapObjects(handles + 1, first, NO);
    printf("replace=%d,%d,%d\n", lookupId(first) == handles + 1, lookupHandle(handles) == first, lookupHandle(handles + 1) == first);
    removeMapping(handles, nil, NO);
    printf("remove-java-only=%d,%d\n", lookupHandle(handles) == nil, lookupId(first) == handles + 1);
    removeMapping(NULL, first, NO);
    printf("remove-objc-only=%d,%d\n", lookupId(first) == NULL, lookupHandle(handles + 1) == first);
    removeMapping(handles + 1, nil, NO);
    mapObjects(handles + 2, second, NO);
    printf("second=%d,%d\n", lookupId(second) == handles + 2, lookupHandle(handles + 2) == second);
    removeMapping(handles + 2, second, NO);
    removeMapping(NULL, nil, YES);
    printf("nil=%d,%d\n", lookupId(nil) == NULL, lookupHandle(NULL) == nil);
    workers[0].object = first;
    workers[1].object = second;
    workers[0].failures = workers[1].failures = 0;
    threads[0] = cthread_fork(exerciseMap, &workers[0]);
    threads[1] = cthread_fork(exerciseMap, &workers[1]);
    cthread_join(threads[0]);
    cthread_join(threads[1]);
    printf("concurrent=%d\n", !workers[0].failures && !workers[1].failures);
    [first release];
    [second release];
    [pool release];
    return 0;
}
