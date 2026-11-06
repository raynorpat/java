/* Invocation API reconstructed from javai_g.dll 10026fcd--100278ed. */
#include <stdlib.h>
#include <string.h>
#include "jni_impl.h"
#include "javaThreads.h"
#include "javaString.h"
#include "monitor.h"
#include "path_md.h"
#undef fprintf
#undef vfprintf
#undef printf

extern int java_initialize_platform(void);
extern int java_initialize_runtime(ExecEnv *, int, int);
extern void add_user_prop(char *);
extern int noasyncgc, tracegc, verbose;
extern int jni_AttachHostThread(sys_thread_t **);
extern void jni_DetachHostThread(void);
extern HThreadGroup *maingroup;
extern ClassClass *Thread_classblock;
extern void WaitToDie(void);
extern sys_mon_t *_heap_lock;
static int creatingVM;
static void (JNICALL *exitHook)(jint);
static void (JNICALL *abortHook)();
extern int (JNICALL *jio_vfprintf_hook)(FILE *, const char *, va_list);
JavaVM jni_vm = &jni_InvokeInterface;

int jni_InvokeExitHook(int code)
{ if (!exitHook) return 0; exitHook(code); return 1; }
int jni_InvokeAbortHook(void)
{ if (!abortHook) return 0; abortHook(); return 1; }

jint JNICALL JNI_GetDefaultJavaVMInitArgs(void *opaque)
{
    JDK1_1InitArgs *args = (JDK1_1InitArgs *)opaque;
    static char *defaultClasspath;
    memset(args, 0, sizeof(*args));
    args->version = 0x10001;
    args->nativeStackSize = 128 * 1024;
    args->javaStackSize = 400 * 1024;
    args->minHeapSize = 1024 * 1024;
    args->maxHeapSize = 16 * 1024 * 1024;
    args->verifyMode = VERIFY_REMOTE;
    args->enableClassGC = 1;
    if (!defaultClasspath) {
        const char *home = sysJavaHome();
        defaultClasspath = malloc(5 * strlen(home) + 100);
        if (!defaultClasspath) return -1;
        /* Primary 10002527: boot archives only; java_main prepends CLASSPATH.
         * deviation: use normalized home and the host's path separators. */
        sprintf(defaultClasspath, "%s%cclasses%c%s%clib%cclasses.zip%c%s%clib%cclasses.jar%c%s%clib%crt.jar%c%s%clib%ci18n.jar",
                home, LOCAL_DIR_SEPARATOR, PATH_SEPARATOR,
                home, LOCAL_DIR_SEPARATOR, LOCAL_DIR_SEPARATOR, PATH_SEPARATOR,
                home, LOCAL_DIR_SEPARATOR, LOCAL_DIR_SEPARATOR, PATH_SEPARATOR,
                home, LOCAL_DIR_SEPARATOR, LOCAL_DIR_SEPARATOR, PATH_SEPARATOR,
                home, LOCAL_DIR_SEPARATOR, LOCAL_DIR_SEPARATOR);
    }
    args->classpath = defaultClasspath;
    return 0;
}

jint JNICALL JNI_CreateJavaVM(JavaVM **vm, JNIEnv **env, void *opaque)
{
    JDK1_1InitArgs *args = (JDK1_1InitArgs *)opaque;
    ExecEnv *ee;
    int i;
    /* JDK 1.1 permits only one VM per process. */
    if (DefaultExecEnv || creatingVM) return -1;
    creatingVM = 1;
    exitHook = args->exit;
    abortHook = args->abort;
    jio_vfprintf_hook = args->vfprintf;
    ee = (ExecEnv *)calloc(1, sizeof(*ee));
    if (!ee) { creatingVM = 0; return -1; }
    mainstktop = (stackp_t)&opaque;
    SkipSourceChecks = !args->checkSource;
    if (args->nativeStackSize >= 1000) ProcStackSize = args->nativeStackSize;
    if (args->javaStackSize >= 1000) JavaStackSize = args->javaStackSize;
    if ((unsigned int)args->verifyMode <= 2) verifyclasses = args->verifyMode;
    verbose = args->verbose;
    noasyncgc = args->disableAsyncGC;
    tracegc = args->enableVerboseGC;
    if (args->classpath) setCLASSPATH(strdup(args->classpath));
    if (args->properties) for (i = 0; args->properties[i]; i++) add_user_prop(args->properties[i]);
    if (java_initialize_platform() || java_initialize_runtime(ee, args->minHeapSize, args->maxHeapSize)) {
        *env = NULL; *vm = NULL;
        return -1;
    }
    if (args->debugging) {
        ClassClass *agent = FindClass(ee, "sun/tools/debug/Agent", TRUE);
        if (!agent) return -1;
        execute_java_static_method(ee, agent, "boot", "(I)V", args->debugPort);
        if (exceptionOccurred(ee)) return -1;
    }
    InitializeMainThread();
    if (exceptionOccurred(ee)) return -1;
    *env = jni_Env(ee);
    *vm = &jni_vm;
    creatingVM = 0;
    return *env ? 0 : -1;
}

jint JNICALL JNI_GetCreatedJavaVMs(JavaVM **buffer, jsize length, jsize *count)
{
    if (count) *count = DefaultExecEnv ? 1 : 0;
    if (length > 0 && DefaultExecEnv) buffer[0] = &jni_vm;
    return 0;
}

static jint JNICALL jni_AttachCurrentThread(JavaVM *vm, JNIEnv **env, void *args)
{
    sys_thread_t *thread = sysThreadSelf();
    ExecEnv *ee;
    HThread *object;
    struct methodblock *constructor = NULL;
    int i;
    if (!DefaultExecEnv) return -1;
    if (thread && sysThreadGetBackPtr(thread)) {
        *env = jni_Env(EE());
        return *env ? 0 : -1;
    }
    ee = (ExecEnv *)calloc(1, sizeof(*ee));
    if (!ee) return -1;
    /* Register before allocating Java objects so GC can scan this host stack. */
    sysMonitorEnter(_heap_lock);
    if (jni_AttachHostThread(&thread) != SYS_OK) {
        sysMonitorExit(_heap_lock); free(ee); return -1;
    }
    object = (HThread *)ObjAlloc(Thread_classblock, 0);
    if (object) {
        InitializeExecEnv(ee, (JHandle *)object);
        THREAD(object)->PrivateInfo = (long)thread;
        THREAD(object)->priority = 5;
        THREAD(object)->group = maingroup;
        sysThreadSetBackPtr(thread, object);
    }
    sysMonitorExit(_heap_lock);
    if (!object || !ee->initial_stack) {
        if (object) DeleteExecEnv(ee, (JHandle *)object);
        else free(ee);
        jni_DetachHostThread(); return -1;
    }
    for (i = 0; i < Thread_classblock->methods_count; i++) {
        struct methodblock *method = &cbMethods(Thread_classblock)[i];
        if (!strcmp(fieldname(&method->fb), "<init>") &&
            !strcmp(fieldsig(&method->fb), "(Ljava/lang/ThreadGroup;Ljava/lang/Runnable;)V")) {
            constructor = method;
            break;
        }
    }
    if (constructor)
        do_execute_java_method(ee, object, NULL, NULL, constructor, FALSE, maingroup, NULL);
    else
        SignalError(ee, JAVAPKG "NoSuchMethodError", "Thread.<init>");
    if (exceptionOccurred(ee)) {
        DeleteExecEnv(ee, (JHandle *)object); jni_DetachHostThread(); return -1;
    }
    *env = jni_Env(ee);
    return *env ? 0 : -1;
}

static jint JNICALL jni_DetachCurrentThread(JavaVM *vm)
{
    ExecEnv *ee;
    HThread *object;
    int attempts;
    if (!sysThreadSelf() || !sysThreadGetBackPtr(sysThreadSelf())) return -1;
    ee = EE();
    if (ee == DefaultExecEnv || ee->current_frame) return -1;
    object = (HThread *)ee->thread;
    for (attempts = 0; attempts < 3 && unhand(object)->group; attempts++) {
        exceptionClear(ee);
        execute_java_dynamic_method(ee, (HObject *)object, "exit", "()V");
    }
    monitorEnter(obj_monitor(object));
    THREAD(object)->PrivateInfo = 0;
    monitorNotifyAll(obj_monitor(object));
    monitorExit(obj_monitor(object));
    DeleteExecEnv(ee, (JHandle *)object);
    jni_DetachHostThread();
    return 0;
}
static jint JNICALL jni_DestroyJavaVM(JavaVM *vm)
{
    WaitToDie();
    /* Primary Win32 implementation waits, then reports unsupported teardown. */
    return -1;
}
const struct JNIInvokeInterface_ jni_InvokeInterface = {
    NULL, NULL, NULL, jni_DestroyJavaVM, jni_AttachCurrentThread, jni_DetachCurrentThread
};
