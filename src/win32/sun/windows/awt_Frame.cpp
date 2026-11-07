/* Frame creation recovered from winawt_g 10011561 -> 10010C30. */
#include "awt_Window.h"
struct FrameRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    BOOL failed;
};
static void createFrame(void *argument)
{
    FrameRequest *r = (FrameRequest *)argument;
    AwtComponent *parent = AwtComponent::Get(r->parent);
    if (r->parent && !parent) { r->failed = TRUE; return; }
    AwtFrame *frame = new AwtFrame(r->peer);
    frame->background = GetSysColor(COLOR_WINDOWFRAME);
    if (!frame->Create("SunAwtFrame", 0x02CF0000, 0x100, parent ? parent->GetHWnd() : NULL)) {
        delete frame; r->failed = TRUE;
    } else frame->UpdateInsets(NULL);
}
extern "C" void sun_awt_windows_WFramePeer_create(Hsun_awt_windows_WFramePeer *p,Hsun_awt_windows_WComponentPeer *parent)
{
    if (!p || !unhand(p)->target) { SignalError(NULL,"java/lang/NullPointerException","invalid peer"); return; }
    FrameRequest r = {(Hsun_awt_windows_WComponentPeer *)p,parent,FALSE};
    AwtToolkit::GetInstance().Invoke(createFrame,&r);
    if (r.failed) SignalError(NULL,"java/lang/InternalError","cannot create frame");
}
/* Menu and image stubs join this file with their owning subsystems. */
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer_create_stub(stack_item *sp,struct execenv *)
{
    sun_awt_windows_WFramePeer_create((Hsun_awt_windows_WFramePeer *)sp[0].p,
        (Hsun_awt_windows_WComponentPeer *)sp[1].p);
    return sp;
}
