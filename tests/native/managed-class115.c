/* Managed Class lifetime: primary DLL and reconstruction share this probe. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oobj.h"
#include "interpreter.h"
#ifdef XP_PC
#include <windows.h>
#else
#include <pthread.h>
#endif
#ifndef SUN_ORACLE
extern void gc(int, unsigned int);
#endif

static JavaVM *vm;
static jclass held_class;
static jobject held_instance;
static char *class_bytes;
static int class_size, mode, worker_failed;
static ClassClass ***registry;
static long *registry_count;

static void worker(void)
{
    JNIEnv *env;
    jclass loader_class, payload;
    jmethodID ctor;
    jobject loader, instance;
    jfieldID marker;
    if ((*vm)->AttachCurrentThread(vm, &env, NULL)) {
        worker_failed = 1; return;
    }
    if (!mode) {
        loader_class = (*env)->FindClass(env, "ManagedLoader115");
        if (!loader_class) goto failed;
        ctor = (*env)->GetMethodID(env, loader_class, "<init>", "()V");
        if (!ctor) goto failed;
        loader = (*env)->NewObject(env, loader_class, ctor);
        if (!loader) goto failed;
        payload = (*env)->DefineClass(env, "ManagedPayload115", loader,
                                     class_bytes, class_size);
        if (!payload) goto failed;
        marker = (*env)->GetStaticFieldID(env, payload, "marker", "[B");
        if (!marker || !(*env)->GetStaticObjectField(env, payload, marker))
            goto failed;
        held_class = (jclass)(*env)->NewGlobalRef(env, payload);
        if (!held_class) goto failed;
    } else {
        ctor = (*env)->GetMethodID(env, held_class, "<init>", "()V");
        if (!ctor) goto failed;
        instance = (*env)->NewObject(env, held_class, ctor);
        if (!instance) goto failed;
        held_instance = (*env)->NewGlobalRef(env, instance);
        if (!held_instance) goto failed;
    }
    (*vm)->DetachCurrentThread(vm);
    return;
failed:
    (*env)->ExceptionDescribe(env);
    worker_failed = 1;
    (*vm)->DetachCurrentThread(vm);
}

#ifdef XP_PC
static DWORD WINAPI thread_entry(void *unused) { worker(); return 0; }
static int run_worker(void)
{
    HANDLE thread = CreateThread(NULL, 0, thread_entry, NULL, 0, NULL);
    if (!thread) return 0;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return !worker_failed;
}
#else
static void *thread_entry(void *unused) { worker(); return NULL; }
static int run_worker(void)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_entry, NULL)) return 0;
    pthread_join(thread, NULL);
    return !worker_failed;
}
#endif

static int payload_count(void)
{
    int i, count = 0;
    for (i = 0; i < *registry_count; ++i)
        if (!strcmp(cbName((*registry)[i]), "ManagedPayload115")) ++count;
    return count;
}

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "class check failed at line %d: %s\n", __LINE__, #test); \
    return 1; } } while (0)

int main(int argc, char **argv)
{
    JNIEnv *env;
    JDK1_1InitArgs args;
    FILE *file;
    jint (JNICALL *defaults)(void *);
    jint (JNICALL *create)(JavaVM **, JNIEnv **, void *);
    void (*collect)(int, unsigned int);
    int offset = 0, enabled;
    jfieldID marker;
    jbyteArray marker_array;
    jbyte marker_bytes[2];
#ifdef SUN_ORACLE
    HMODULE dll;
    offset = 1;
    if (argc != 5) return 2;
    dll = LoadLibraryA(argv[1]);
    if (!dll) return 3;
    defaults = (void *)GetProcAddress(dll, "JNI_GetDefaultJavaVMInitArgs");
    create = (void *)GetProcAddress(dll, "JNI_CreateJavaVM");
    collect = (void *)((char *)dll + 0x2c9a9);
    registry = (ClassClass ***)((char *)dll + 0x83550);
    registry_count = (long *)((char *)dll + 0x8351c);
#else
    if (argc != 4) return 2;
    defaults = JNI_GetDefaultJavaVMInitArgs;
    create = JNI_CreateJavaVM;
    collect = gc;
    registry = &binclasses;
    registry_count = &nbinclasses;
#endif
    file = fopen(argv[offset + 2], "rb");
    CHECK(file);
    fseek(file, 0, SEEK_END);
    class_size = ftell(file);
    rewind(file);
    class_bytes = malloc(class_size);
    CHECK(class_bytes && fread(class_bytes, 1, class_size, file) == class_size);
    fclose(file);
    memset(&args, 0, sizeof(args));
    args.version = 0x00010001;
    CHECK(defaults(&args) == 0);
    args.classpath = argv[offset + 1];
    enabled = atoi(argv[offset + 3]);
    args.enableClassGC = enabled;
    CHECK(create(&vm, &env, &args) == 0);
    CHECK(payload_count() == 0);
    CHECK(run_worker());
    collect(0, 0);
    CHECK(payload_count() == 1); /* Global Class keeps its loader alive. */
    marker = (*env)->GetStaticFieldID(env, held_class, "marker", "[B");
    CHECK(marker);
    marker_array = (jbyteArray)(*env)->GetStaticObjectField(env, held_class, marker);
    CHECK(marker_array);
    (*env)->GetByteArrayRegion(env, marker_array, 0, 2, marker_bytes);
    CHECK(marker_bytes[0] == 17 && marker_bytes[1] == 34);
    (*env)->DeleteLocalRef(env, marker_array);
    mode = 1;
    CHECK(run_worker());
    (*env)->DeleteGlobalRef(env, held_class);
    held_class = NULL;
    collect(0, 0);
    CHECK(payload_count() == 1); /* Instance keeps its defining Class alive. */
    (*env)->DeleteGlobalRef(env, held_instance);
    held_instance = NULL;
    collect(0, 0);
    CHECK(payload_count() == (enabled ? 0 : 1));
    printf("managed-class=%s\n", enabled ? "unloaded" : "retained");
    free(class_bytes);
    return 0;
}
