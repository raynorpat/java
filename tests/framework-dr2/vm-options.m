/* Capture the JNI arguments produced by the original and rebuilt adapters. */
#import <Foundation/Foundation.h>
#include "jni.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DONOR
#include <mach-o/dyld.h>
static void *(*JAVALoadVirtualMachine)(NSString *, id);
#else

extern void *JAVALoadVirtualMachine(NSString *, id);
#endif
static const char *scenario;

jint JNI_GetDefaultJavaVMInitArgs(void *opaque)
{
    JDK1_1InitArgs *args = opaque;
    memset(args, 0, sizeof(*args));
    args->version = 0x10001;
    args->checkSource = 11;
    args->nativeStackSize = 12;
    args->javaStackSize = 13;
    args->minHeapSize = 14;
    args->maxHeapSize = 15;
    args->verifyMode = 16;
    args->enableClassGC = 17;
    args->enableVerboseGC = 18;
    args->disableAsyncGC = 19;
    args->verbose = 20;
    args->debugging = 21;
    args->debugPort = 22;
    return 0;
}

jint JNI_CreateJavaVM(JavaVM **vm, JNIEnv **env, void *opaque)
{
    JDK1_1InitArgs *a = opaque;
    printf("%s=%x,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",
        scenario, a->version, a->checkSource, a->nativeStackSize,
        a->javaStackSize, a->minHeapSize, a->maxHeapSize, a->verifyMode,
        a->enableClassGC, a->enableVerboseGC, a->disableAsyncGC,
        a->verbose, a->debugging, a->debugPort, a->classpath);
    *vm = (JavaVM *)0x1234;
    *env = (JNIEnv *)0x5678;
    return 0;
}

int main(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    const char *values[] = {"YES", "yes", "true", "1", "NO"};
    unsigned i;
    void **data;
    NSMutableDictionary *options = [NSMutableDictionary dictionary];
#ifdef DONOR
    if (!NSAddLibrary("/usr/lib/java/libObjCJava.A.dylib")) return 1;
    JAVALoadVirtualMachine = (void *(*)(NSString *, id))
        NSAddressOfSymbol(NSLookupAndBindSymbol("_JAVALoadVirtualMachine"));
#endif
    scenario = "defaults";
    data = JAVALoadVirtualMachine(@"fixture path:classes.zip", nil);
    if (!data || data[0] != (void *)0x1234 || data[1] != (void *)0x5678) return 1;
    free(data);
    [options setObject:@"2048" forKey:@"NSJavaNativeStackSize"];
    [options setObject:@"4096" forKey:@"NSJavaStackSize"];
    [options setObject:@"8192" forKey:@"NSJavaMinHeapSize"];
    [options setObject:@"16384" forKey:@"NSJavaMaxHeapSize"];
    [options setObject:@"-1" forKey:@"NSJavaDebugPort"];
    for (i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
        NSString *value = [NSString stringWithCString:values[i]];
        scenario = values[i];
        [options setObject:value forKey:@"NSJavaCheckSource"];
        [options setObject:value forKey:@"NSJavaVerifyMode"];
        [options setObject:value forKey:@"NSJavaEnableClassGC"];
        [options setObject:value forKey:@"NSJavaEnableVerboseGC"];
        [options setObject:value forKey:@"NSJavaDisableAsyncGC"];
        [options setObject:value forKey:@"NSJavaVerbose"];
        [options setObject:value forKey:@"NSJavaDebugging"];
        data = JAVALoadVirtualMachine(@"fixture path:classes.zip", options);
        if (!data || data[0] != (void *)0x1234 || data[1] != (void *)0x5678) return 1;
        free(data);
    }
    [pool release];
    return 0;
}
