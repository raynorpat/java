/* Windowed entry for the existing VM bootstrap; no console is allocated. */
#include <windows.h>
#include <stdlib.h>
#include "sys_api.h"

extern JAVA_PUBLIC_API(int) java_main(int, char **);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR line, int show)
{
    char **args = calloc(__argc + 1, sizeof(char *));
    int i, result;
    if (args == NULL) return 1;
    args[0] = "java";
    for (i = 1; i < __argc; ++i) args[i] = __argv[i];
    result = java_main(__argc, args);
    free(args);
    return result;
}
