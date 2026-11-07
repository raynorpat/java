/* Clipboard oracle fixture: intercept only this process's winawt imports.
 * The interactive desktop clipboard is never opened by this test. */
#include <windows.h>
#include <stdlib.h>
#include "StubPreamble.h"
#include "typecodes.h"
static HWND owner,clipboardOwner,openedWindow;
static HGLOBAL clipboardData;
static DWORD openThread;
static BOOL installed;
static BOOL WINAPI clipboardOpen(HWND window)
{
    DWORD thread=GetCurrentThreadId();
    if (InterlockedCompareExchange((LONG volatile *)&openThread,thread,0)!=0) return FALSE;
    openedWindow=window; return TRUE;
}
static BOOL WINAPI clipboardClose(void) { openedWindow=NULL; InterlockedExchange((LONG volatile *)&openThread,0); return TRUE; }
static HWND WINAPI clipboardGetOwner(void) { return clipboardOwner; }
static BOOL WINAPI clipboardEmpty(void)
{
    HWND previous=clipboardOwner;
    if (clipboardData) GlobalFree(clipboardData); clipboardData=NULL;
    clipboardOwner=openedWindow;
    if (previous) SendMessage(previous,WM_DESTROYCLIPBOARD,0,0);
    return TRUE;
}
static HANDLE WINAPI clipboardSet(UINT format,HANDLE memory)
{
    if (format!=CF_UNICODETEXT || !openThread) return NULL;
    if (clipboardData) GlobalFree(clipboardData);
    clipboardData=memory; return memory;
}
static HANDLE WINAPI clipboardGet(UINT format) { return format==CF_UNICODETEXT ? clipboardData:NULL; }
static int patch(HMODULE module,const char *name,void *function)
{
    BYTE *base=(BYTE *)module;
    IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imports=(IMAGE_IMPORT_DESCRIPTOR *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for (;imports->Name;imports++) {
        IMAGE_THUNK_DATA *names,*slots;
        if (!imports->OriginalFirstThunk) continue;
        names=(IMAGE_THUNK_DATA *)(base+imports->OriginalFirstThunk); slots=(IMAGE_THUNK_DATA *)(base+imports->FirstThunk);
        for (;names->u1.AddressOfData;names++,slots++) if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) {
            IMAGE_IMPORT_BY_NAME *import=(IMAGE_IMPORT_BY_NAME *)(base+names->u1.AddressOfData);
            if (!lstrcmpA((char *)import->Name,name)) {
                DWORD old;
                if (!VirtualProtect(&slots->u1.Function,sizeof(DWORD),PAGE_READWRITE,&old)) return 0;
                InterlockedExchange((LONG volatile *)&slots->u1.Function,(LONG)function);
                VirtualProtect(&slots->u1.Function,sizeof(DWORD),old,&old); return 1;
            }
        }
    }
    return 0;
}
__declspec(dllexport) stack_item *Java_Clipboard115_isolate_stub(stack_item *sp,struct execenv *ee)
{
    HMODULE module=GetModuleHandleA("winawt.dll"); int count=0;
    if (!module) module=GetModuleHandleA("winawt_g.dll");
    if (module) {
        count+=patch(module,"OpenClipboard",clipboardOpen); count+=patch(module,"CloseClipboard",clipboardClose);
        count+=patch(module,"GetClipboardOwner",clipboardGetOwner); count+=patch(module,"EmptyClipboard",clipboardEmpty);
        count+=patch(module,"SetClipboardData",clipboardSet); count+=patch(module,"GetClipboardData",clipboardGet);
    }
    owner=CreateWindowExA(0,"STATIC","Clipboard115 owner",WS_POPUP,0,0,0,0,NULL,NULL,GetModuleHandle(NULL),NULL);
    installed=count==6 && owner!=NULL;
    if (!installed) SignalError(ee,"java/lang/RuntimeException","clipboard API fixture incomplete");
    return sp;
}
__declspec(dllexport) stack_item *Java_Clipboard115_verifyIsolation_stub(stack_item *sp,struct execenv *ee)
{
    if (!installed) SignalError(ee,"java/lang/RuntimeException","clipboard fixture not installed");
    return sp;
}
__declspec(dllexport) stack_item *Java_Clipboard115_external_stub(stack_item *sp,struct execenv *ee)
{
    struct Hjava_lang_String *value=(struct Hjava_lang_String *)sp[0].p;
    int length=javaStringLength(value); HGLOBAL memory; WCHAR *text;
    memory=GlobalAlloc(GMEM_MOVEABLE,(length+1)*sizeof(WCHAR));
    text=(WCHAR *)GlobalLock(memory); javaString2unicode(value,(unicode *)text,length); text[length]=0; GlobalUnlock(memory);
    if (!clipboardOpen(owner)) { GlobalFree(memory); SignalError(ee,"java/lang/RuntimeException","isolated clipboard busy"); return sp; }
    clipboardEmpty(); clipboardSet(CF_UNICODETEXT,memory); clipboardClose(); return sp;
}
__declspec(dllexport) stack_item *Java_Clipboard115_raw_stub(stack_item *sp,struct execenv *ee)
{
    HGLOBAL memory; WCHAR *text,*copy=NULL; int length=0,capacity; HArrayOfChar *chars;
    if (!clipboardOpen(NULL)) { SignalError(ee,"java/lang/RuntimeException","isolated clipboard busy"); return sp+1; }
    memory=clipboardGet(CF_UNICODETEXT);
    if (memory) {
        capacity=(int)(GlobalSize(memory)/sizeof(WCHAR)); text=(WCHAR *)GlobalLock(memory);
        if (text) {
            while (length<capacity && text[length]) ++length;
            copy=(WCHAR *)malloc((length+1)*sizeof(WCHAR));
            if (copy) CopyMemory(copy,text,length*sizeof(WCHAR));
            GlobalUnlock(memory);
        }
    }
    clipboardClose(); sp[0].p=NULL;
    if (copy) {
        chars=(HArrayOfChar *)ArrayAlloc(T_CHAR,length);
        if (chars) { CopyMemory(unhand(chars)->body,copy,length*sizeof(WCHAR)); sp[0].p=execute_java_constructor(ee,"java/lang/String",NULL,"([CII)",chars,0,length); }
        free(copy);
    }
    return sp+1;
}
