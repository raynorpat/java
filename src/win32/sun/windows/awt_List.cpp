/* List peer recovered from winawt_g1001B270..1001BEC0. */
#include "awt_ItemComponent.h"

class AwtList : public AwtItemComponent {
public:
    BOOL multiple;
    int maximumWidth;
    AwtList(Hsun_awt_windows_WComponentPeer *p) : AwtItemComponent(p), multiple(FALSE), maximumWidth(0) {}
    virtual void Command(WORD code) {
        if (started && (code == LBN_SELCHANGE || code == LBN_DBLCLK))
            PostMessage(hwnd,WM_APP+4,SendMessage(hwnd,LB_GETCURSEL,0,0),code);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message == WM_APP+4) {
            if (started) {
                execute_java_dynamic_method(EE(),(HObject *)peer,l == LBN_SELCHANGE ? "handleListChanged" : "handleAction","(I)V",(int)w);
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtItemComponent::WindowProc(message,w,l);
    }
    void Extent() {
        RECT rect; GetClientRect(hwnd,&rect);
        if (maximumWidth > rect.right) SendMessage(hwnd,LB_SETHORIZONTALEXTENT,maximumWidth,0);
    }
    BOOL Multiple(BOOL value) {
        if (multiple == value) return TRUE;
        int count = (int)SendMessage(hwnd,LB_GETCOUNT,0,0);
        int selected = (int)SendMessage(hwnd,LB_GETCURSEL,0,0);
        RECT rect; GetWindowRect(hwnd,&rect); HWND parent = GetParent(hwnd);
        MapWindowPoints(NULL,parent,(POINT *)&rect,2);
        DWORD style = GetWindowLong(hwnd,GWL_STYLE)|WS_HSCROLL;
        style = value ? style|LBS_MULTIPLESEL : style&~LBS_MULTIPLESEL;
        DWORD exStyle = GetWindowLong(hwnd,GWL_EXSTYLE);
        BOOL wasStarted = started; multiple = value;
        /* Native list selection mode is fixed at creation. Retire the old
         * HWND and rebuild the items, as 1001B467 does. Item data is unused:
         * shared owner drawing obtains text from target.getItem(index). */
        DestroyWindow(hwnd);
        Unlink();
        if (!CreateControl(L"LISTBOX",L"",style,parent,exStyle)) return FALSE;
        SetWindowPos(hwnd,NULL,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessage(hwnd,WM_SETFONT,(WPARAM)font,FALSE);
        for (int i=0;i<count;i++) SendMessage(hwnd,LB_INSERTSTRING,i,0);
        Extent(); SendMessage(hwnd,LB_SETCURSEL,selected,0); started = wasStarted;
        return TRUE;
    }
};
enum ListOperation { LIST_CREATE, LIST_ADD, LIST_DELETE, LIST_SELECT, LIST_DESELECT, LIST_VISIBLE, LIST_MULTIPLE, LIST_SELECTED };
struct ListRequest {
    Hsun_awt_windows_WComponentPeer *peer,*parent;
    ListOperation operation;
    WCHAR *text;
    long first,last,width;
    BOOL result,failed;
};
static void listRequest(void *argument)
{
    ListRequest *r = (ListRequest *)argument;
    if (r->operation == LIST_CREATE) {
        AwtComponent *parent = AwtComponent::Get(r->parent);
        if (!parent) { r->failed = TRUE; return; }
        AwtList *list = new AwtList(r->peer);
        if (!list->CreateControl(L"LISTBOX",L"",0x44300121,parent->GetHWnd(),WS_EX_CLIENTEDGE)) { delete list; r->failed = TRUE; }
        return;
    }
    AwtList *list = (AwtList *)AwtComponent::Get(r->peer);
    if (!list) { r->failed = TRUE; return; }
    AwtComponentLease lease(list);
    HWND window = list->GetHWnd();
    switch (r->operation) {
    case LIST_ADD:
        SendMessage(window,LB_INSERTSTRING,r->first,(LPARAM)r->text);
        if (list->maximumWidth < r->width) { list->maximumWidth = r->width; list->Extent(); } break;
    case LIST_DELETE:
        if (r->first == 0 && r->last == SendMessage(window,LB_GETCOUNT,0,0)-1) SendMessage(window,LB_RESETCONTENT,0,0);
        else for (long i=r->first;i<=r->last;i++) SendMessage(window,LB_DELETESTRING,r->first,0);
        break;
    case LIST_SELECT:
        if (list->multiple) SendMessage(window,LB_SETSEL,TRUE,r->first);
        else SendMessage(window,LB_SETCURSEL,r->first,0); break;
    case LIST_DESELECT:
        if (list->multiple) SendMessage(window,LB_SETSEL,FALSE,r->first);
        else SendMessage(window,LB_SETCURSEL,-1,0); break;
    case LIST_VISIBLE: SendMessage(window,LB_SETTOPINDEX,r->first,0); break;
    case LIST_MULTIPLE: r->failed = !list->Multiple(r->first != 0); break;
    case LIST_SELECTED: r->result = SendMessage(window,LB_GETSEL,r->first,0)>0; break;
    }
}
static void invokeList(ListRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(listRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid list peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WListPeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    ListRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,LIST_CREATE};
    invokeList(&r,ee); return sp;
}
STUB(_0005faddItem) {
    if (!sp[1].p) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    WCHAR *text = awtUnicode((Hjava_lang_String *)sp[1].p);
    ListRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,LIST_ADD,text,sp[2].i,0,sp[3].i};
    invokeList(&r,ee); free(text); return sp;
}
STUB(delItems) {
    ListRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,LIST_DELETE,NULL,sp[1].i,sp[2].i};
    invokeList(&r,ee); return sp;
}
#define LIST_STUB(name,operation) STUB(name) { \
    ListRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,operation,NULL,sp[1].i}; \
    invokeList(&r,ee); return sp; }
LIST_STUB(select,LIST_SELECT)
LIST_STUB(deselect,LIST_DESELECT)
LIST_STUB(makeVisible,LIST_VISIBLE)
LIST_STUB(setMultipleSelections,LIST_MULTIPLE)
STUB(isSelected) {
    ListRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,LIST_SELECTED,NULL,sp[1].i};
    invokeList(&r,ee); sp[0].i = r.result; return sp+1;
}
