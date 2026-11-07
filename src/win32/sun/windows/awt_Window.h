#ifndef AWT_WINDOW_H
#define AWT_WINDOW_H
#include "awt_Component.h"
extern "C" {
#include "sun_awt_windows_WWindowPeer.h"
#include "sun_awt_windows_WFramePeer.h"
#include "java_awt_Container.h"
#include "java_awt_Insets.h"
}
class AwtWindow : public AwtComponent {
public:
    AwtWindow(Hsun_awt_windows_WComponentPeer *peer) : AwtComponent(peer), iconic(FALSE) {}
    virtual void UpdateInsets(Hjava_awt_Insets *insets);
    virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l);
protected:
    BOOL iconic;
};
class AwtFrame : public AwtWindow {
public:
    AwtFrame(Hsun_awt_windows_WComponentPeer *peer) : AwtWindow(peer) {}
    virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l);
};
#endif
