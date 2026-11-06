/*
 * Hooks the VM expects from its host program. In javai.dll the class
 * loader cannot reach the ones in java.exe, so the DLL supplies its own.
 */
#include <oobj.h>
#include <tree.h>
#include <interpreter.h>
#include <io_md.h>
#include <sys/stat.h>

void ClassDefHook(ClassClass *cb) { }

int OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long fd = -1;

    if ((st != 0) && (fn != 0) &&
        ((fd = sysOpen(fn, O_RDONLY, 0400)) >= 0) &&
        (fstat(fd, st) < 0)) {
        sysClose(fd);
        fd = -1;
    }
    return fd < 0 ? -2 : fd;
}

/* Used by the Win32 AWT (awt/windows/awtapp.cpp). */
extern ClassClass *Thread_classblock;
ClassClass *GetThreadClassBlock(void)
{
    return Thread_classblock;
}
