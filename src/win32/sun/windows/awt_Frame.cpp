/* Frame creation recovered from winawt_g 10011561 -> 10010C30. */
#include "awt_Window.h"
#include "awt_Menu.h"
/* Activation/focus restoration: 10010FA4. Defer Java work until the
 * activating SendMessage/ShowWindow caller has released its monitors. */
LRESULT AwtFrame::WindowProc(UINT message,WPARAM w,LPARAM l)
{
    if (started && message==WM_ACTIVATE) {
        BOOL active=LOWORD(w)!=WA_INACTIVE;
        PostMessage(hwnd,WM_APP+2,active ? 205:206,0);
        if (active) PostMessage(hwnd,WM_APP+8,0,0);
    }
    if (message==WM_APP+8) {
        if (started && GetActiveWindow()==hwnd) {
            Hsun_awt_windows_WComponentPeer *focus=(Hsun_awt_windows_WComponentPeer *)
                execute_java_dynamic_method(EE(),(HObject *)peer,"getFocusPeer","()Lsun/awt/windows/WComponentPeer;");
            AwtComponent *component=AwtComponent::Get(focus);
            if (component) SetFocus(component->GetHWnd());
            else if (!exceptionOccurred(EE())) execute_java_dynamic_method(EE(),(HObject *)peer,"setDefaultFocus","()V");
            if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
        }
        return 0;
    }
    return AwtWindow::WindowProc(message,w,l);
}
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
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer_setMenuBar0_stub(stack_item *sp,struct execenv *)
{
    awtSetMenuBar((Hsun_awt_windows_WComponentPeer *)sp[0].p,(HObject *)sp[1].p);
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer_create_stub(stack_item *sp,struct execenv *)
{
    sun_awt_windows_WFramePeer_create((Hsun_awt_windows_WFramePeer *)sp[0].p,
        (Hsun_awt_windows_WComponentPeer *)sp[1].p);
    return sp;
}
