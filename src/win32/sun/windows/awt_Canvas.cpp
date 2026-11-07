/* WCanvasPeer (also WPanelPeer), winawt_g100031C7 ->10002DF6. */
#include "awt_Component.h"
struct CanvasRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    BOOL failed;
};
static void createCanvas(void *argument)
{
    CanvasRequest *r = (CanvasRequest *)argument;
    AwtComponent *parent = AwtComponent::Get(r->parent);
    if (!parent) { r->failed = TRUE; return; }
    AwtComponent *canvas = new AwtComponent(r->peer);
    if (!canvas->Create("SunAwtCanvas",0x46000000,0,parent->GetHWnd())) {
        delete canvas; r->failed = TRUE;
    }
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WCanvasPeer_create_stub(stack_item *sp,struct execenv *ee)
{
    CanvasRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,
        (Hsun_awt_windows_WComponentPeer *)sp[1].p,FALSE};
    AwtToolkit::GetInstance().Invoke(createCanvas,&r);
    if (r.failed) SignalError(ee,"java/lang/NullPointerException","invalid canvas parent");
    return sp;
}
