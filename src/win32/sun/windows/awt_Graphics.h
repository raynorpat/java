#ifndef AWT_GRAPHICS_H
#define AWT_GRAPHICS_H
#include "awt_Component.h"
#include "awt_PixelFormat.h"
extern "C" {
#include "sun_awt_windows_WGraphics.h"
}
/* Owner-thread-only refcounts. A component retires its pointer before destroying
 * the HWND; offscreen graphics keep their bitmap alive after Image.flush(). */
struct AwtSurface {
    AwtComponent *component;
    CRITICAL_SECTION nativeLock;
    volatile DWORD nativeOwner;
    unsigned nativeLocks;
    unsigned references;
    HDC dc;
    HBITMAP bitmap;
    HGDIOBJ oldBitmap;
    DWORD *bits; /* raw DIB address; consult format.depth and stride */
    AwtPixelFormat format;
    int stride;
    int *ditherErrors;
    unsigned imageHints;
    HPALETTE palette,oldPalette;
    BYTE *alpha, *coverage, *rows;
    unsigned remaining;
    int width, height;
    COLORREF background;
    BOOL ownsDC, finished;
    AwtSurface(AwtComponent *c) : component(c), nativeOwner(0), nativeLocks(0), references(1), dc(NULL),
        bitmap(NULL), oldBitmap(NULL), bits(NULL), stride(0), ditherErrors(NULL), imageHints(0), palette(NULL), oldPalette(NULL), alpha(NULL), coverage(NULL), rows(NULL), remaining(0), width(0), height(0),
        background(RGB(255,255,255)), ownsDC(FALSE), finished(FALSE) { InitializeCriticalSection(&nativeLock); }
    DWORD Pixel(int x,int y) const { return format.Read((BYTE *)bits+y*stride,x); }
    void Store(int x,int y,DWORD pixel) { format.Write((BYTE *)bits+y*stride,x,pixel); }
    ~AwtSurface();
    void Release() { if (!--references) delete this; }
};
/* Return TRUE if the toolkit acquired the mutex, FALSE for a request
 * reentered by the native client that already owns it. */
BOOL awtEnterSurface(AwtSurface *surface);
class AwtSurfaceLock {
    AwtSurface *surface;
    BOOL entered;
public:
    AwtSurfaceLock(AwtSurface *s) : surface(s) {
        ++surface->references; entered=awtEnterSurface(surface);
    }
    ~AwtSurfaceLock() {
        if (entered) LeaveCriticalSection(&surface->nativeLock);
        surface->Release();
    }
private:
    AwtSurfaceLock(const AwtSurfaceLock &);
    AwtSurfaceLock &operator=(const AwtSurfaceLock &);
};
/* These interfaces must be called on the toolkit owner thread. A lease saves
 * and restores DC state; callers must release it before returning to Java. */
struct AwtDC {
    HDC dc;
    HWND window;
    int saved;
    AwtSurface *locked;
    BOOL entered;
};
BOOL awtGraphicsDC(Hsun_awt_windows_WGraphics *, AwtDC *);
void awtReleaseDC(AwtDC *);
AwtSurface *awtGraphicsSurface(Hsun_awt_windows_WGraphics *);
COLORREF awtGraphicsColor(Hsun_awt_windows_WGraphics *);
COLORREF awtGraphicsXORColor(Hsun_awt_windows_WGraphics *);
BOOL awtGraphicsXOR(Hsun_awt_windows_WGraphics *);
#endif
