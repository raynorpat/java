/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
#include "tree.h"
#include "javaString.h"
#include "java_io_FileOutputStream.h"
#include "java_io_FileDescriptor.h"
#include "java_io_File.h"
#include "fd_md.h"
#include <fcntl.h>
#ifdef XP_PC
#include <direct.h>
#else
#include <unistd.h>
#endif

void java_io_FileOutputStream_openAppend(Hjava_io_FileOutputStream *stream,
                                       Hjava_lang_String *name)
{
    Classjava_io_FileDescriptor *fd;
    char *path;
    if (!unhand(stream)->fd) {
        SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
        return;
    }
    if (!name) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }
    fd = unhand(unhand(stream)->fd);
    path = allocCString(name);
    if (sysOpenFD(fd, path, O_WRONLY | O_CREAT | O_APPEND, 0664) == -1)
        SignalError(0, "java/io/IOException", path);
    free(path);
}

long java_io_File_rmdir0(Hjava_io_File *file)
{
    char *path;
    int result;
    if (!unhand(file)->path) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return 0;
    }
    path = allocCString(unhand(file)->path);
    result = rmdir(path);
    free(path);
    return result != -1;
}
