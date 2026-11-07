#ifndef AWT_GRAPHICS_H
#define AWT_GRAPHICS_H
#include "awt_Component.h"
/* AwtDC ownership: the component retires its surface before HWND destruction;
 * surviving Graphics objects keep the record, but cannot draw into a new HWND. */
struct AwtSurface {
    AwtComponent *component;
    unsigned references;
    AwtSurface(AwtComponent *c) : component(c), references(1) {}
    void Release() { if (!--references) delete this; }
};
#endif
