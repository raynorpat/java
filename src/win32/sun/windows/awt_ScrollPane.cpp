/* ScrollPane recovered from winawt_g1001F675..100207C2. */
#include "awt_Component.h"
extern "C" {
#include "sun_awt_windows_WScrollPanePeer.h"
#include "java_awt_ScrollPane.h"
#include "java_awt_ScrollPaneAdjustable.h"
#include "java_awt_Insets.h"
}
class AwtScrollPane : public AwtComponent {
public:
    AwtScrollPane(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) {}
    virtual void UpdateInsets(Hjava_awt_Insets *) {
        Hjava_awt_Insets *insets=unhand((Hsun_awt_windows_WScrollPanePeer *)peer)->insets_;
        if (!insets) return;
        RECT outer,client; GetWindowRect(hwnd,&outer); GetClientRect(hwnd,&client);
        MapWindowPoints(hwnd,NULL,(POINT *)&client,2);
        unhand(insets)->top=client.top-outer.top; unhand(insets)->left=client.left-outer.left;
        unhand(insets)->bottom=outer.bottom-client.bottom; unhand(insets)->right=outer.right-client.right;
    }
    void Position(int x,int y) {
        int dx=GetScrollPos(hwnd,SB_HORZ)-x,dy=GetScrollPos(hwnd,SB_VERT)-y;
        SetScrollPos(hwnd,SB_HORZ,x,TRUE); SetScrollPos(hwnd,SB_VERT,y,TRUE);
        ScrollWindow(hwnd,dx,dy,NULL,NULL); UpdateWindow(hwnd);
    }
    void Span(int axis,int maximum,int page,BOOL always) {
        SCROLLINFO *info=new SCROLLINFO;
        ZeroMemory(info,sizeof(*info)); info->cbSize=sizeof(*info); info->nMax=maximum; info->fMask=SIF_RANGE;
        if (!maximum && always) info->fMask|=SIF_DISABLENOSCROLL;
        if (page>0) { info->fMask|=SIF_PAGE; info->nPage=page; }
        if (!PostMessage(hwnd,WM_APP+6,axis,(LPARAM)info)) delete info;
    }
    void Spans(int width,int height,int childWidth,int childHeight) {
        int viewportWidth=width-2*GetSystemMetrics(SM_CXEDGE),viewportHeight=height-2*GetSystemMetrics(SM_CYEDGE);
        Classjava_awt_ScrollPane *target=unhand((Hjava_awt_ScrollPane *)GetTarget());
        int policy=target->scrollbarDisplayPolicy;
        BOOL horizontal=policy==1 || (policy==0 && childWidth>viewportWidth);
        if (horizontal) viewportHeight-=GetSystemMetrics(SM_CYHSCROLL);
        BOOL vertical=policy==1 || (policy==0 && childHeight>viewportHeight);
        if (vertical) viewportWidth-=GetSystemMetrics(SM_CXVSCROLL);
        if (!horizontal) {
            horizontal=policy==1 || (policy==0 && childWidth>viewportWidth);
            if (horizontal) viewportHeight-=GetSystemMetrics(SM_CYHSCROLL);
        }
        if (horizontal) unhand(target->hAdjustable)->blockIncrement=viewportWidth;
        if (vertical) unhand(target->vAdjustable)->blockIncrement=viewportHeight;
        Span(SB_HORZ,horizontal ? childWidth-1:0,horizontal ? viewportWidth:0,policy==1);
        Span(SB_VERT,vertical ? childHeight-1:0,vertical ? viewportHeight:0,policy==1);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message==WM_APP+6) {
            SCROLLINFO *info=(SCROLLINFO *)l;
            SetScrollInfo(hwnd,(int)w,info,TRUE); delete info; return 0;
        }
        if ((message==WM_HSCROLL || message==WM_VSCROLL) && !l) {
            PostMessage(hwnd,WM_APP+7,w,message==WM_VSCROLL); return 0;
        }
        if (message==WM_APP+7 && started) {
            int axis=(int)l,low,high; GetScrollRange(hwnd,axis,&low,&high);
            RECT client; GetClientRect(hwnd,&client); high-=axis ? client.bottom:client.right;
            int old=GetScrollPos(hwnd,axis),value=old;
            Classjava_awt_ScrollPane *target=unhand((Hjava_awt_ScrollPane *)GetTarget());
            Classjava_awt_ScrollPaneAdjustable *adjustable=unhand(axis ? target->vAdjustable:target->hAdjustable);
            switch (LOWORD(w)) {
            case SB_LINEUP: value=old-adjustable->unitIncrement; if (value<low) value=low; break;
            case SB_LINEDOWN: value=old+adjustable->unitIncrement; if (value>high) value=high; break;
            case SB_PAGEUP: value=old-adjustable->blockIncrement; if (value<low) value=low; break;
            case SB_PAGEDOWN: value=old+adjustable->blockIncrement; if (value>high) value=high; break;
            case SB_THUMBPOSITION: case SB_THUMBTRACK: value=HIWORD(w); break;
            case SB_TOP: value=low; break;
            case SB_BOTTOM: value=high; break;
            }
            if (old!=value) {
                SetScrollPos(hwnd,axis,value,TRUE);
                ScrollWindow(hwnd,axis ? 0:old-value,axis ? old-value:0,NULL,NULL); UpdateWindow(hwnd);
                execute_java_dynamic_method(EE(),(HObject *)peer,axis ? "scrolledVertical":"scrolledHorizontal","(I)V",value);
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        if (message==WM_NCDESTROY) {
            MSG pending;
            while (PeekMessage(&pending,hwnd,WM_APP+6,WM_APP+6,PM_REMOVE)) delete (SCROLLINFO *)pending.lParam;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
};
enum PaneOperation { PANE_CREATE,PANE_OFFSET,PANE_INSETS,PANE_POSITION,PANE_SPANS };
struct PaneRequest { Hsun_awt_windows_WComponentPeer *peer,*parent; PaneOperation operation; long v[4]; BOOL failed; };
static void paneRequest(void *argument)
{
    PaneRequest *r=(PaneRequest *)argument;
    if (r->operation==PANE_CREATE) {
        AwtComponent *parent=AwtComponent::Get(r->parent);
        if (!parent) { r->failed=TRUE; return; }
        AwtScrollPane *pane=new AwtScrollPane(r->peer);
        int policy=unhand((Hjava_awt_ScrollPane *)pane->GetTarget())->scrollbarDisplayPolicy;
        DWORD style=0x46000000|(policy==0 ? WS_HSCROLL|WS_VSCROLL:0);
        if (!pane->Create("SunAwtScrollPane",style,WS_EX_CLIENTEDGE,parent->GetHWnd())) { delete pane; r->failed=TRUE; }
        return;
    }
    AwtScrollPane *pane=(AwtScrollPane *)AwtComponent::Get(r->peer);
    if (!pane) { r->failed=TRUE; return; }
    AwtComponentLease lease(pane);
    switch (r->operation) {
    case PANE_OFFSET: r->v[0]=GetScrollPos(pane->GetHWnd(),r->v[0] ? SB_VERT:SB_HORZ); break;
    case PANE_INSETS: pane->UpdateInsets(NULL); break;
    case PANE_POSITION: pane->Position(r->v[0],r->v[1]); break;
    case PANE_SPANS: pane->Spans(r->v[0],r->v[1],r->v[2],r->v[3]); break;
    }
}
static void invokePane(PaneRequest *r,struct execenv *ee) {
    /* Sun reads insets and queues span updates on the calling Java thread.
     * An extra owner-thread round trip can dispatch one scrollbar update
     * between the horizontal/vertical posts and the Java inset read. */
    if (r->operation==PANE_INSETS || r->operation==PANE_SPANS) {
        AwtComponentLock lock;
        paneRequest(r);
    }
    else AwtToolkit::GetInstance().Invoke(paneRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid scroll pane peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WScrollPanePeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    PaneRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,PANE_CREATE};
    invokePane(&r,ee); return sp;
}
STUB(getOffset) {
    PaneRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,PANE_OFFSET,{sp[1].i}};
    invokePane(&r,ee); sp[0].i=r.v[0]; return sp+1;
}
STUB(setInsets) {
    PaneRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,PANE_INSETS};
    invokePane(&r,ee); return sp;
}
STUB(setScrollPosition) {
    PaneRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,PANE_POSITION,{sp[1].i,sp[2].i}};
    invokePane(&r,ee); return sp;
}
STUB(setSpans) {
    PaneRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,PANE_SPANS,{sp[1].i,sp[2].i,sp[3].i,sp[4].i}};
    invokePane(&r,ee); return sp;
}
STUB(_0005fgetHScrollbarHeight) { sp[0].i=GetSystemMetrics(SM_CYHSCROLL); return sp+1; }
STUB(_0005fgetVScrollbarWidth) { sp[0].i=GetSystemMetrics(SM_CXVSCROLL); return sp+1; }
STUB(getScrollChild) {
    Hsun_awt_windows_WComponentPeer *peer=(Hsun_awt_windows_WComponentPeer *)sp[0].p;
    sp[0].p=NULL;
    if (!peer || !AwtComponent::Get(peer)) SignalError(ee,"java/lang/NullPointerException","invalid scroll pane peer");
    else {
        Classjava_awt_ScrollPane *target=unhand((Hjava_awt_ScrollPane *)unhand(peer)->target);
        if (target->ncomponents && target->component) sp[0].p=unhand(target->component)->body[0];
        else SignalError(ee,"java/lang/NullPointerException","child is null");
    }
    return sp+1;
}
