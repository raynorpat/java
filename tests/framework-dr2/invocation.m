#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static void *(*findClass)(void *, char *, int);
static void *(*currentEnv)(void);
static void *(*construct)(void *, char *, void *, char *, ...);
static void *(*lookup)(void *, void *, const char *, const char *, int, int);
static long (*invokeRaw)(void *, void *, void *, int, int, va_list, long *, void **);
#else
extern void *JAVALoadVirtualMachine(NSString *, id);
extern void *FindClass(void *, char *, int);
extern void *EE(void);
extern void *execute_java_constructor(void *, char *, void *, char *, ...);
extern void *_JAVAMethodLookup(void *, void *, const char *, const char *, int, int);
extern long __JAVAMethodInvokeNoExceptionConversion(void *, void *, void *, int, int, va_list, long *, void **);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define construct execute_java_constructor
#define lookup _JAVAMethodLookup
#define invokeRaw __JAVAMethodInvokeNoExceptionConversion
#endif

static void *env, *cls, *object;
static long high;
static void *exception;

static long invoke(const char *name, const char *signature, int isStatic, ...)
{
    void *method = lookup(env, cls, name, signature, !isStatic, isStatic);
    long result;
    va_list args;
    if (!method) { fprintf(stderr, "Missing method %s\n", name); exit(1); }
    va_start(args, isStatic);
    high = 0;
    exception = NULL;
    result = invokeRaw(env, isStatic ? cls : object, method, isStatic, 0, args, &high, &exception);
    va_end(args);
    return result;
}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    long low;
    union { long bits; float value; } single;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
    findClass = NSAddressOfSymbol(NSLookupAndBindSymbol("_FindClass"));
    currentEnv = NSAddressOfSymbol(NSLookupAndBindSymbol("_EE"));
    construct = NSAddressOfSymbol(NSLookupAndBindSymbol("_execute_java_constructor"));
    lookup = NSAddressOfSymbol(NSLookupAndBindSymbol("__JAVAMethodLookup"));
    invokeRaw = NSAddressOfSymbol(NSLookupAndBindSymbol("___JAVAMethodInvokeNoExceptionConversion"));
#endif
    if (!loadVM([NSString stringWithCString:argv[1]], nil)) return 1;
    env = currentEnv();
    cls = findClass(env, "InvocationFixture", 1);
    if (!cls) return 1;
    object = construct(env, NULL, cls, "()");
    if (!object) return 1;
    printf("instance=%ld\n", invoke("add", "(I)I", 0, -12));
    printf("static=%ld\n", invoke("addStatic", "(I)I", 1, 100));
    printf("inherited=%ld\n", invoke("inherited", "()I", 0));
    printf("declared-only=%d\n", lookup(env, cls, "inherited", "()I", 0, 0) == NULL);
    printf("static-not-instance=%d\n", lookup(env, cls, "addStatic", "(I)I", 0, 0) == NULL);
    printf("instance-not-static=%d\n", lookup(env, cls, "add", "(I)I", 0, 1) == NULL);
    printf("inherited-static=%d\n", lookup(env, cls, "inheritedStatic", "()I", 0, 1) == NULL);
    printf("null-name=%d\n", lookup(env, cls, NULL, NULL, 0, 0) == NULL);
    printf("signature-optional=%d\n", lookup(env, cls, "add", NULL, 0, 0) != NULL);
    low = invoke("wide", "(J)J", 1, 0x0102030405060708LL);
    printf("long=%08lx,%08lx\n", (unsigned long)low, (unsigned long)high);
    single.bits = invoke("single", "(F)F", 1, 2.25);
    printf("float=%.2f\n", single.value);
    single.value = 3.25f;
    single.bits = invokeRaw(env, cls, lookup(env, cls, "single", "(F)F", 0, 1),
        1, 1, (va_list)&single.value, &high, &exception);
    printf("short-float=%.2f\n", single.value);
    low = invoke("twice", "(D)D", 1, -1.25);
    printf("double=%08lx,%08lx\n", (unsigned long)low, (unsigned long)high);
    low = invoke("identity", "(Ljava/lang/Object;)Ljava/lang/Object;", 1, object);
    printf("object=%d\n", (void *)low == object);
    invoke("fail", "()V", 1);
    printf("exception=%d\n", exception != NULL);
    low = invoke("addStatic", "(I)I", 1, 1);
    printf("exception-cleared=%d,%ld\n", exception == NULL, low);
    low = 10;
    low = invokeRaw(NULL, cls, lookup(env, cls, "addStatic", "(I)I", 0, 1),
        1, 0, (va_list)&low, &high, &exception);
    printf("default-env=%ld\n", low);
    [pool release];
    return 0;
}
