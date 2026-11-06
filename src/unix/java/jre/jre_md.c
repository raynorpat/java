/* Pthreads port of the published JDK 1.1.5 demo JRE platform interface.
 * There is no Unix registry; discover the installed bin/../lib runtime.
 * This requires real JNI invocation exports, exactly as the Windows loader.
 */
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
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
    sprintf(path, "%s/lib/libjavai.so", dir);
    return path;
}

char *JRE_GetDefaultClassPath(const char *dir)
{
    char *path = JRE_Malloc(strlen(dir) * 4 + 64);
    sprintf(path, "%s/lib/rt.jar:%s/lib/i18n.jar:%s/lib/classes.zip:%s/classes",
            dir, dir, dir, dir);
    return path;
}

jint JRE_GetDefaultSettings(JRESettings *set)
{
    char path[PATH_MAX], *tail;
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (count < 0 || count == sizeof(path) - 1) return -1;
    path[count] = 0;
    tail = strrchr(path, '/');
    if (tail == NULL) return -1;
    *tail = 0;
    tail = strrchr(path, '/');
    if (tail == NULL || strcmp(tail, "/bin")) return -1;
    *tail = 0;
    memset(set, 0, sizeof(*set));
    set->javaHome = strdup(path);
    set->runtimeLib = JRE_GetDefaultRuntimeLib(path);
    set->classPath = JRE_GetDefaultClassPath(path);
    return 0;
}

void *JRE_LoadLibrary(const char *path)
{
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

void JRE_UnloadLibrary(void *handle)
{
    dlclose(handle);
}

jint JRE_GetDefaultJavaVMInitArgs(void *handle, void *vmargs)
{
    typedef jint (JNICALL *InitArgsProc)(void *);
    InitArgsProc proc = (InitArgsProc)dlsym(handle, "JNI_GetDefaultJavaVMInitArgs");
    return proc != 0 ? ((*proc)(vmargs), 0) : -1;
}

jint JRE_CreateJavaVM(void *handle, JavaVM **vmp, JNIEnv **envp, void *vmargs)
{
    typedef jint (JNICALL *CreateVMProc)(JavaVM **, JNIEnv **, void *);
    CreateVMProc proc = (CreateVMProc)dlsym(handle, "JNI_CreateJavaVM");
    return proc != 0 ? (*proc)(vmp, envp, vmargs) : -1;
}
