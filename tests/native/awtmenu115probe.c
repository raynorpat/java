/* Menu contracts driven only through this process's own Win32 windows. */
#include <windows.h>
#include "StubPreamble.h"
__declspec(dllexport) stack_item *Java_WinAwtMenuProbe_exports_stub(stack_item *sp,struct execenv *ee)
{
    static const char *names[]={
        "Java_sun_awt_windows_WCheckboxMenuItemPeer_setState_stub",
        "Java_sun_awt_windows_WFramePeer_setMenuBar0_stub",
        "Java_sun_awt_windows_WMenuBarPeer_addMenu_stub",
        "Java_sun_awt_windows_WMenuBarPeer_create_stub",
        "Java_sun_awt_windows_WMenuBarPeer_delMenu_stub",
        "Java_sun_awt_windows_WMenuItemPeer__0005fdispose_stub",
        "Java_sun_awt_windows_WMenuItemPeer__0005fsetLabel_stub",
        "Java_sun_awt_windows_WMenuItemPeer_create_stub",
        "Java_sun_awt_windows_WMenuItemPeer_enable_stub",
        "Java_sun_awt_windows_WMenuPeer_addSeparator_stub",
        "Java_sun_awt_windows_WMenuPeer_createMenu_stub",
        "Java_sun_awt_windows_WMenuPeer_createSubMenu_stub",
        "Java_sun_awt_windows_WMenuPeer_delItem_stub",
        "Java_sun_awt_windows_WPopupMenuPeer__0005fshow_stub",
        "Java_sun_awt_windows_WPopupMenuPeer_createMenu_stub"
    };
    HMODULE library=GetModuleHandleA("winawt.dll"); int i,count=0;
    if (!library) library=GetModuleHandleA("winawt_g.dll");
    for (i=0;i<sizeof(names)/sizeof(names[0]);i++) if (library && GetProcAddress(library,names[i])) count++;
    sp[0].i=count; return sp+1;
}
struct search { WCHAR title[256]; HWND window; };
static BOOL CALLBACK find_window(HWND window, LPARAM value)
{
    struct search *s=(struct search *)value; WCHAR title[256]; DWORD process;
    GetWindowThreadProcessId(window,&process);
    if (process!=GetCurrentProcessId()) return TRUE;
    GetWindowTextW(window,title,256);
    if (!lstrcmpW(title,s->title)) { s->window=window; return FALSE; }
    return TRUE;
}
static HWND frame(struct Hjava_lang_String *title)
{
    struct search s; int length=javaStringLength(title);
    if (length>255) return NULL;
    javaString2unicode(title,(unicode *)s.title,length); s.title[length]=0; s.window=NULL;
    EnumWindows(find_window,(LPARAM)&s); return s.window;
}
static HMENU menu_item(HWND window,int menu,int *item,int nested)
{
    HMENU result=GetMenu(window);
    if (menu>=0) result=GetSubMenu(result,menu);
    if (nested>=0) { result=GetSubMenu(result,*item); *item=nested; }
    return result;
}
__declspec(dllexport) stack_item *Java_WinAwtMenuProbe_query_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=frame((struct Hjava_lang_String *)sp[0].p);
    int item=sp[2].i,operation=sp[4].i,result=-1;
    HMENU menu=menu_item(window,sp[1].i,&item,sp[3].i);
    MENUITEMINFOW info; ZeroMemory(&info,sizeof(info)); info.cbSize=sizeof(info);
    if (!window) { SignalError(ee,"java/lang/RuntimeException","menu frame not found"); return sp; }
    if (operation==0) result=menu ? GetMenuItemCount(menu):0;
    else if (operation==1) result=(int)GetMenuState(menu,item,MF_BYPOSITION);
    else if (GetMenuItemInfoW(menu,item,TRUE,&info)) result=0;
    if (operation>=2) {
        info.fMask=MIIM_FTYPE|MIIM_DATA|MIIM_ID;
        if (GetMenuItemInfoW(menu,item,TRUE,&info)) {
            MEASUREITEMSTRUCT measure; ZeroMemory(&measure,sizeof(measure));
            measure.CtlType=ODT_MENU; measure.itemID=info.wID; measure.itemData=info.dwItemData;
            if (operation==2) result=(info.fType&MFT_OWNERDRAW)!=0;
            else if (operation==3 || operation==5) {
                SendMessage(window,WM_MEASUREITEM,0,(LPARAM)&measure);
                result=operation==3 ? measure.itemWidth:measure.itemHeight;
            } else if (operation==4) {
                HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen); HBITMAP bitmap=CreateCompatibleBitmap(screen,320,60);
                HGDIOBJ old=SelectObject(dc,bitmap); DRAWITEMSTRUCT draw; int x,y,ink=0;
                ReleaseDC(NULL,screen);
                ZeroMemory(&draw,sizeof(draw)); draw.CtlType=ODT_MENU; draw.itemID=info.wID;
                draw.itemData=info.dwItemData; draw.itemAction=ODA_DRAWENTIRE;
                draw.hwndItem=(HWND)menu; draw.hDC=dc; SetRect(&draw.rcItem,0,0,320,60);
                if (GetMenuState(menu,item,MF_BYPOSITION)&MF_CHECKED) draw.itemState|=ODS_CHECKED;
                SendMessage(window,WM_DRAWITEM,0,(LPARAM)&draw);
                for (y=5;y<55;y++) for (x=5;x<315;x++) if (GetPixel(dc,x,y)!=GetSysColor(COLOR_MENU)) ink++;
                SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); result=ink;
            }
        }
    }
    sp[0].i=result; return sp+1;
}
__declspec(dllexport) stack_item *Java_WinAwtMenuProbe_activate_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=frame((struct Hjava_lang_String *)sp[0].p); int item=sp[2].i;
    HMENU menu=menu_item(window,sp[1].i,&item,sp[3].i); UINT id=GetMenuItemID(menu,item);
    if (!window || id==(UINT)-1) SignalError(ee,"java/lang/RuntimeException","menu item not found");
    else SendMessage(window,WM_COMMAND,MAKEWPARAM(id,0),0);
    return sp;
}
static BOOL CALLBACK find_popup(HWND window,LPARAM value)
{
    DWORD process; WCHAR name[64]; GetWindowThreadProcessId(window,&process);
    GetClassNameW(window,name,64);
    if (process==GetCurrentProcessId() && !lstrcmpW(name,L"#32768") && IsWindowVisible(window)) {
        *(HWND *)value=window; return FALSE;
    }
    return TRUE;
}
static BOOL CALLBACK find_canvas(HWND window,LPARAM value)
{
    WCHAR name[64]; GetClassNameW(window,name,64);
    if (!lstrcmpW(name,L"SunAwtCanvas") || !lstrcmpW(name,L"AwtCanvas")) { *(HWND *)value=window; return FALSE; }
    return TRUE;
}
__declspec(dllexport) stack_item *Java_WinAwtMenuProbe_popup_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=frame((struct Hjava_lang_String *)sp[0].p),popup=NULL,canvas=NULL;
    HMENU menu; UINT id; EnumWindows(find_popup,(LPARAM)&popup);
    sp[0].i=0;
    if (window && popup) {
        EnumChildWindows(window,find_canvas,(LPARAM)&canvas); if (canvas) window=canvas;
        menu=(HMENU)SendMessage(popup,0x1e1,0,0); /* MN_GETHMENU */
        id=GetMenuItemID(menu,0);
        if (id!=(UINT)-1) { PostMessage(window,WM_COMMAND,MAKEWPARAM(id,0),0); PostMessage(window,WM_CANCELMODE,0,0); sp[0].i=1; }
    }
    return sp+1;
}
static WNDPROC retire_original;
static HObject *retire_item;
static LRESULT CALLBACK retire_proc(HWND window,UINT message,WPARAM w,LPARAM l)
{
    LRESULT result=CallWindowProcA(retire_original,window,message,w,l);
    if (message==WM_COMMAND && !l) {
        HObject *volatile item=retire_item;
        SetWindowLongA(window,GWL_WNDPROC,(LONG)retire_original);
        execute_java_dynamic_method(EE(),item,"removeNotify","()V");
    }
    return result;
}
__declspec(dllexport) stack_item *Java_WinAwtMenuProbe_queueAndRetire_stub(stack_item *sp,struct execenv *ee)
{
    HWND window=frame((struct Hjava_lang_String *)sp[0].p); int item=sp[2].i;
    HMENU menu=menu_item(window,sp[1].i,&item,-1); UINT id=GetMenuItemID(menu,item);
    if (!window || id==(UINT)-1) { SignalError(ee,"java/lang/RuntimeException","retiring menu not found"); return sp; }
    retire_item=(HObject *)sp[3].p;
    retire_original=(WNDPROC)GetWindowLongA(window,GWL_WNDPROC);
    SetWindowLongA(window,GWL_WNDPROC,(LONG)retire_proc);
    SendMessage(window,WM_COMMAND,MAKEWPARAM(id,0),0);
    SetWindowLongA(window,GWL_WNDPROC,(LONG)retire_original); retire_item=NULL;
    return sp;
}
