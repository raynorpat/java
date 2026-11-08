#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
@interface NSObject (ExceptionProbe)
- (void *)javaException;
- (NSString *)stackTrace;
@end
extern int checkExceptionState(void *, void *);
#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static void *(*findClass)(void *, char *, int);
static void *(*currentEnv)(void);
static void (*initGC)(void), (*initExceptions)(void), (*initMaps)(void);
static id (*toObjc)(void *);
static void *(*toJava)(id);
static void (*mapObjects)(void *, id, BOOL);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invokeRaw)(void *, void *, void *, int, int, va_list, long *, void **);
static long (*invokeChecked)(void *, void *, void *, int, int, va_list, long *);
static void *(*newObject)(void *, void *, const char *, int, va_list);
static void *(*retainObject)(void *);
static void *(*raiseInJava)(void *, id);
#else
extern void *JAVALoadVirtualMachine(NSString *, id), *FindClass(void *, char *, int), *EE(void);
extern void JavaGCInitialization(void), ExceptionInitialization(void), bridgeInitializeObjectMap(void);
extern id _JavaExceptionToNSException(void *);
extern void *_NSExceptionToJavaException(id);
extern void NSJavaMapObjects(void *, id, BOOL);
extern id NSJavaLookupHandle(void *);
extern void *NSJavaLookupId(id);
extern void *_JAVAMethodLookup(void *, void *, const char *, const char *, int, int);
extern long __JAVAMethodInvokeNoExceptionConversion(void *, void *, void *, int, int, va_list, long *, void **);
extern long __JAVAMethodInvoke(void *, void *, void *, int, int, va_list, long *);
extern void *JAVANewObject(void *, void *, const char *, int, va_list);
extern void *NSRetainJavaObject(void *);
extern void *_NSRaiseExceptionInJava(void *, id);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define initGC JavaGCInitialization
#define initExceptions ExceptionInitialization
#define initMaps bridgeInitializeObjectMap
#define toObjc _JavaExceptionToNSException
#define toJava _NSExceptionToJavaException
#define mapObjects NSJavaMapObjects
#define lookup _JAVAMethodLookup
#define invokeRaw __JAVAMethodInvokeNoExceptionConversion
#define invokeChecked __JAVAMethodInvoke
#define newObject JAVANewObject
#define retainObject NSRetainJavaObject
#define raiseInJava _NSRaiseExceptionInJava
/* Component boundary: only explicit identities, never fabricated proxies. */
id NSJavaHandleToId(void *handle)
{
    id value = NSJavaLookupHandle(handle);
    if (handle && !value) abort();
    return value;
}
void *NSIdToJavaHandle(id value)
{
    void *handle = NSJavaLookupId(value);
    if (value && !handle) abort();
    return handle;
}
#endif
static void *env, *fixture;
static long call(const char *name, const char *signature, ...)
{
    void *method = lookup(env, fixture, name, signature, 0, 1), *exception;
    long result, high;
    va_list args;
    if (!method) exit(1);
    va_start(args, signature);
    result = invokeRaw(env, fixture, method, 1, 0, args, &high, &exception);
    va_end(args);
    if (exception) exit(1);
    return result;
}
static long rootCount(void)
{
    void *runtime = findClass(env, "com/apple/yellow/foundation/NSRuntime", 1), *exception;
    long high, count = invokeRaw(env, runtime, lookup(env, runtime, "count", "()I", 0, 1),
        1, 0, NULL, &high, &exception);
    if (exception) exit(1);
    return count;
}
int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSDictionary *info = [NSDictionary dictionaryWithObject:@"value" forKey:@"key"];
    void *token, *handle, *converted;
    NSException *exception, *decoded;
    NSData *archive;
    NSAutoreleasePool *inner;
    long roots;
    unsigned i;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
#define BIND(var, symbol) var = NSAddressOfSymbol(NSLookupAndBindSymbol(symbol))
    BIND(loadVM, "_JAVALoadVirtualMachine");
    BIND(findClass, "_FindClass"); BIND(currentEnv, "_EE");
    BIND(initGC, "_JavaGCInitialization"); BIND(initExceptions, "_ExceptionInitialization");
    BIND(initMaps, "_MappingInitialization");
    BIND(toObjc, "__JavaExceptionToNSException"); BIND(toJava, "__NSExceptionToJavaException");
    BIND(mapObjects, "_NSJavaMapObjects"); BIND(lookup, "__JAVAMethodLookup");
    BIND(invokeRaw, "___JAVAMethodInvokeNoExceptionConversion");
    BIND(invokeChecked, "___JAVAMethodInvoke"); BIND(newObject, "_JAVANewObject");
    BIND(retainObject, "_NSRetainJavaObject");
    BIND(raiseInJava, "__NSRaiseExceptionInJava");
#endif
    if (!loadVM([NSString stringWithCString:argv[1]], nil)) return 1;
    env = currentEnv(); initGC(); initExceptions(); initMaps();
    fixture = findClass(env, "ExceptionFixture", 1);
    if (!fixture) return 1;
    token = (void *)call("token", "()Ljava/lang/Object;");
    mapObjects(token, info, NO);
    printf("nil=%d,%d\n", toObjc(NULL) == nil, toJava(nil) == NULL);
    for (i = 0; i < 6; ++i) {
        handle = (void *)call("make", "(ILjava/lang/Object;)Ljava/lang/Throwable;", i, token);
        exception = toObjc(handle);
        printf("case%u=%s|%s|%d|%d\n", i, [[exception name] cString],
            [exception reason] ? [[exception reason] cString] : "<nil>",
            [exception userInfo] == (i == 3 ? info : nil), toJava(exception) == handle);
        if (i == 0) {
            printf("trace=%d\n", [[exception stackTrace] rangeOfString:@"bad argument"].length != 0);
            printf("description=%d\n", [[exception description] hasPrefix:@"java/lang/IllegalArgumentException: bad argument\nStack Trace:\n"]);
            archive = [NSArchiver archivedDataWithRootObject:exception];
            decoded = [NSUnarchiver unarchiveObjectWithData:archive];
            printf("coding=%d,%d,%d\n", [[decoded name] isEqual:[exception name]],
                [[decoded reason] isEqual:[exception reason]], [decoded javaException] == NULL);
        }
    }
    exception = [NSException exceptionWithName:@"ObjcName" reason:@"ObjcReason" userInfo:info];
    converted = toJava(exception);
    exception = toObjc(converted);
    printf("objc-roundtrip=%s|%s|%d\n", [[exception name] cString], [[exception reason] cString], [exception userInfo] == info);
    printf("raise-in-java=%d\n", raiseInJava(env, exception) == converted && checkExceptionState(env, converted));
    NS_DURING
        invokeChecked(env, fixture, lookup(env, fixture, "fail", "()V", 0, 1), 1, 0, NULL, NULL);
        puts("method=not-raised");
    NS_HANDLER
        printf("method=%s|%s\n", [[localException name] cString], [[localException reason] cString]);
    NS_ENDHANDLER
    NS_DURING
        newObject(env, findClass(env, "ExceptionCtor", 1), "()", 0, NULL);
        puts("constructor=not-raised");
    NS_HANDLER
        printf("constructor=%s|%s\n", [[localException name] cString], [[localException reason] cString]);
    NS_ENDHANDLER
    NS_DURING
        retainObject(NULL);
        puts("retain-nil=not-raised");
    NS_HANDLER
        printf("retain-nil=%s\n", [[localException name] cString]);
    NS_ENDHANDLER
    printf("recovery=%d\n", call("token", "()Ljava/lang/Object;") != 0);
    roots = rootCount();
    inner = [NSAutoreleasePool new];
    handle = (void *)call("make", "(ILjava/lang/Object;)Ljava/lang/Throwable;", 0, NULL);
    exception = toObjc(handle);
    printf("rooted=%d\n", rootCount() == roots + 1 && [exception javaException] == handle);
    [inner release];
    printf("unrooted=%d\n", rootCount() == roots);
#ifndef DONOR
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "JavaVM.framework") || strstr(path, "libObjCJava")) return 1;
    }
#endif
    [pool release];
    return 0;
}
