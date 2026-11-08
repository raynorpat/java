/* Pthreads port of the published JDK 1.1.5 demo JRE platform interface.
 * There is no Unix registry; discover the installed bin/../lib runtime.
 * This requires real JNI invocation exports, exactly as the Windows loader.
 */
#include <string.h>
#include <unistd.h>
#ifdef JDK115_CTHREADS
#include <mach-o/dyld.h>
#include <crt_externs.h>
#define VM_SYMBOL(name) (NSIsSymbolNameDefined("_" name) ? NSAddressOfSymbol(NSLookupAndBindSymbol("_" name)) : NULL)
#else
#include <dlfcn.h>
#define VM_SYMBOL(name) dlsym(handle, name)
#endif
#include <limits.h>
#include "jre.h"

jint JRE_GetCurrentSettings(JRESettings *set)
{
    return -1;
}

jint JRE_GetSettings(JRESettings *set, const char *version)
{
    return -1;
}

char *JRE_GetDefaultRuntimeLib(const char *dir)
{
    char *path = JRE_Malloc(strlen(dir) + 32);
#ifdef JDK115_CTHREADS
    sprintf(path, "%s/lib/libjavai.dylib", dir);
#else
    sprintf(path, "%s/lib/libjavai.so", dir);
#endif
    return path;
}

char *JRE_GetDefaultClassPath(const char *dir)
{
    char *path = JRE_Malloc(strlen(dir) * 5 + 96);
    sprintf(path, "%s/lib/rt.jar:%s/lib/i18n.jar:%s/lib/classes.zip:%s/classes",
            dir, dir, dir, dir);
#ifdef JDK115_CTHREADS
    strcat(path, ":"); strcat(path, dir);
    strcat(path, "/lib/rhapsody-peers.zip");
#endif
    return path;
}

jint JRE_GetDefaultSettings(JRESettings *set)
{
    char path[PATH_MAX], *tail;
#ifdef JDK115_CTHREADS
    if (!realpath((*_NSGetArgv())[0], path)) return -1;
#else
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (count < 0 || count == sizeof(path) - 1) return -1;
    path[count] = 0;
#endif
    tail = strrchr(path, '/');
    if (tail == NULL) return -1;
    *tail = 0;
    tail = strrchr(path, '/');
    if (tail == NULL) return -1;
#ifdef JDK115_CTHREADS
    if (strcmp(tail, "/bin") && strcmp(tail, "/libexec")) return -1;
#else
    if (strcmp(tail, "/bin")) return -1;
#endif
    *tail = 0;
    memset(set, 0, sizeof(*set));
    set->javaHome = strdup(path);
    set->runtimeLib = JRE_GetDefaultRuntimeLib(path);
    set->classPath = JRE_GetDefaultClassPath(path);
    return 0;
}

void *JRE_LoadLibrary(const char *path)
{
#ifdef JDK115_CTHREADS
    return NSAddLibrary(path) ? (void *)1 : NULL;
#else
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif
}

void JRE_UnloadLibrary(void *handle)
{
#ifdef JDK115_CTHREADS
    /* DR2 NSAddLibrary images remain loaded for the process lifetime. */
    (void)handle;
#else
    dlclose(handle);
#endif
}

jint JRE_GetDefaultJavaVMInitArgs(void *handle, void *vmargs)
{
    typedef jint (JNICALL *InitArgsProc)(void *);
    InitArgsProc proc = (InitArgsProc)VM_SYMBOL("JNI_GetDefaultJavaVMInitArgs");
    return proc != 0 ? ((*proc)(vmargs), 0) : -1;
}

jint JRE_CreateJavaVM(void *handle, JavaVM **vmp, JNIEnv **envp, void *vmargs)
{
    typedef jint (JNICALL *CreateVMProc)(JavaVM **, JNIEnv **, void *);
    CreateVMProc proc = (CreateVMProc)VM_SYMBOL("JNI_CreateJavaVM");
    return proc != 0 ? (*proc)(vmp, envp, vmargs) : -1;
}
