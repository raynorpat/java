/* JNI-only oracle: run unchanged against Sun's and the reconstructed VM.
 * Sun javai_g.dll 100131C0/100135F7 notify after local-reference cleanup,
 * and the synchronized invoker releases the receiver monitor first. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jni.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
typedef void (*ExceptionHook)(void *, unsigned char *, void *);
typedef void (JNICALL *SetHooks)(void *, void *, ExceptionHook);
static JNIEnv *callback_env;
static jobject receiver;
static jthrowable thrown;
static jclass monitor_exception_class;
static void *expected_pc;
static int callbacks, callback_error, check_monitor, monitor_released;

static void JNICALL throw_plain(JNIEnv *env, jobject object)
{
    (*env)->GetObjectClass(env, object);
    (*env)->Throw(env, thrown);
}
static void JNICALL throw_locked(JNIEnv *env, jobject object)
{
    (*env)->GetObjectClass(env, object);
    (*env)->Throw(env, thrown);
}
static void JNICALL return_normally(JNIEnv *env, jobject object) { }

static void exception_hook(void *ee, unsigned char *pc, void *exception)
{
    JNIEnv *env = callback_env;
    jthrowable pending, monitor_exception;
    if (pc != (unsigned char *)expected_pc) return;
    callbacks++;
    pending = (*env)->ExceptionOccurred(env);
    if (!pending || !exception || !(*env)->IsSameObject(env, pending, thrown)) {
        callback_error = 1;
        return;
    }
    if (!check_monitor) return;
    /* A current-thread exit must fail after the synchronized invoker's exit.
     * Preserve the pending exception while probing the JNI monitor contract.
     * Restore ownership if a broken invoker still held it, avoiding imbalance. */
    (*env)->ExceptionClear(env);
    (*env)->MonitorExit(env, receiver);
    monitor_exception = (*env)->ExceptionOccurred(env);
    monitor_released = monitor_exception &&
        (*env)->IsInstanceOf(env, monitor_exception, monitor_exception_class);
    (*env)->ExceptionClear(env);
    if (!monitor_exception) (*env)->MonitorEnter(env, receiver);
    if (monitor_exception) (*env)->DeleteLocalRef(env, monitor_exception);
    (*env)->Throw(env, pending);
    (*env)->DeleteLocalRef(env, pending);
}

int main(int argc, char **argv)
{
    JavaVM *vm;
    JNIEnv *env;
    JDK1_1InitArgs args;
    jint (JNICALL *defaults)(void *);
    jint (JNICALL *create)(JavaVM **, JNIEnv **, void *);
    SetHooks hooks;
    jclass clazz, exception_class;
    jmethodID plain, locked, success, constructor;
    JNINativeMethod methods[3];
    jthrowable pending;
    int i, expect_hooks;
#ifdef _WIN32
    HMODULE library;
    CHECK(argc == 4);
    library = LoadLibraryExA(argv[1], NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    CHECK(library);
    defaults = (void *)GetProcAddress(library, "JNI_GetDefaultJavaVMInitArgs");
    create = (void *)GetProcAddress(library, "JNI_CreateJavaVM");
    hooks = (SetHooks)GetProcAddress(library, "_set_debugger_hooks@12");
#else
    void *library;
    CHECK(argc == 4);
    library = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
    CHECK(library);
    defaults = dlsym(library, "JNI_GetDefaultJavaVMInitArgs");
    create = dlsym(library, "JNI_CreateJavaVM");
    hooks = dlsym(library, "set_debugger_hooks");
#endif
    expect_hooks = !strcmp(argv[3], "Debug");
    CHECK(defaults && create);
    CHECK((hooks != NULL) == expect_hooks);
    memset(&args, 0, sizeof(args));
    args.version = 0x10001;
    CHECK(defaults(&args) == JNI_OK);
    args.classpath = argv[2];
    args.debugging = expect_hooks;
    args.debugPort = -1;
    CHECK(create(&vm, &env, &args) == JNI_OK);
    callback_env = env;
    clazz = (*env)->FindClass(env, "NativeException115"); CHECK(clazz);
    receiver = (*env)->NewGlobalRef(env, (*env)->AllocObject(env, clazz)); CHECK(receiver);
    exception_class = (*env)->FindClass(env, "java/lang/RuntimeException"); CHECK(exception_class);
    constructor = (*env)->GetMethodID(env, exception_class, "<init>", "()V"); CHECK(constructor);
    thrown = (*env)->NewGlobalRef(env, (*env)->NewObject(env, exception_class, constructor)); CHECK(thrown);
    monitor_exception_class = (*env)->FindClass(env, "java/lang/IllegalMonitorStateException"); CHECK(monitor_exception_class);
    methods[0].name = "plain"; methods[0].signature = "()V"; methods[0].fnPtr = throw_plain;
    methods[1].name = "locked"; methods[1].signature = "()V"; methods[1].fnPtr = throw_locked;
    methods[2].name = "success"; methods[2].signature = "()V"; methods[2].fnPtr = return_normally;
    CHECK((*env)->RegisterNatives(env, clazz, methods, 3) == JNI_OK);
    plain = (*env)->GetMethodID(env, clazz, "plain", "()V"); CHECK(plain);
    locked = (*env)->GetMethodID(env, clazz, "locked", "()V"); CHECK(locked);
    success = (*env)->GetMethodID(env, clazz, "success", "()V"); CHECK(success);
    if (hooks) hooks(NULL, NULL, exception_hook);
    for (i = 0; i < 10; i++) {
        callbacks = callback_error = check_monitor = 0;
        expected_pc = throw_plain;
        (*env)->CallVoidMethod(env, receiver, plain);
        pending = (*env)->ExceptionOccurred(env); CHECK(pending);
        (*env)->ExceptionClear(env);
        CHECK((*env)->IsSameObject(env, pending, thrown));
        (*env)->DeleteLocalRef(env, pending);
        CHECK(callbacks == expect_hooks && !callback_error);
        callbacks = monitor_released = 0;
        check_monitor = 1; expected_pc = throw_locked;
        (*env)->CallVoidMethod(env, receiver, locked);
        pending = (*env)->ExceptionOccurred(env); CHECK(pending);
        (*env)->ExceptionClear(env);
        CHECK((*env)->IsSameObject(env, pending, thrown));
        (*env)->DeleteLocalRef(env, pending);
        CHECK(callbacks == expect_hooks && !callback_error);
        CHECK(monitor_released == expect_hooks);
        callbacks = 0; check_monitor = 0; expected_pc = return_normally;
        (*env)->CallVoidMethod(env, receiver, success);
        CHECK(!(*env)->ExceptionOccurred(env) && callbacks == 0);
    }
    if (hooks) hooks(NULL, NULL, NULL);
    puts(expect_hooks ? "JNI exceptions: one callback per return, synchronized monitor released; 10 repeats passed"
                      : "JNI exceptions: Release propagation without debugger hooks; 10 repeats passed");
    return 0;
}
