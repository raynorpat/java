/* Drive only windows owned by this test process; usable with either VM. */
#include <windows.h>
#include "StubPreamble.h"
struct window_search { WCHAR *title; HWND window; };
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
static HWND test_window(struct Hjava_lang_String *title)
{
    WCHAR text[256]; struct window_search search; int length;
    if (!title) return NULL;
    length = javaStringLength(title);
    if (length > 255) return NULL;
    javaString2unicode(title,(unicode *)text,length); text[length] = 0;
    search.title = text; search.window = NULL;
    EnumWindows(find_window,(LPARAM)&search);
    return search.window;
}
__declspec(dllexport) stack_item *Java_WinAwtProbe_move_stub(stack_item *sp,struct execenv *ee)
{
    HWND window = test_window((struct Hjava_lang_String *)sp[0].p);
    if (!window || !SetWindowPos(window,NULL,sp[1].i,sp[2].i,sp[3].i,sp[4].i,SWP_NOZORDER|SWP_NOACTIVATE))
        SignalError(ee,"java/lang/RuntimeException","cannot move test window");
    return sp;
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

