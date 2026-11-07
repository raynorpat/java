/* Reconstructed from JDK 1.1.5 winawt_g.dll, awt_Toolkit.cpp.
 * Query bodies: 1002360D, 10023625, 1002363D, 100236DC, 100237F3.
 * Toolkit lifecycle and event dispatch are reconstructed separately. */
#include <windows.h>
extern "C" {
#include "StubPreamble.h"
}

#ifndef AWT115_QUERIES_ONLY
#include "awt_Toolkit.h"
#include "awt_Component.h"
#include "awt_Image.h"
#include "awt_Menu.h"

AwtToolkit &AwtToolkit::GetInstance()
{
    static AwtToolkit toolkit;
    return toolkit;
}

/* DR2-independent Win32 implementation: 10022729/100227A0. */
BOOL AwtToolkit::Initialize(BOOL standalone)
{
    WNDCLASSA wc = {0};
    if (thread) return FALSE;
    HDC desktop = GetDC(NULL);
    if (!desktop) return FALSE;
    int depth = GetDeviceCaps(desktop,BITSPIXEL) * GetDeviceCaps(desktop,PLANES);
    ReleaseDC(NULL,desktop);
    if (depth < 4) return FALSE;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "AwtToolkit";
    if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return FALSE;
    thread = GetCurrentThreadId();
    window = CreateWindowExA(0, "AwtToolkit", "The AWT Toolkit Window", WS_DISABLED,
        -1, -1, 0, 0, NULL, NULL, wc.hInstance, NULL);
    if (!window) { thread = 0; return FALSE; }
    hook = SetWindowsHookExA(WH_GETMESSAGE, GetMessageHook, NULL, thread);
    if (!hook) { DestroyWindow(window); window = NULL; thread = 0; return FALSE; }
    return TRUE;
}

struct AwtRequest { void (*function)(void *); void *argument; DWORD caller; };
void AwtToolkit::Invoke(void (*function)(void *), void *argument)
{
    if (GetCurrentThreadId() == thread) function(argument);
    else {
        AwtRequest request = { function, argument, GetCurrentThreadId() };
        SendMessageA(window, 0x464, 0, (LPARAM)&request);
    }
}

LRESULT CALLBACK AwtToolkit::WndProc(HWND hwnd, UINT message, WPARAM w, LPARAM l)
{
    if (message == 0x464) {
        AwtRequest *request = (AwtRequest *)l;
        AwtToolkit &toolkit=GetInstance(); DWORD previous=toolkit.callerThread;
        toolkit.callerThread=request->caller;
        request->function(request->argument); toolkit.callerThread=previous; return 0;
    }
    if (message == 0x466) { GetInstance().Cleanup(); return 0; }
    return DefWindowProcA(hwnd, message, w, l);
}

LRESULT CALLBACK AwtToolkit::GetMessageHook(int code, WPARAM w, LPARAM l)
{
    /* Input is staged by the peer window procedure, including sent messages. */
    return CallNextHookEx(GetInstance().hook, code, w, l);
}

void AwtToolkit::Cleanup()
{
    if (!window) return;
    if (GetCurrentThreadId() != thread) {
        SendMessageA(window, 0x466, 0, 0); return;
    }
    awtMenuCleanup();
    AwtComponent::Cleanup();
    awtCleanupImagePalette();
    DestroyWindow(window); window = NULL;
    UnregisterClassA("AwtToolkit",GetModuleHandle(NULL));
    UnhookWindowsHookEx(hook); hook = NULL;
    quit = TRUE;
    PostQuitMessage(0);
    thread = 0;
}

static void awtAtExit() { AwtToolkit::GetInstance().Cleanup(); }

/* WColor.cpp, 10005210: selector is a static native argument, not a receiver. */
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WColor_getDefaultColor_stub(
    stack_item *sp, struct execenv *ee)
{
    static const int indices[] = {5,8,6,0,4,7,15,18,13};
    int selector = sp[0].i;
    if (selector < 1 || selector > 9) sp[0].p = NULL;
    else {
        COLORREF color = GetSysColor(indices[selector - 1]);
        sp[0].p = execute_java_constructor(ee,"java/awt/Color",NULL,"(III)",
            (int)GetRValue(color),(int)GetGValue(color),(int)GetBValue(color));
    }
    return sp + 1;
}

void AwtToolkit::EventLoop()
{
    MSG message;
    sysAtexit(awtAtExit);
    while (!quit) {
        WaitMessage();
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { quit = TRUE; exitCode = (int)message.wParam; break; }
            if (message.message != WM_NULL) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }
    }
    PostQuitMessage(exitCode);
}

extern "C" __declspec(dllexport) int AwtInit()
{ return AwtToolkit::GetInstance().Initialize(FALSE); }

extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WToolkit_init_stub(
    stack_item *sp, struct execenv *ee)
{
    if (!AwtToolkit::GetInstance().Initialize(TRUE))
        SignalError(ee, "java/lang/InternalError", "Cannot initialize Windows toolkit");
    return sp;
}

extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WToolkit_eventLoop_stub(
    stack_item *sp, struct execenv *ee)
{ AwtToolkit::GetInstance().EventLoop(); return sp; }

extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WToolkit_makeColorModel_stub(
    stack_item *sp, struct execenv *ee)
{
    sp[0].p = awtImageColorModel(ee);
    return sp + 1;
}

extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WToolkit_sync_stub(
    stack_item *sp, struct execenv *ee)
{ GdiFlush(); return sp; }

extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WToolkit_beep_stub(
    stack_item *sp, struct execenv *ee)
{ MessageBeep(0); return sp; }
#endif

extern "C" stack_item *Java_sun_awt_windows_WToolkit_getScreenWidth_stub(
    stack_item *sp, struct execenv *ee)
{
    sp[0].i = GetSystemMetrics(SM_CXSCREEN);
    return sp + 1;
}

extern "C" stack_item *Java_sun_awt_windows_WToolkit_getScreenHeight_stub(
    stack_item *sp, struct execenv *ee)
{
    sp[0].i = GetSystemMetrics(SM_CYSCREEN);
    return sp + 1;
}

extern "C" stack_item *Java_sun_awt_windows_WToolkit_getScreenResolution_stub(
    stack_item *sp, struct execenv *ee)
{
    HWND desktop = GetDesktopWindow();
    HDC dc = GetDC(desktop);
    sp[0].i = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(desktop, dc);
    return sp + 1;
}

extern "C" stack_item *Java_sun_awt_windows_WToolkit_loadSystemColors_stub(
    stack_item *sp, struct execenv *ee)
{
    /* Original table at 100491B8, java.awt.SystemColor order. */
    static const int indices[] = {
        1, 2, 9, 10, 3, 19, 11, 5, 6, 8, 4, 7, 15,
        18, 13, 14, 17, 15, 18, 22, 20, 16, 21, 0, 24, 23
    };
    HArrayOfInt *array = (HArrayOfInt *)sp[1].p;
    unsigned int length = obj_length(array);
    /* Deviation: Sun compares against sizeof(indices), reading beyond the
     * table for oversized arrays. Bound by its actual element count. */
    for (unsigned int i = 0; i < sizeof(indices) / sizeof(indices[0]) && i < length; ++i) {
        COLORREF color = GetSysColor(indices[i]);
        unhand(array)->body[i] = (GetRValue(color) << 16) |
                                (GetGValue(color) << 8) | GetBValue(color);
    }
    return sp;
}
