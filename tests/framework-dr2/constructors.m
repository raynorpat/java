#import <Foundation/Foundation.h>
#import "vm.h"
#include <mach-o/dyld.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static void *(*findClass)(void *, char *, int);
static void *(*currentEnv)(void);
static void *(*donorConstructor)(void *, char *, void *, char *, va_list, int);
static long (*dynamicCall)(void *, void *, char *, char *, ...);
static void *constructRaw(void *env, void *cls, const char *signature, int shortFloats, va_list args)
{
    return donorConstructor(env, NULL, cls, (char *)signature, args, shortFloats);
}
#else
extern void *FindClass(void *, char *, int);
extern void *EE(void);
extern long execute_java_dynamic_method(void *, void *, char *, char *, ...);
extern void *bridgeNewJavaObject(void *, void *, const char *, int, va_list);
#define loadVM JAVALoadVirtualMachine
#define findClass FindClass
#define currentEnv EE
#define dynamicCall execute_java_dynamic_method
#define constructRaw bridgeNewJavaObject
#endif

static void *env, *cls;
static void *construct(const char *signature, ...)
{
    va_list args;
    void *result;
    va_start(args, signature);
    result = constructRaw(env, cls, signature, 0, args);
    va_end(args);
    return result;
}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    JavaBridgeVM *data;
    JNIEnv *jni;
    void *object;
    char packed[24], *p = packed;
    float f = 1.25f;
    int i = 7;
    double d = -2.5;
    long long wide = 0x0000000300000004LL;
    if (argc != 2) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
    findClass = NSAddressOfSymbol(NSLookupAndBindSymbol("_FindClass"));
    currentEnv = NSAddressOfSymbol(NSLookupAndBindSymbol("_EE"));
    donorConstructor = NSAddressOfSymbol(NSLookupAndBindSymbol("_execute_java_constructor_vararg"));
    dynamicCall = NSAddressOfSymbol(NSLookupAndBindSymbol("_execute_java_dynamic_method"));
#endif
    data = loadVM([NSString stringWithCString:argv[1]], nil);
    if (!data) return 1;
    jni = data->env;
    env = currentEnv();
    cls = findClass(env, "ConstructorFixture", 1);
    if (!cls) return 1;
    object = construct("()");
    if (!object) return 1;
    printf("default=%ld\n", dynamicCall(env, object, "value", "()I"));
    object = construct("(FIDJ)", (double)f, i, d, wide);
    if (!object) return 1;
    printf("promoted=%ld\n", dynamicCall(env, object, "value", "()I"));
    memcpy(p, &f, 4); p += 4;
    memcpy(p, &i, 4); p += 4;
    memcpy(p, &d, 8); p += 8;
    memcpy(p, &wide, 8);
    object = constructRaw(env, cls, "(FIDJ)", 1, (va_list)packed);
    if (!object) return 1;
    printf("short-floats=%ld\n", dynamicCall(env, object, "value", "()I"));
    memset(packed, 0, 8);
    f = 1.5f;
    memcpy(packed + 8, &f, 4);
    object = constructRaw(env, cls, "(Ljava/lang/Object;[FF)", 1, (va_list)packed);
    if (!object) return 1;
    printf("reference-array-float=%ld\n", dynamicCall(env, object, "value", "()I"));
    construct("(I)", -1);
    printf("constructor-throws=%d\n", (*jni)->ExceptionOccurred(jni) != NULL);
    (*jni)->ExceptionClear(jni);
    object = constructRaw(env, cls, "(FZ)", 1, NULL);
    printf("missing-constructor=%d\n", !object && (*jni)->ExceptionOccurred(jni) != NULL);
    (*jni)->ExceptionClear(jni);
    object = construct("(I)", 9);
    if (!object) return 1;
    printf("after-exception=%ld\n", dynamicCall(env, object, "value", "()I"));
    [pool release];
    return 0;
}
