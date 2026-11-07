/* Reconstructed from JDK 1.1.5 winawt_g.dll, awt_Toolkit.cpp.
 * Query bodies: 1002360D, 10023625, 1002363D, 100236DC, 100237F3.
 * Toolkit lifecycle and event dispatch are reconstructed separately. */
#include <windows.h>
extern "C" {
#include "StubPreamble.h"
}

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
