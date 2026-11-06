/* Sun Debug private sysAtexit/sysExit oracle and standalone port regression. */
#include <stdio.h>
#ifdef SUN_ORACLE
#include <windows.h>
#endif

static void first(void) { puts("callback=1"); }
static void second(void) { puts("callback=2"); }
#ifdef _WIN32
static void __stdcall exit_hook(int status) { printf("exit=%d\n", status); }
#else
static void exit_hook(int status) { printf("exit=%d\n", status); }
#endif

#ifndef SUN_ORACLE
extern int sysAtexit(void (*callback)(void));
extern void sysExit(int status);
int jni_InvokeExitHook(int status) { exit_hook(status); return 1; }
int jni_InvokeAbortHook(void) { return 0; }
#endif

int main(int argc, char **argv)
{
    int (*register_exit)(void (*)(void));
    void (*perform_exit)(int);
#ifdef SUN_ORACLE
    HMODULE module;
    if (argc != 2) return 2;
    module = LoadLibraryA(argv[1]);
    if (!module) return 3;
    register_exit = (int (*)(void (*)(void)))((char *)module + 0x4bffa);
    perform_exit = (void (*)(int))((char *)module + 0x4bf67);
    *(void **)((char *)module + 0x71868) = exit_hook;
#else
    (void)argc; (void)argv;
    register_exit = sysAtexit;
    perform_exit = sysExit;
#endif
    if (register_exit(first) || register_exit(second)) return 4;
    perform_exit(7);
    puts("returned");
    return 0;
}
