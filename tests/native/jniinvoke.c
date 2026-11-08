#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#elif defined(JDK115_CTHREADS)
#include <mach-o/dyld.h>
#include <mach/cthreads.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#endif
static JavaVM *vm;
static int attachedResult;
static int exited, printed, aborted;
static void JNICALL exitHook(jint code) { exited = code; }
static void JNICALL abortHook(void) { aborted++; }
static jint JNICALL printHook(FILE *stream, const char *format, va_list args)
{ printed++; return 0; }
static void attached(void)
{
    JNIEnv *env, *again;
    jclass cls;
    jmethodID method;
    jobject current;
    jstring name;
    const char *chars;
    if ((*vm)->AttachCurrentThread(vm, &env, NULL)) return;
    if ((*vm)->AttachCurrentThread(vm, &again, NULL) || again != env) return;
    cls = (*env)->FindClass(env, "java/lang/Thread");
    method = (*env)->GetStaticMethodID(env, cls, "currentThread", "()Ljava/lang/Thread;");
    current = (*env)->CallStaticObjectMethod(env, cls, method);
    method = (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");
    name = (jstring)(*env)->CallObjectMethod(env, current, method);
    chars = (*env)->GetStringUTFChars(env, name, NULL);
    if (chars[0] != 'T' || chars[6] != '-') return;
    (*env)->ReleaseStringUTFChars(env, name, chars);
    cls = (*env)->FindClass(env, "java/lang/Integer");
    method = (*env)->GetStaticMethodID(env, cls, "parseInt", "(Ljava/lang/String;)I");
    if ((*env)->CallStaticIntMethod(env, cls, method, (*env)->NewStringUTF(env, "37")) != 37) return;
    if ((*vm)->DetachCurrentThread(vm)) return;
    attachedResult = 1;
}
#ifdef _WIN32
static DWORD WINAPI hostThread(void *unused) { attached(); return 0; }
#else
static void *hostThread(void *unused) { attached(); return NULL; }
#endif
int main(int argc, char **argv)
{
    jint (JNICALL *defaults)(void *);
    jint (JNICALL *create)(JavaVM **, JNIEnv **, void *);
    jint (JNICALL *list)(JavaVM **, jsize, jsize *);
    void (*abortVM)(void);
    JDK1_1InitArgs args;
    JNIEnv *env;
    JavaVM *found;
    jsize count;
    jclass cls;
    jmethodID method;
    jstring string;
    char *properties[] = { "jni.invocation=works", NULL };
    const char *text;
#ifdef _WIN32
    HMODULE library;
    HANDLE thread;
#elif defined(JDK115_CTHREADS)
    int library;
    cthread_t thread;
#else
    void *library;
    pthread_t thread;
#endif
    if (argc != 3) return 2;
#ifdef _WIN32
    library = LoadLibraryA(argv[1]);
#define SYMBOL(name) GetProcAddress(library, name)
#elif defined(JDK115_CTHREADS)
    library = NSAddLibrary(argv[1]);
#define SYMBOL(name) NSAddressOfSymbol(NSLookupAndBindSymbol("_" name))
#else
    library = dlopen(argv[1], RTLD_NOW | RTLD_GLOBAL);
#define SYMBOL(name) dlsym(library, name)
#endif
    if (!library) return 3;
    defaults = (jint (JNICALL *)(void *))SYMBOL("JNI_GetDefaultJavaVMInitArgs");
    create = (jint (JNICALL *)(JavaVM **, JNIEnv **, void *))SYMBOL("JNI_CreateJavaVM");
    list = (jint (JNICALL *)(JavaVM **, jsize, jsize *))SYMBOL("JNI_GetCreatedJavaVMs");
    abortVM = (void (*)(void))SYMBOL("sysAbort");
    if (!defaults || !create || !list) return 4;
    list(&found, 1, &count);
    printf("before=%ld\n", (long)count);
    putenv("CLASSPATH=jni-default-must-not-use-this");
    if (defaults(&args)) return 5;
    if (args.nativeStackSize != 128 * 1024 || args.javaStackSize != 400 * 1024 ||
        args.minHeapSize != 1024 * 1024 || args.maxHeapSize != 16 * 1024 * 1024 ||
        strstr(args.classpath, "jni-default-must-not-use-this") ||
        !strstr(args.classpath, "classes.zip") || !strstr(args.classpath, "classes.jar") ||
        !strstr(args.classpath, "rt.jar") || !strstr(args.classpath, "i18n.jar")) return 9;
    printf("version=%lx\n", (long)args.version);
    args.classpath = argv[2];
    args.properties = properties;
    args.exit = exitHook;
    args.abort = abortHook;
    args.vfprintf = printHook;
    if (create(&vm, &env, &args)) return 6;
    /* Original JavaVM_ holds the main EE, whose JNI interface is at +20. */
    if (!((struct JavaVM_ *)vm)->reserved0 ||
        ((char *)env - (char *)((struct JavaVM_ *)vm)->reserved0) != 20 ||
        ((struct JavaVM_ *)vm)->reserved1 || ((struct JavaVM_ *)vm)->reserved2)
        return 10;
    list(&found, 1, &count);
    printf("created=%ld/%d\n", (long)count, found == vm);
    cls = (*env)->FindClass(env, "java/lang/System");
    method = (*env)->GetStaticMethodID(env, cls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
    string = (jstring)(*env)->CallStaticObjectMethod(env, cls, method, (*env)->NewStringUTF(env, "jni.invocation"));
    text = (*env)->GetStringUTFChars(env, string, NULL);
    printf("property=%s\n", text);
    (*env)->ReleaseStringUTFChars(env, string, text);
    fflush(stdout);
    cls = (*env)->FindClass(env, "Hello115");
    if (!cls) return 7;
    method = (*env)->GetStaticMethodID(env, cls, "main", "([Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, cls, method,
        (*env)->NewObjectArray(env, 0, (*env)->FindClass(env, "java/lang/String"), NULL));
    if ((*env)->ExceptionOccurred(env)) return 8;
#ifdef _WIN32
    thread = CreateThread(NULL, 0, hostThread, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#elif defined(JDK115_CTHREADS)
    thread = cthread_fork(hostThread, NULL);
    cthread_join(thread);
#else
    pthread_create(&thread, NULL, hostThread, NULL);
    pthread_join(thread, NULL);
#endif
    printf("attached=%d\n", attachedResult);
    (*env)->FatalError(env, "invocation hook test");
    if (abortVM) abortVM();
    printf("hooks=%d/%d/%d\n", exited, printed != 0, aborted);
    printf("destroy=%ld\n", (long)(*vm)->DestroyJavaVM(vm));
    return 0;
}
