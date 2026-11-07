/* Embedded frame contracts recovered from winawt_g 10010C30, 1001165C,
 * 1002238D, 100223FA and 1002C190. */
#include "awt_EmbeddedFrame.h"
extern "C" {
#include "sun_awt_windows_WEmbeddedFrame.h"
}
struct EmbeddedRequest {
    Hsun_awt_windows_WComponentPeer *peer,*parent;
    BOOL failed;
};
static void createEmbedded(void *argument)
{
    EmbeddedRequest *r=(EmbeddedRequest *)argument;
    AwtComponent *parent=AwtComponent::Get(r->parent);
    if (r->parent && !parent) { r->failed=TRUE; return; }
    Hsun_awt_windows_WEmbeddedFrame *target=(Hsun_awt_windows_WEmbeddedFrame *)unhand(r->peer)->target;
    HWND host=(HWND)unhand(target)->handle;
    if (host) {
        RECT client;
        if (!GetClientRect(host,&client)) { r->failed=TRUE; return; }
        Classjava_awt_Component *component=unhand((Hjava_awt_Component *)target);
        component->x=component->y=0;component->width=client.right;component->height=client.bottom;
    }
    AwtEmbeddedFrame *frame=new AwtEmbeddedFrame(r->peer);
    frame->background=GetSysColor(COLOR_WINDOWFRAME);
    if (!frame->Create("SunAwtFrame",host?0x42000000:0x02CF0000,host?4:0x100,
        host?host:parent?parent->GetHWnd():NULL)) {
        delete frame;r->failed=TRUE;
    } else frame->UpdateInsets(NULL);
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WEmbeddedFramePeer_create_stub(stack_item *sp,struct execenv *ee)
{
    Hsun_awt_windows_WComponentPeer *peer=(Hsun_awt_windows_WComponentPeer *)sp[0].p;
    if (!peer || !unhand(peer)->target) {SignalError(ee,"java/lang/NullPointerException","null peer or target");return sp;}
    EmbeddedRequest r={peer,(Hsun_awt_windows_WComponentPeer *)sp[1].p,FALSE};
    AwtToolkit::GetInstance().Invoke(createEmbedded,&r);
    if(r.failed)SignalError(ee,"java/lang/InternalError","cannot create embedded frame");
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_EmbeddedFrame_setPeer_stub(stack_item *sp,struct execenv *ee)
{
    Hjava_awt_Component *target=(Hjava_awt_Component *)sp[0].p;
    if (!target) SignalError(ee,"java/lang/NullPointerException","null embedded frame");
    else unhand(target)->peer=(Hjava_awt_peer_ComponentPeer *)sp[1].p;
    return sp;
}
extern "C" __declspec(dllexport) HObject *CreateEmbeddedFrame(HWND host)
{
    return (HObject *)execute_java_constructor(EE(),"sun/awt/windows/WEmbeddedFrame",NULL,"(I)",(long)host);
}
extern "C" __declspec(dllexport) void PrintEmbeddedFrame(HObject *frame,HDC dc)
{
    if (!frame) {SignalError(NULL,"java/lang/NullPointerException","null WEmbeddedFrame");return;}
    execute_java_dynamic_method(EE(),frame,"print","(I)V",(long)dc);
}
