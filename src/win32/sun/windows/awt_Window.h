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
    AwtWindow(Hsun_awt_windows_WComponentPeer *peer) : AwtComponent(peer) {}
    virtual void UpdateInsets(Hjava_awt_Insets *insets);
};
class AwtFrame : public AwtWindow {
public:
    AwtFrame(Hsun_awt_windows_WComponentPeer *peer) : AwtWindow(peer) {}
};
#endif
