#ifndef AWT_EMBEDDED_FRAME_H
#define AWT_EMBEDDED_FRAME_H
#include "awt_Window.h"
/* The host HWND is borrowed. AwtComponent retires only the created child. */
class AwtEmbeddedFrame : public AwtFrame {
public:
    AwtEmbeddedFrame(Hsun_awt_windows_WComponentPeer *peer) : AwtFrame(peer) {}
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (GetWindowLong(hwnd,GWL_STYLE)&WS_CHILD) return AwtComponent::WindowProc(message,w,l);
        return AwtFrame::WindowProc(message,w,l);
    }
};
#endif
