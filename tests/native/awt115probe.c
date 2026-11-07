/* Drive only windows owned by this test process; usable with either VM. */
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <dlgs.h>
#include "StubPreamble.h"
struct window_search { WCHAR *title; HWND window; };
static BOOL CALLBACK find_file_edit(HWND window,LPARAM parameter)
{
    WCHAR name[32]; GetClassNameW(window,name,32);
    if (GetDlgCtrlID(window)==1001 && !lstrcmpiW(name,L"EDIT") && IsWindowVisible(window)) {
        *(HWND *)parameter=window; return FALSE;
    }
    return TRUE;
}
static HWND file_edit(HWND window)
{
    HWND combo,edit;
    if (!window) return NULL;
    combo=GetDlgItem(window,cmb13);
    edit=combo ? (HWND)SendMessage(combo,CBEM_GETEDITCONTROL,0,0):GetDlgItem(window,edt1);
    /* Modern Save dialogs nest their filename editor below DirectUI. */
    if (!edit) EnumChildWindows(window,find_file_edit,(LPARAM)&edit);
    return edit;
}
static BOOL CALLBACK find_choice(HWND window, LPARAM parameter)
{
    WCHAR name[32];
    GetClassNameW(window,name,32);
    if (!lstrcmpiW(name,L"COMBOBOX")) { *(HWND *)parameter = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK find_list(HWND window, LPARAM parameter)
{
    WCHAR name[32]; GetClassNameW(window,name,32);
    if (!lstrcmpiW(name,L"LISTBOX")) { *(HWND *)parameter = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK find_scrollbar(HWND window, LPARAM parameter)
{
    WCHAR name[32]; GetClassNameW(window,name,32);
    if (!lstrcmpiW(name,L"SCROLLBAR")) { *(HWND *)parameter = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK find_edit(HWND window, LPARAM parameter)
{
    WCHAR name[32]; GetClassNameW(window,name,32);
    if (!lstrcmpiW(name,L"EDIT")) { *(HWND *)parameter = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK find_canvas(HWND window, LPARAM parameter)
{
    WCHAR name[64]; GetClassNameW(window,name,64);
    if (!lstrcmpiW(name,L"SunAwtCanvas") || !lstrcmpiW(name,L"AwtCanvas")) {
        *(HWND *)parameter=window; return FALSE;
    }
    return TRUE;
}
static BOOL CALLBACK find_pane(HWND window, LPARAM parameter)
{
    WCHAR name[64]; GetClassNameW(window,name,64);
    if (!lstrcmpiW(name,L"SunAwtScrollPane") || !lstrcmpiW(name,L"AwtScrollPane")) {
        *(HWND *)parameter = window; return FALSE;
    }
    return TRUE;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_choice_stub(stack_item *sp,struct execenv *ee);
static BOOL CALLBACK find_window(HWND window, LPARAM parameter)
{
    struct window_search *search = (struct window_search *)parameter;
    WCHAR title[256]; DWORD process;
    GetWindowThreadProcessId(window,&process);
    if (process != GetCurrentProcessId()) return TRUE;
    GetWindowTextW(window,title,256);
    if (!lstrcmpW(title,search->title)) { search->window = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK find_visible_window(HWND window, LPARAM parameter)
{ return IsWindowVisible(window) ? find_window(window,parameter):TRUE; }
static HWND test_window_matching(struct Hjava_lang_String *title,BOOL visible)
{
    WCHAR text[256]; struct window_search search; int length;
    if (!title) return NULL;
    length = javaStringLength(title);
    if (length > 255) return NULL;
    javaString2unicode(title,(unicode *)text,length); text[length] = 0;
    search.title = text; search.window = NULL;
    EnumWindows(visible ? find_visible_window:find_window,(LPARAM)&search);
    return search.window;
}
static HWND test_window(struct Hjava_lang_String *title)
{ return test_window_matching(title,FALSE); }
__declspec(dllexport) stack_item *Java_WinAwtProbe_choice_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame = test_window((struct Hjava_lang_String *)sp[0].p), choice = NULL;
    int index = sp[1].i;
    if (frame) EnumChildWindows(frame,find_choice,(LPARAM)&choice);
    if (!choice) { SignalError(ee,"java/lang/RuntimeException","choice not found"); return sp; }
    if (index >= 0) {
        SendMessage(choice,CB_SETCURSEL,index,0);
        PostMessage(GetParent(choice),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(choice),CBN_SELCHANGE),(LPARAM)choice);
    }
    sp[0].i = (long)SendMessage(choice,CB_GETCURSEL,0,0); return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_listEvent_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame = test_window((struct Hjava_lang_String *)sp[0].p), list = NULL;
    if (frame) EnumChildWindows(frame,find_list,(LPARAM)&list);
    if (!list) { SignalError(ee,"java/lang/RuntimeException","list not found"); return sp; }
    if (GetWindowLong(list,GWL_STYLE)&LBS_MULTIPLESEL) {
        SendMessage(list,LB_SETSEL,TRUE,sp[1].i); SendMessage(list,LB_SETCARETINDEX,sp[1].i,FALSE);
    } else SendMessage(list,LB_SETCURSEL,sp[1].i,0);
    PostMessage(GetParent(list),WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(list),sp[2].i ? LBN_DBLCLK : LBN_SELCHANGE),(LPARAM)list);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_scroll_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame = test_window((struct Hjava_lang_String *)sp[0].p), scroll = NULL;
    if (frame) EnumChildWindows(frame,find_scrollbar,(LPARAM)&scroll);
    if (!scroll) { SignalError(ee,"java/lang/RuntimeException","scrollbar not found"); return sp; }
    PostMessage(GetParent(scroll),(GetWindowLong(scroll,GWL_STYLE)&SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
        MAKEWPARAM(sp[1].i,sp[2].i),(LPARAM)scroll);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_edit_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame = test_window((struct Hjava_lang_String *)sp[0].p), edit = NULL;
    struct Hjava_lang_String *value = (struct Hjava_lang_String *)sp[1].p;
    WCHAR text[256]; int length;
    if (frame) EnumChildWindows(frame,find_edit,(LPARAM)&edit);
    if (!edit || !value || (length=javaStringLength(value))>255) {
        SignalError(ee,"java/lang/RuntimeException","invalid edit request"); return sp;
    }
    javaString2unicode(value,(unicode *)text,length); text[length]=0;
    SetWindowTextW(edit,text);
    if (sp[2].i) PostMessage(edit,WM_CHAR,13,1);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_paneScroll_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame = test_window((struct Hjava_lang_String *)sp[0].p), pane = NULL;
    if (frame) EnumChildWindows(frame,find_pane,(LPARAM)&pane);
    if (!pane) { SignalError(ee,"java/lang/RuntimeException","scroll pane not found"); return sp; }
    PostMessage(pane,sp[1].i ? WM_VSCROLL:WM_HSCROLL,MAKEWPARAM(sp[2].i,sp[3].i),0);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_surfacePixel_stub(stack_item *sp,struct execenv *ee)
{
    HDC dc=(HDC)sp[0].i;
    if (sp[1].i>=0) SetPixel(dc,1,1,(COLORREF)sp[1].i);
    GdiFlush(); sp[0].i=(long)GetPixel(dc,1,1); return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_heapBytes_stub(stack_item *sp,struct execenv *ee)
{
    PROCESS_HEAP_ENTRY entry; HANDLE heaps[64]; DWORD bytes=0,count,index;
    count=GetProcessHeaps(64,heaps);
    if (count>64) { SignalError(ee,"java/lang/RuntimeException","too many native heaps"); return sp; }
    for (index=0;index<count;index++) {
        ZeroMemory(&entry,sizeof(entry));
        if (!HeapLock(heaps[index])) continue;
        while (HeapWalk(heaps[index],&entry)) if (entry.wFlags&PROCESS_HEAP_ENTRY_BUSY) bytes+=entry.cbData;
        HeapUnlock(heaps[index]);
    }
    sp[0].i=bytes; return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_motionBurst_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame=test_window((struct Hjava_lang_String *)sp[0].p), child=NULL; int i;
    if (frame) EnumChildWindows(frame,find_canvas,(LPARAM)&child);
    if (!child) { SignalError(ee,"java/lang/RuntimeException","motion canvas missing"); return sp; }
    for (i=0;i<1000;i++) SendMessage(child,WM_MOUSEMOVE,sp[2].i ? MK_LBUTTON:0,MAKELPARAM(i,sp[1].i));
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_mousePosition_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame=test_window((struct Hjava_lang_String *)sp[0].p), child=NULL; POINT point;
    if (frame) EnumChildWindows(frame,find_canvas,(LPARAM)&child);
    if (!child) { SignalError(ee,"java/lang/RuntimeException","motion canvas missing"); return sp; }
    GetCursorPos(&point); ScreenToClient(child,&point);
    sp[0].i=MAKELPARAM(point.x,point.y); return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_input_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame=test_window((struct Hjava_lang_String *)sp[0].p), child=NULL;
    if (frame) EnumChildWindows(frame,sp[1].i ? find_edit:find_canvas,(LPARAM)&child);
    if (!child || !PostMessageW(child,sp[2].i,sp[3].i,MAKELPARAM(sp[4].i,sp[5].i)))
        SignalError(ee,"java/lang/RuntimeException","input target not found");
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_move_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    if (!window || !SetWindowPos(window,NULL,sp[1].i,sp[2].i,sp[3].i,sp[4].i,SWP_NOZORDER|SWP_NOACTIVATE))
        SignalError(ee,"java/lang/RuntimeException","cannot move test window");
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_activation_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=test_window((struct Hjava_lang_String *)sp[0].p);
    if (!window || !PostMessage(window,WM_ACTIVATE,sp[1].i ? WA_ACTIVE:WA_INACTIVE,0))
        SignalError(ee,"java/lang/RuntimeException","activation target missing");
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_windowState_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    if (!window) SignalError(ee,"java/lang/RuntimeException","window not found");
    else ShowWindow(window,sp[1].i);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_windowEnabled_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=test_window((struct Hjava_lang_String *)sp[0].p);
    sp[0].i=window && IsWindowEnabled(window); return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_chooseFile_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=test_window_matching((struct Hjava_lang_String *)sp[0].p,TRUE);
    struct Hjava_lang_String *path=(struct Hjava_lang_String *)sp[1].p;
    WCHAR wide[MAX_PATH]; HWND edit; int length;
    if (!window || !path || (length=javaStringLength(path))>=MAX_PATH) {
        SignalError(ee,"java/lang/RuntimeException","invalid file dialog request"); return sp;
    }
    javaString2unicode(path,(unicode *)wide,length); wide[length]=0;
    edit=file_edit(window);
    if (!edit) { SignalError(ee,"java/lang/RuntimeException","filename editor missing"); return sp; }
    SetWindowTextW(edit,wide);
    PostMessage(window,WM_COMMAND,IDOK,0); return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_fileDialogReady_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=test_window_matching((struct Hjava_lang_String *)sp[0].p,TRUE);
    HWND edit=file_edit(window);
    sp[0].i=window && IsWindowVisible(window) && IsWindowEnabled(window) && edit;
    return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_close_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    if (!window || !PostMessage(window,WM_CLOSE,0,0))
        SignalError(ee,"java/lang/RuntimeException","cannot close test window");
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_click_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    struct Hjava_lang_String *label = (struct Hjava_lang_String *)sp[1].p;
    WCHAR text[256]; struct window_search search; int length;
    if (!window || !label || (length = javaStringLength(label)) > 255) {
        SignalError(ee,"java/lang/RuntimeException","invalid button search"); return sp;
    }
    javaString2unicode(label,(unicode *)text,length); text[length] = 0;
    search.title = text; search.window = NULL;
    EnumChildWindows(window,find_window,(LPARAM)&search);
    if (!search.window) SignalError(ee,"java/lang/RuntimeException","button text not found");
    else SendMessage(search.window,BM_CLICK,0,0);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_paint_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame=test_window((struct Hjava_lang_String *)sp[0].p);
    struct Hjava_lang_String *label=(struct Hjava_lang_String *)sp[1].p;
    WCHAR text[256]; struct window_search search; int length;
    if (!frame || !label || (length=javaStringLength(label))>255) {
        SignalError(ee,"java/lang/RuntimeException","invalid paint request"); return sp;
    }
    javaString2unicode(label,(unicode *)text,length); text[length]=0;
    search.title=text; search.window=NULL; EnumChildWindows(frame,find_window,(LPARAM)&search);
    if (!search.window) SignalError(ee,"java/lang/RuntimeException","paint target not found");
    else RedrawWindow(search.window,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_paintAll_stub(stack_item *sp,struct execenv *ee)
{
    HWND frame=test_window((struct Hjava_lang_String *)sp[0].p);
    if (frame) RedrawWindow(frame,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
    return sp;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_paneSpans_stub(stack_item *sp,struct execenv *ee)
{
    execute_java_dynamic_method(ee,(HObject *)sp[0].p,"setSpans","(IIII)V",220,160,400,300);
    if (!exceptionOccurred(ee)) execute_java_dynamic_method(ee,(HObject *)sp[0].p,"setInsets","()V");
    return sp;
}

__declspec(dllexport) stack_item *Java_WinAwtProbe_inkLeft_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    struct Hjava_lang_String *label = (struct Hjava_lang_String *)sp[1].p;
    WCHAR text[256]; struct window_search search; int length, x, y, left = -1;
    RECT rect; HDC dc;
    if (!window || !label || (length = javaStringLength(label)) > 255) {
        SignalError(ee,"java/lang/RuntimeException","invalid label search"); sp[0].i = -1; return sp+1;
    }
    javaString2unicode(label,(unicode *)text,length); text[length] = 0;
    search.title = text; search.window = NULL; EnumChildWindows(window,find_window,(LPARAM)&search);
    if (!search.window) SignalError(ee,"java/lang/RuntimeException","label text not found");
    else {
        /* CMake starts child processes with a hidden initial show state. Force
         * this process-owned test frame visible before reading screen pixels. */
        ShowWindow(window,SW_SHOW);
        RedrawWindow(search.window,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
        GetClientRect(search.window,&rect); dc = GetDC(search.window);
        for (x = 0; dc && x < rect.right && left < 0; x++) for (y = 0; y < rect.bottom; y++) {
            COLORREF pixel = GetPixel(dc,x,y);
            if (GetRValue(pixel)<128 && GetGValue(pixel)<128 && GetBValue(pixel)<128) { left = x; break; }
        }
        if (dc) ReleaseDC(search.window,dc);
    }
    sp[0].i = left; return sp+1;
}
