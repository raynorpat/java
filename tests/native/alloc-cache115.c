/* Cache invariants captured from Sun's active 1.1.5 allocator. */
#include <stdio.h>
#include <string.h>
#include "oobj.h"
#include "interpreter.h"
#include "typecodes.h"
#ifdef SUN_ORACLE
#include <windows.h>
#else
extern void gc(int, unsigned int);
#endif
#if defined(XP_PC) && !defined(SUN_ORACLE)
#include <windows.h>
#elif !defined(XP_PC)
#include <pthread.h>
#endif

static JavaVM *test_vm;
static volatile int attach_done;
static int attach_failed;
static void attach_loop(void)
{
    int i;
    JNIEnv *env;
    jbyteArray array;
    for (i = 0; i < 30; ++i) {
        if ((*test_vm)->AttachCurrentThread(test_vm, &env, NULL)) {
            attach_failed = 1; break;
        }
        array = (*env)->NewByteArray(env, 32);
        if (!array) attach_failed = 1;
        if ((*test_vm)->DetachCurrentThread(test_vm)) attach_failed = 1;
        if (attach_failed) break;
    }
    attach_done = 1;
}
#ifdef XP_PC
static DWORD WINAPI attach_entry(void *unused) { attach_loop(); return 0; }
#else
static void *attach_entry(void *unused) { attach_loop(); return NULL; }
#endif

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "cache check failed at line %d: %s\n", __LINE__, #test); \
    return 1; } } while (0)

int main(int argc, char **argv)
{
    JavaVM *vm;
    JNIEnv *env;
    JDK1_1InitArgs args;
    struct alloc_cache *cache;
    JHandle *first, *tail, *boundary;
    jbyteArray objects[80];
    jbyte bytes[100];
    long size;
    int i, j;
#ifdef XP_PC
    HANDLE thread;
#else
    pthread_t thread;
#endif
    jint (JNICALL *defaults)(void *);
    jint (JNICALL *create)(JavaVM **, JNIEnv **, void *);
    JHandle *(*array_alloc)(int, int);
    void (*cleanup)(struct alloc_cache *);
    void (*collect)(int, unsigned int);
#ifdef SUN_ORACLE
    HMODULE dll;
    if (argc != 3) return 2;
    dll = LoadLibraryA(argv[1]);
    if (!dll) return 3;
    defaults = (void *)GetProcAddress(dll, "JNI_GetDefaultJavaVMInitArgs");
    create = (void *)GetProcAddress(dll, "JNI_CreateJavaVM");
    array_alloc = (void *)GetProcAddress(dll, "ArrayAlloc");
    cleanup = (void *)((char *)dll + 0x278f0);
    collect = (void *)((char *)dll + 0x2c9a9);
#else
    if (argc != 2) return 2;
    defaults = JNI_GetDefaultJavaVMInitArgs;
    create = JNI_CreateJavaVM;
    array_alloc = ArrayAlloc;
    cleanup = allocCacheCleanup;
    collect = gc;
#endif
    memset(&args, 0, sizeof(args));
    args.version = 0x00010001;
    CHECK(defaults(&args) == 0);
    args.classpath = argv[argc - 1];
    CHECK(create(&vm, &env, &args) == 0);
    cache = &JNIEnv2EE(env)->alloc_cache;
    cleanup(cache);
    CHECK(!cache->cache_busy && !cache->cache_tail && !cache->cache_handles);
    first = array_alloc(T_BYTE, 1);
    CHECK(first && cache->cache_tail && cache->cache_size == 1024);
    CHECK((long)((JHandle *)cache->cache_tail)->methods == 40);
    CHECK(cache->cache_handles != NULL && !cache->cache_busy);
    tail = cache->cache_tail;
    size = cache->cache_size;
    boundary = array_alloc(T_BYTE, 252); /* rounded block is 256: not local */
    CHECK(boundary && cache->cache_tail == tail && cache->cache_size == size);
    boundary = array_alloc(T_BYTE, 244); /* rounded block is 248: local */
    CHECK(boundary == tail && cache->cache_size == size - 248);
    tail = cache->cache_tail;
    size = cache->cache_size;
    cache->cache_busy = 1;
    boundary = array_alloc(T_BYTE, 1);
    CHECK(boundary && cache->cache_tail == tail && cache->cache_size == size);
    collect(0, 0);
    CHECK(cache->cache_tail == tail && tail->obj && cache->cache_size == size);
    cache->cache_busy = 0;
    collect(0, 0);
    CHECK(!cache->cache_tail && !cache->cache_size && !cache->cache_handles);
    for (i = 0; i < 80; ++i) {
        objects[i] = (*env)->NewByteArray(env, 100);
        CHECK(objects[i]);
        (*env)->GetByteArrayRegion(env, objects[i], 0, 100, bytes);
        for (j = 0; j < 100; ++j)
            CHECK(bytes[j] == 0);
        memset(bytes, i + 1, 100);
        (*env)->SetByteArrayRegion(env, objects[i], 0, 100, bytes);
    }
    collect(0, 0);
    for (i = 0; i < 80; ++i) {
        (*env)->GetByteArrayRegion(env, objects[i], 0, 100, bytes);
        for (j = 0; j < 100; ++j)
            CHECK(bytes[j] == (jbyte)(i + 1));
    }
    CHECK(!cache->cache_busy && !cache->cache_tail && !cache->cache_handles);
    test_vm = vm;
#ifdef XP_PC
    thread = CreateThread(NULL, 0, attach_entry, NULL, 0, NULL);
    CHECK(thread);
#else
    CHECK(pthread_create(&thread, NULL, attach_entry, NULL) == 0);
#endif
    /* A native attachment must never borrow the main EE's active cache. */
    do {
        jbyteArray array = (*env)->NewByteArray(env, 32);
        CHECK(array);
        (*env)->DeleteLocalRef(env, array);
    } while (!attach_done);
#ifdef XP_PC
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
    CHECK(!attach_failed);
    puts("allocation-cache=ok");
    return 0;
}
