/* Standalone javap has no VM console; send verifier diagnostics to stderr. */
#include <stdio.h>

void PrintToConsole(const char *bytes)
{
    fputs(bytes, stderr);
}
