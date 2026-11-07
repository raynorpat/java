/* Scrollbar peer recovered from winawt_g1001EE93..1001F600. */
#include "awt_Component.h"
extern "C" {
#include "java_awt_Scrollbar.h"
}
class AwtScrollbar : public AwtComponent {
public:
    int line,page;
    AwtScrollbar(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) {
        Classjava_awt_Scrollbar *target=unhand((Hjava_awt_Scrollbar *)GetTarget());
        line=target->lineIncrement; page=target->pageIncrement;
        foreground=background=GetSysColor(COLOR_SCROLLBAR);
    }
    virtual void Scroll(WORD code,WORD position) {
        if (started) PostMessage(hwnd,WM_APP+4,code,position);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message == WM_APP+4) {
            if (!started) return 0;
            int low,high; GetScrollRange(hwnd,SB_CTL,&low,&high);
            int old=GetScrollPos(hwnd,SB_CTL), value=old;
            char *method=NULL;
            switch (w) {
            case SB_LINEUP: value=old-line; if (value<low) value=low; method="lineUp"; break;
            case SB_LINEDOWN: value=old+line; if (value>high) value=high; method="lineDown"; break;
            case SB_PAGEUP: value=old-page; if (value<low) value=low; method="pageUp"; break;
            case SB_PAGEDOWN: value=old+page; if (value>high) value=high; method="pageDown"; break;
            case SB_THUMBPOSITION: case SB_THUMBTRACK: case SB_TOP: case SB_BOTTOM:
                value=(int)l; method="dragAbsolute"; break;
            case SB_ENDSCROLL: value=(int)l; method="dragEnd"; break;
            case 9: value=(int)l; method="dragBegin"; break;
            }
            if (method && (w>SB_PAGEDOWN || old!=value)) {
                execute_java_dynamic_method(EE(),(HObject *)peer,method,"(I)V",value);
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
};
enum ScrollbarOperation { SCROLL_CREATE,SCROLL_VALUES,SCROLL_LINE,SCROLL_PAGE };
struct ScrollbarRequest {
    Hsun_awt_windows_WComponentPeer *peer,*parent;
    ScrollbarOperation operation;
    long values[4];
    BOOL failed;
};
static void scrollbarRequest(void *argument)
{
    ScrollbarRequest *r=(ScrollbarRequest *)argument;
    if (r->operation==SCROLL_CREATE) {
        AwtComponent *parent=AwtComponent::Get(r->parent);
        if (!parent) { r->failed=TRUE; return; }
        AwtScrollbar *bar=new AwtScrollbar(r->peer);
        Classjava_awt_Scrollbar *target=unhand((Hjava_awt_Scrollbar *)bar->GetTarget());
        if (!bar->CreateControl(L"SCROLLBAR",L"",0x44000000|(target->orientation==1 ? SBS_VERT:0),parent->GetHWnd())) {
            delete bar; r->failed=TRUE;
        }
        return;
    }
    AwtScrollbar *bar=(AwtScrollbar *)AwtComponent::Get(r->peer);
    if (!bar) { r->failed=TRUE; return; }
    AwtComponentLease lease(bar);
    if (r->operation==SCROLL_LINE) bar->line=r->values[0];
    if (r->operation==SCROLL_PAGE) bar->page=r->values[0];
    if (r->operation==SCROLL_VALUES) {
        SCROLLINFO info={sizeof(info),SIF_ALL};
        info.nPos=r->values[0]; info.nPage=r->values[1]; info.nMin=r->values[2]; info.nMax=r->values[3]-1;
        SetScrollInfo(bar->GetHWnd(),SB_CTL,&info,TRUE);
    }
}
static void invokeScrollbar(ScrollbarRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(scrollbarRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid scrollbar peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WScrollbarPeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    ScrollbarRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,SCROLL_CREATE};
    invokeScrollbar(&r,ee); return sp;
}
STUB(_0005fsetValues) {
    ScrollbarRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,SCROLL_VALUES,{sp[1].i,sp[2].i,sp[3].i,sp[4].i}};
    invokeScrollbar(&r,ee); return sp;
}
STUB(setLineIncrement) {
    ScrollbarRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,SCROLL_LINE,{sp[1].i}};
    invokeScrollbar(&r,ee); return sp;
}
STUB(setPageIncrement) {
    ScrollbarRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,SCROLL_PAGE,{sp[1].i}};
    invokeScrollbar(&r,ee); return sp;
}
