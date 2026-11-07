/* Common file dialog recovered from winawt_g1000BFF0 (ANSI common-dialog ABI). */
#include "awt_Component.h"
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "sun_awt_windows_WFileDialogPeer.h"
#include "java_awt_FileDialog.h"
}
static char *fileDialogString(Hjava_lang_String *value)
{
    if (!value) return NULL;
    WCHAR *wide=awtUnicode(value);
    if (!wide) return NULL;
    int length=WideCharToMultiByte(CP_ACP,0,wide,-1,NULL,0,NULL,NULL);
    char *bytes=length ? (char *)malloc(length):NULL;
    if (bytes) WideCharToMultiByte(CP_ACP,0,wide,-1,bytes,length,NULL,NULL);
    free(wide); return bytes;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFileDialogPeer_show_stub(stack_item *sp,struct execenv *ee)
{
    Hsun_awt_windows_WFileDialogPeer *peer=(Hsun_awt_windows_WFileDialogPeer *)sp[0].p;
    if (!peer || !unhand(peer)->target) { SignalError(ee,"java/lang/NullPointerException","invalid file dialog peer"); return sp; }
    Classjava_awt_FileDialog *target=unhand((Hjava_awt_FileDialog *)unhand(peer)->target);
    char *title=fileDialogString(target->title), *directory=fileDialogString(target->dir), *initial=fileDialogString(target->file);
    char file[MAX_PATH]={0};
    /* Sun allocates MAX_PATH then copies an unbounded initial Java filename. */
    if (initial) lstrcpynA(file,initial,MAX_PATH);
    free(initial);
    DWORD capacity=GetCurrentDirectoryA(0,NULL);
    char *current=capacity ? (char *)malloc(capacity):NULL;
    if (!current || !GetCurrentDirectoryA(capacity,current)) {
        free(title); free(directory); free(current);
        SignalError(ee,"java/io/IOException","cannot save current directory"); return sp;
    }
    OPENFILENAMEA dialog={0}; dialog.lStructSize=OPENFILENAME_SIZE_VERSION_400;
    {
        AwtComponentLock lock;
        AwtComponent *parent=AwtComponent::Get(unhand(peer)->parent);
        dialog.hwndOwner=parent ? parent->GetHWnd():NULL;
    }
    dialog.lpstrFilter="All Files (*.*)\0*.*\0\0"; dialog.nFilterIndex=1;
    dialog.lpstrFile=file; dialog.nMaxFile=MAX_PATH; dialog.lpstrTitle=title;
    dialog.lpstrInitialDir=directory; dialog.Flags=0x200006;
    BOOL selected=target->mode ? GetSaveFileNameA(&dialog):GetOpenFileNameA(&dialog);
    BOOL restored=SetCurrentDirectoryA(current);
    free(title); free(directory); free(current);
    if (!restored) { SignalError(ee,"java/io/IOException","cannot restore current directory"); return sp; }
    if (selected) {
        int bytes=(int)strlen(file), length=MultiByteToWideChar(CP_ACP,0,file,bytes,NULL,0);
        HArrayOfChar *chars=(HArrayOfChar *)ArrayAlloc(T_CHAR,length);
        Hjava_lang_String *path=NULL;
        if (chars) {
            MultiByteToWideChar(CP_ACP,0,file,bytes,(WCHAR *)unhand(chars)->body,length);
            path=(Hjava_lang_String *)execute_java_constructor(ee,"java/lang/String",NULL,"([CII)",chars,0,length);
        }
        if (path) execute_java_dynamic_method(ee,(HObject *)peer,"handleSelected","(Ljava/lang/String;)V",path);
    } else execute_java_dynamic_method(ee,(HObject *)peer,"handleCancel","()V");
    return sp;
}
