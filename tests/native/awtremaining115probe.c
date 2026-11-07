/* Focused contracts: every window/DC belongs to this process. Printing APIs
 * are intercepted only in this process's winawt import table, before any job. */
#include <windows.h>
#include <commdlg.h>
#include "StubPreamble.h"
#define NATIVE(name) __declspec(dllexport) stack_item *Java_RemainingProbe115_##name##_stub(stack_item *sp,struct execenv *ee)
static HMODULE library(void) {
    HMODULE module=GetModuleHandleA("winawt.dll");
    return module?module:GetModuleHandleA("winawt_g.dll");
}
NATIVE(exports) {
    static const char *names[]={"CreateEmbeddedFrame","PrintEmbeddedFrame","Java_sun_awt_EmbeddedFrame_setPeer_stub","Java_sun_awt_windows_WEmbeddedFramePeer_create_stub",
        "Java_sun_awt_windows_WFramePeer__0005fsetIconImage_stub","Java_sun_awt_windows_WGraphics_createFromPrintJob_stub","Java_sun_awt_windows_WGraphics_print_stub","Java_sun_awt_windows_WGraphics_close_stub","Java_sun_awt_windows_WPrintJob_end_stub"};
    int group=sp[0].i,start=group==0?0:group==1?4:5,end=group==0?4:group==1?5:9,i,count=0;
    for(i=start;i<end;i++) if(GetProcAddress(library(),names[i])) count++;
    sp[0].i=count;return sp+1;
}
NATIVE(host) {
    sp[0].i=(long)CreateWindowA("STATIC","RemainingProbe115",WS_POPUP,0,0,120,90,NULL,NULL,GetModuleHandle(NULL),NULL);return sp+1;
}
NATIVE(embed) {
    typedef HObject *(__cdecl *Create)(HWND);
    Create create=(Create)GetProcAddress(library(),"CreateEmbeddedFrame");
    sp[0].p=create?create((HWND)sp[0].i):NULL;return sp+1;
}
NATIVE(child) {
    HWND host=(HWND)sp[0].i,child=GetWindow(host,GW_CHILD);
    sp[0].i=sp[1].i ? child && GetParent(child)==host && (GetWindowLong(child,GWL_STYLE)&WS_CHILD) : child==NULL;
    return sp+1;
}
NATIVE(destroy) {sp[0].i=DestroyWindow((HWND)sp[0].i);return sp+1;}
static HDC memoryDC;
static HBITMAP bitmap;
static HGDIOBJ oldBitmap;
static HDC makeDC(void) {
    BITMAPINFO info;void *bits;
    ZeroMemory(&info,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=128;info.bmiHeader.biHeight=-128;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    memoryDC=CreateCompatibleDC(NULL);bitmap=CreateDIBSection(memoryDC,&info,DIB_RGB_COLORS,&bits,NULL,0);
    oldBitmap=SelectObject(memoryDC,bitmap);PatBlt(memoryDC,0,0,128,128,WHITENESS);return memoryDC;
}
NATIVE(dc) {sp[0].i=(long)makeDC();return sp+1;}
NATIVE(print) {
    typedef void (__cdecl *Print)(HObject *,HDC);
    Print print=(Print)GetProcAddress(library(),"PrintEmbeddedFrame");
    if(print) print((HObject *)sp[0].p,(HDC)sp[1].i);return sp;
}
NATIVE(pixel) {sp[0].i=GetPixel(memoryDC,sp[0].i,sp[1].i);return sp+1;}
NATIVE(closeDC) {
    BOOL valid=GetObjectType(memoryDC)==OBJ_MEMDC;
    if(valid) {SelectObject(memoryDC,oldBitmap);DeleteDC(memoryDC);}
    DeleteObject(bitmap);bitmap=NULL;memoryDC=NULL;sp[0].i=valid;return sp+1;
}
struct search {WCHAR title[256];HWND window;};
static BOOL CALLBACK find(HWND window,LPARAM value) {
    DWORD process;WCHAR title[256];struct search *s=(struct search *)value;
    GetWindowThreadProcessId(window,&process);if(process!=GetCurrentProcessId())return TRUE;
    GetWindowTextW(window,title,256);if(!lstrcmpW(title,s->title)){s->window=window;return FALSE;}return TRUE;
}
NATIVE(icon) {
    struct search s;int length=javaStringLength((struct Hjava_lang_String *)sp[0].p),result=-1;
    ICONINFO icon;HICON handle;HDC dc;HGDIOBJ previous;
    if(length>255){sp[0].i=-1;return sp+1;}
    javaString2unicode((struct Hjava_lang_String *)sp[0].p,(unicode *)s.title,length);s.title[length]=0;s.window=NULL;
    EnumWindows(find,(LPARAM)&s);handle=(HICON)SendMessage(s.window,WM_GETICON,ICON_BIG,0);
    if(handle && GetIconInfo(handle,&icon)) {
        dc=CreateCompatibleDC(NULL);previous=SelectObject(dc,icon.hbmColor);result=GetPixel(dc,4,4);
        SelectObject(dc,previous);DeleteDC(dc);DeleteObject(icon.hbmColor);DeleteObject(icon.hbmMask);
    }
    sp[0].i=result;return sp+1;
}
NATIVE(resources) {sp[0].i=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);return sp+1;}
static BOOL cancelPrint;
static int calls[8];
static HObject *printCallback;
NATIVE(duringPrint) {printCallback=(HObject *)sp[0].p;return sp;}
static BOOL WINAPI printDialog(LPPRINTDLGA dialog) {
    DEVMODEA *mode;calls[0]++;if(cancelPrint)return FALSE;
    dialog->hDC=makeDC();dialog->hDevMode=GlobalAlloc(GHND,sizeof(DEVMODEA));
    mode=(DEVMODEA *)GlobalLock(dialog->hDevMode);mode->dmSize=sizeof(DEVMODEA);mode->dmFields=DM_PRINTQUALITY;mode->dmPrintQuality=300;
    GlobalUnlock(dialog->hDevMode);return TRUE;
}
static int WINAPI startDoc(HDC dc,const DOCINFOA *info){calls[1]++;return dc==memoryDC?1:-1;}
static int WINAPI startPage(HDC dc){calls[2]++;return dc==memoryDC?1:-1;}
static int WINAPI endPage(HDC dc){calls[3]++;return dc==memoryDC?1:-1;}
static int WINAPI endDoc(HDC dc){calls[4]++;return dc==memoryDC?1:-1;}
static BOOL WINAPI deleteDC(HDC dc){
    if(dc==memoryDC) {calls[5]++;SelectObject(dc,oldBitmap);DeleteObject(bitmap);bitmap=NULL;memoryDC=NULL;}
    return DeleteDC(dc);
}
static int WINAPI caps(HDC dc,int index) {
    if(dc==memoryDC) {
        if(index==LOGPIXELSX || index==LOGPIXELSY)return 72;
        if(index==PHYSICALWIDTH)return 612;if(index==PHYSICALHEIGHT)return 792;
    }
    return GetDeviceCaps(dc,index);
}
static LRESULT WINAPI sendMessage(HWND window,UINT message,WPARAM w,LPARAM l) {
    if(message==WM_PRINT) {
        DWORD process;GetWindowThreadProcessId(window,&process);
        if(process!=GetCurrentProcessId() || (HDC)w!=memoryDC || l!=23)return 0;
        calls[6]++;
        if(printCallback) {
            HObject *callback=printCallback;printCallback=NULL;
            execute_java_dynamic_method(EE(),callback,"run","()V");
            calls[7]=GetObjectType((HDC)w)==OBJ_MEMDC;
            /* Keep testing this process's DC; never forward a retired HDC. */
            if(!calls[7])return 0;
        }
    }
    return SendMessageA(window,message,w,l);
}
static int patch(const char *name,void *function) {
    BYTE *base=(BYTE *)library();IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *imports=(IMAGE_IMPORT_DESCRIPTOR *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(;imports->Name;imports++) {
        IMAGE_THUNK_DATA *names=(IMAGE_THUNK_DATA *)(base+imports->OriginalFirstThunk),*slots=(IMAGE_THUNK_DATA *)(base+imports->FirstThunk);
        for(;names->u1.AddressOfData;names++,slots++) if(!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) {
            IMAGE_IMPORT_BY_NAME *import=(IMAGE_IMPORT_BY_NAME *)(base+names->u1.AddressOfData);
            if(!lstrcmpA((char *)import->Name,name)) {
                DWORD old;VirtualProtect(&slots->u1.Function,sizeof(DWORD),PAGE_READWRITE,&old);
                slots->u1.Function=(DWORD)function;VirtualProtect(&slots->u1.Function,sizeof(DWORD),old,&old);return 1;
            }
        }
    }
    return 0;
}
NATIVE(printing) {
    int patched=0;cancelPrint=sp[0].i;ZeroMemory(calls,sizeof(calls));
    patched+=patch("PrintDlgA",printDialog);patched+=patch("StartDocA",startDoc);
    patched+=patch("StartPage",startPage);patched+=patch("EndPage",endPage);patched+=patch("EndDoc",endDoc);
    patched+=patch("DeleteDC",deleteDC);patched+=patch("GetDeviceCaps",caps);
    patch("SendMessageA",sendMessage);
    sp[0].i=patched==7;return sp+1;
}
NATIVE(calls) {sp[0].i=calls[sp[0].i];return sp+1;}
