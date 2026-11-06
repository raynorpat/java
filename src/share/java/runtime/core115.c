/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Reconstructed from javai_g.dll; see core115-evidence.md. */
#include <string.h>
#include <errno.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "java_lang_SecurityManager.h"
#include "java_io_FileDescriptor.h"
#ifdef XP_PC
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

HArrayOfObject *java_lang_Class_getSigners(ClassClass *self)
{
    HArrayOfObject *source = (HArrayOfObject *)unhand(self)->signers, *copy;
    int length;
    if (!source) return NULL;
    length = obj_length(source);
    copy = (HArrayOfObject *)ArrayAlloc(T_CLASS, length);
    if (!copy) { SignalError(0, JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    /* Include the component-class pointer after the element slots. */
    memcpy(unhand(copy)->body, unhand(source)->body, (length + 1) * sizeof(HObject *));
    return copy;
}
void java_lang_Class_setSigners(ClassClass *self, HArrayOfObject *signers)
{ unhand(self)->signers = (struct HArrayOfObject *)signers; }

ClassClass *java_lang_SecurityManager_currentLoadedClass0(Hjava_lang_SecurityManager *self)
{
    JavaFrame *frame, buffer;
    ClassClass *cb;
    if (!java_lang_SecurityManager_checkInitialized(self)) return NULL;
    for (frame = EE()->current_frame; frame;) {
        if (frame->current_method) {
            cb = frame->current_method->fb.clazz;
            if (cb && cbLoader(cb)) return cbHandle(cb);
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED
                ? CompiledFramePrev(frame, &buffer) : frame->prev;
        } else frame = frame->prev;
    }
    return NULL;
}

void java_io_FileDescriptor_sync(Hjava_io_FileDescriptor *self)
{
#ifdef XP_PC
    HANDLE handle;
    DWORD error;
    char message[256];
#endif
    if (!self || !unhand(self)) {
        SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
        return;
    }
#ifdef XP_PC
    /* Sun also stores CRT descriptor plus one, then converts to HANDLE. */
    handle = (HANDLE)_get_osfhandle(unhand(self)->fd - 1);
    if (!FlushFileBuffers(handle)) {
        error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) return;
        message[0] = 0;
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0,
                       message, sizeof(message), NULL);
        SignalError(0, "java/io/SyncFailedException", message);
    }
#else
    /* Platform mapping: fsync is the POSIX descriptor flush operation. */
    if (fsync(unhand(self)->fd - 1) != 0)
        SignalError(0, "java/io/SyncFailedException", strerror(errno));
#endif
}
