#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static void *(*findClass)(void *, char *, int);
static void *(*currentEnv)(void);
static void (*initializeGC)(void);
static void *(*retainJava)(void *);
static void (*releaseJava)(void *);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invokeRaw)(void *, void *, void *, int, int, va_list, long *, void **);
#else
extern void *JAVALoadVirtualMachine(NSString *, id);
extern void *FindClass(void *, char *, int);
extern void *EE(void);
extern void JavaGCInitialization(void);
extern void *bridgeRetainJavaObject(void *);
extern void bridgeReleaseJavaObject(void *);
extern void *_JAVAMethodLookup(void *, void *, const char *, const char *, int, int);
extern long __JAVAMethodInvokeNoExceptionConversion(void *, void *, void *, int, int, va_list, long *, void **);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initializeGC JavaGCInitialization
#define retainJava bridgeRetainJavaObject
#define releaseJava bridgeReleaseJavaObject
#define lookup _JAVAMethodLookup
#define invokeRaw __JAVAMethodInvokeNoExceptionConversion
#endif
static void *env, *runtime;
static long call(const char *name, const char *signature, ...)
{
    void *method = lookup(env, runtime, name, signature, 0, 1), *exception;
    long result, high;
    va_list args;
    if (!method) exit(1);
    va_start(args, signature);
    result = invokeRaw(env, runtime, method, 1, 0, args, &high, &exception);
    va_end(args);
    if (exception) { fprintf(stderr, "Java fixture exception in %s\n", name); exit(1); }
    return result;
}
static void retainBatch(void)
{
    int i;
    for (i = 1; i <= 100; ++i) retainJava((void *)call("make", "(I)Ljava/lang/Object;", i));
}
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    void *object;
    unsigned i;
    if (argc != 2 && argc != 3) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
    findClass = NSAddressOfSymbol(NSLookupAndBindSymbol("_FindClass"));
    currentEnv = NSAddressOfSymbol(NSLookupAndBindSymbol("_EE"));
    initializeGC = NSAddressOfSymbol(NSLookupAndBindSymbol("_JavaGCInitialization"));
    retainJava = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSRetainJavaObject"));
    releaseJava = NSAddressOfSymbol(NSLookupAndBindSymbol("_NSReleaseJavaObject"));
    lookup = NSAddressOfSymbol(NSLookupAndBindSymbol("__JAVAMethodLookup"));
    invokeRaw = NSAddressOfSymbol(NSLookupAndBindSymbol("___JAVAMethodInvokeNoExceptionConversion"));
#endif
    if (!loadVM([NSString stringWithCString:argv[1]], nil)) return 1;
    env = currentEnv();
    if (argc == 3) {
        NS_DURING
            initializeGC();
            printf("missing-methods=not-rejected\n");
        NS_HANDLER
            printf("missing-methods=%s:%s\n", [[localException name] cString], [[localException reason] cString]);
        NS_ENDHANDLER
        [pool release];
        return 0;
    }
    initializeGC();
    runtime = findClass(env, "com/apple/yellow/foundation/NSRuntime", 1);
    if (!runtime) return 1;
    printf("empty=%ld\n", call("count", "()I"));
    object = (void *)call("make", "(I)Ljava/lang/Object;", 37);
    printf("identity=%d\n", retainJava(object) == object);
    retainJava(object);
    printf("duplicate-count=%ld\n", call("count", "()I"));
    printf("retained-value=%ld\n", call("collectAndSum", "()I"));
    releaseJava(object);
    printf("released-count=%ld\n", call("count", "()I"));
    releaseJava(object);
    releaseJava(NULL);
    printf("absent-release=%ld\n", call("count", "()I"));
    object = NULL;
    retainBatch();
    printf("batch-count=%ld\n", call("count", "()I"));
    printf("batch-after-gc=%ld\n", call("collectAndSum", "()I"));
    call("releaseAll", "()V");
    printf("cleared=%ld\n", call("count", "()I"));
#ifndef DONOR
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "JavaVM.framework") || strstr(path, "libObjCJava")) {
            fprintf(stderr, "Unexpected donor dependency: %s\n", path);
            return 1;
        }
    }
#endif
    [pool release];
    return 0;
}
