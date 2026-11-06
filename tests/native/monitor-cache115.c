/* Original monitor cache layout/capacity and reuse regression. */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "oobj.h"
#include "interpreter.h"
#include "monitor.h"
#include "monitor_cache.h"
#ifdef SUN_ORACLE
#include <windows.h>
#else
extern int monCount;
extern void expandMonCache(int);
#endif
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "monitor cache check line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static long keys[4096];
int main(int argc, char **argv)
{
    JavaVM *vm;
    JNIEnv *env;
    JDK1_1InitArgs args;
    int i, before;
    int *capacity;
    jint (JNICALL *defaults)(void *);
    jint (JNICALL *create)(JavaVM **, JNIEnv **, void *);
    void (*expand)(int);
    void (*enter)(unsigned int);
    void (*leave)(unsigned int);
#ifdef SUN_ORACLE
    HMODULE module = LoadLibraryA("javai_g.dll");
    CHECK(module != NULL);
    defaults = (void *)GetProcAddress(module, "JNI_GetDefaultJavaVMInitArgs");
    create = (void *)GetProcAddress(module, "JNI_CreateJavaVM");
    enter = (void *)GetProcAddress(module, "monitorEnter");
    leave = (void *)GetProcAddress(module, "monitorExit");
    expand = (void *)((char *)module + 0x370ed);
    capacity = (int *)((char *)module + 0x73ea8);
#else
    defaults = JNI_GetDefaultJavaVMInitArgs;
    create = JNI_CreateJavaVM;
    enter = monitorEnter; leave = monitorExit;
    expand = expandMonCache; capacity = &monCount;
    CHECK(offsetof(monitor_t, mid) == 8);
#endif
    CHECK(argc == 2);
    memset(&args, 0, sizeof(args)); args.version = 0x00010001;
    CHECK(defaults(&args) == 0);
    args.classpath = argv[1]; args.disableAsyncGC = 1;
    CHECK(create(&vm, &env, &args) == 0);
    CHECK(*capacity == 128);
#ifndef SUN_ORACLE
    CACHE_LOCK(); CACHE_LOCK();
    CHECK(CACHE_LOCKED());
    CACHE_UNLOCK(); CHECK(CACHE_LOCKED());
    CACHE_UNLOCK(); CHECK(!CACHE_LOCKED());
#endif
    expand(257);
    CHECK(*capacity == 385);
    before = *capacity;
    enter((unsigned int)&keys[0]);
    for (i = 1; i < 4096; ++i) {
        enter((unsigned int)&keys[i]);
        leave((unsigned int)&keys[i]);
    }
    leave((unsigned int)&keys[0]);
    CHECK(*capacity == before);
    puts("monitor cache layout, capacity and reuse passed");
    return 0;
}
