/* JNI oracle harness: patch real bytecode, execute it, and restore it. */
#include "jni.h"
#include "oobj.h"
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
static FARPROC symbol(const char *name)
{
    HMODULE module = GetModuleHandleA("javai.dll");
    if (!module) module = GetModuleHandleA("javai_g.dll");
    return GetProcAddress(module, name);
}
#define SYMBOL(name) symbol(name)
#else
#include <dlfcn.h>
#define SYMBOL(name) dlsym(RTLD_DEFAULT, name)
#endif

JNIEXPORT jint JNICALL Java_Debug115_exercise(JNIEnv *env, jclass clazz, jboolean sunLayout)
{
    void (*set)(void *, unsigned char *) = (void (*)(void *, unsigned char *))SYMBOL("set_breakpoint");
    void (*clear)(void *, unsigned char *) = (void (*)(void *, unsigned char *))SYMBOL("clear_breakpoint");
    int (*opcode)(void *, unsigned char *) = (int (*)(void *, unsigned char *))SYMBOL("get_opcode");
    void *(*getee)(void) = (void *(*)(void))SYMBOL("EE");
    void (*lock)(void) = (void (*)(void))SYMBOL("lock_for_debugger");
    void (*unlock)(void) = (void (*)(void))SYMBOL("unlock_for_debugger");
    int (*locked)(void) = (int (*)(void))SYMBOL("JDB_loadclass_locked");
    jmethodID method = (*env)->GetStaticMethodID(env, clazz, "target", "()I");
    jclass illegal = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    jthrowable exception;
    unsigned char *pc, original;
    int result = 0;
    if (!set || !clear || !opcode || !getee || !method || !illegal) return 0;
    /* Sun methodblock.code is at24 (1001864C); current layout is declared. */
    pc = *(unsigned char **)((char *)method + (sunLayout ? 24 : offsetof(struct methodblock, code)));
    original = *pc;
    if (lock && unlock && locked && !locked()) {
        lock();
        if (locked()) result |= 32;
        unlock();
        if (locked()) result &= ~32;
    }
    set(getee(), pc);
    if (*pc == 202 && opcode(getee(), pc) == original) result |= 1;
    if ((*env)->CallStaticIntMethod(env, clazz, method) == 115 && !(*env)->ExceptionOccurred(env)) result |= 2;
    set(getee(), pc);
    exception = (*env)->ExceptionOccurred(env);
    (*env)->ExceptionClear(env);
    if (exception && (*env)->IsInstanceOf(env, exception, illegal)) result |= 8;
    clear(getee(), pc);
    if (*pc == original && (*env)->CallStaticIntMethod(env, clazz, method) == 115) result |= 4;
    clear(getee(), pc);
    exception = (*env)->ExceptionOccurred(env);
    (*env)->ExceptionClear(env);
    if (exception && (*env)->IsInstanceOf(env, exception, illegal)) result |= 16;
    return result;
}
