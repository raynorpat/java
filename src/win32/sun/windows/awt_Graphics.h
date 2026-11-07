#ifndef AWT_GRAPHICS_H
#define AWT_GRAPHICS_H
#include "awt_Component.h"
extern "C" {
#include "sun_awt_windows_WGraphics.h"
}
/* Owner-thread-only refcounts. A component retires its pointer before destroying
 * the HWND; offscreen graphics keep their bitmap alive after Image.flush(). */
struct AwtSurface {
    AwtComponent *component;
    unsigned references;
    HDC dc;
    HBITMAP bitmap;
    HGDIOBJ oldBitmap;
    DWORD *bits;
    BYTE *alpha;
    int width, height;
    COLORREF background;
    BOOL ownsDC, finished;
    AwtSurface(AwtComponent *c) : component(c), references(1), dc(NULL),
        bitmap(NULL), oldBitmap(NULL), bits(NULL), alpha(NULL), width(0), height(0),
        background(RGB(255,255,255)), ownsDC(FALSE), finished(FALSE) {}
    ~AwtSurface();
    void Release() { if (!--references) delete this; }
};
/* These interfaces must be called on the toolkit owner thread. A lease saves
 * and restores DC state; callers must release it before returning to Java. */
struct AwtDC {
    HDC dc;
    HWND window;
    int saved;
};
BOOL awtGraphicsDC(Hsun_awt_windows_WGraphics *, AwtDC *);
void awtReleaseDC(AwtDC *);
AwtSurface *awtGraphicsSurface(Hsun_awt_windows_WGraphics *);
COLORREF awtGraphicsColor(Hsun_awt_windows_WGraphics *);
COLORREF awtGraphicsXORColor(Hsun_awt_windows_WGraphics *);
BOOL awtGraphicsXOR(Hsun_awt_windows_WGraphics *);
#endif
