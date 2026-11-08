#import <Foundation/Foundation.h>
#import "vm.h"
#include <mach-o/dyld.h>
#include <mach/cthreads.h>
#include <stdio.h>
#include <string.h>

#ifdef DONOR
static void *(*loadVM)(NSString *, id);
static jclass (*addClass)(void *, const char *, const void *, unsigned);
static void (*attachThread)(void *), (*detachThread)(void *);
#else
#define loadVM JAVALoadVirtualMachine
#define addClass JAVAAddClassFile
#define attachThread JAVAAttachCurrentThread
#define detachThread JAVADetachCurrentThread
#endif

static any_t worker(any_t opaque)
{
    JavaBridgeVM *data = (JavaBridgeVM *)opaque;
    JNIEnv *env = NULL;
    jclass cls;
    jmethodID method;
    int value = 0;
    attachThread(data);
    if ((*data->vm)->AttachCurrentThread(data->vm, &env, NULL) == 0 && env) {
        cls = (*env)->FindClass(env, "AdapterFixture");
        method = cls ? (*env)->GetStaticMethodID(env, cls, "value", "()I") : NULL;
        if (method) value = (*env)->CallStaticIntMethod(env, cls, method);
    }
    detachThread(data);
    return (any_t)value;
}

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    JavaBridgeVM *data;
    NSData *bytes;
    jclass cls;
    jmethodID method;
    JNIEnv *env;
    unsigned i;
    int value;
    if (argc != 3) return 2;
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    loadVM = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
    addClass = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVAAddClassFile"));
    attachThread = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVAAttachCurrentThread"));
    detachThread = NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVADetachCurrentThread"));
#endif
    data = loadVM([NSString stringWithCString:argv[1]], nil);
    if (!data || !data->vm || !data->env) return 1;
    env = data->env;
    bytes = [NSData dataWithContentsOfFile:[NSString stringWithCString:argv[2]]];
    if (!bytes) return 1;
    cls = addClass(data, "AdapterFixture", [bytes bytes], [bytes length]);
    if (!cls) { (*env)->ExceptionDescribe(env); return 1; }
    method = (*env)->GetStaticMethodID(env, cls, "value", "()I");
    if (!method) return 1;
    value = (*env)->CallStaticIntMethod(env, cls, method);
    printf("defined-class=%d\n", value);
    if (value != 115118 || (*env)->ExceptionOccurred(env)) return 1;
    cls = addClass(data, "Broken", "bad", 3);
    printf("invalid-class=%d\n", !cls && (*env)->ExceptionOccurred(env) != NULL);
    if (cls || !(*env)->ExceptionOccurred(env)) return 1;
    (*env)->ExceptionClear(env);
    value = (int)cthread_join(cthread_fork(worker, data));
    printf("attached-thread=%d\n", value);
    if (value != 115118) return 1;
#ifndef DONOR
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        if (strstr(path, "JavaVM.framework") || strstr(path, "/usr/lib/java/")) {
            fprintf(stderr, "Unexpected donor dependency: %s\n", path);
            return 1;
        }
    }
#endif
    [pool release];
    return 0;
}
