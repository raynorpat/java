/* Checkbox peer recovered from winawt_g1000344F..10003EEB. */
#include "awt_Component.h"
extern "C" {
#include "java_awt_Checkbox.h"
}
#include <stdlib.h>

class AwtCheckbox : public AwtComponent {
public:
    AwtCheckbox(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) { background = GetSysColor(COLOR_BTNFACE); }
    virtual void Command(WORD code) {
        if (started && code == BN_CLICKED) PostMessage(hwnd,WM_APP+4,0,0);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message == WM_APP+4 || message == WM_APP+5) {
            if (started) {
                if (message == WM_APP+4) {
                    Hjava_awt_Checkbox *target = (Hjava_awt_Checkbox *)GetTarget();
                    execute_java_dynamic_method(EE(),(HObject *)peer,"handleAction","(Z)V",!unhand(target)->state);
                } else {
                    RECT rect; GetClientRect(hwnd,&rect); paintPending = FALSE;
                    execute_java_dynamic_method(EE(),(HObject *)peer,"handlePaint","(IIII)V",0,0,rect.right,rect.bottom);
                }
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
    virtual void DrawItem(DRAWITEMSTRUCT *item) {
        Classjava_awt_Checkbox *target = unhand((Hjava_awt_Checkbox *)GetTarget());
        HDC dc = item->hDC; int saved = SaveDC(dc); RECT rect = item->rcItem;
        HBRUSH brush = CreateSolidBrush(background); FillRect(dc,&rect,brush); DeleteObject(brush);
        int extent = GetSystemMetrics(SM_CYMENUCHECK);
        RECT check = {rect.left,(rect.bottom-rect.top-extent)/2,rect.left+extent,(rect.bottom-rect.top+extent)/2};
        UINT state = target->group ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
        if (target->state) state |= DFCS_CHECKED;
        if (item->itemState & ODS_SELECTED) state |= DFCS_PUSHED;
        DrawFrameControl(dc,&check,DFC_BUTTON,state);
        rect.left += extent + extent/4;
        SelectObject(dc,font ? font : GetStockObject(DEFAULT_GUI_FONT)); SetBkMode(dc,TRANSPARENT);
        int length = GetWindowTextLengthW(hwnd); WCHAR *text = (WCHAR *)malloc((length+1)*sizeof(WCHAR));
        if (text) {
            GetWindowTextW(hwnd,text,length+1); SIZE size; GetTextExtentPoint32W(dc,text,length,&size);
            int y = (rect.bottom+rect.top-size.cy)/2;
            if (!IsWindowEnabled(hwnd)) {
                SetTextColor(dc,GetSysColor(COLOR_3DHILIGHT));
                ExtTextOutW(dc,rect.left+1,y+1,ETO_CLIPPED,&rect,text,length,NULL);
                SetTextColor(dc,GetSysColor(COLOR_GRAYTEXT));
            } else SetTextColor(dc,foreground);
            ExtTextOutW(dc,rect.left,y,ETO_CLIPPED,&rect,text,length,NULL);
            if (item->itemState & ODS_FOCUS) {
                RECT focus = {rect.left-2,y,rect.left+size.cx+2,y+size.cy}; DrawFocusRect(dc,&focus);
            }
            free(text);
        }
        RestoreDC(dc,saved);
        if (started && !paintPending) { paintPending = TRUE; PostMessage(hwnd,WM_APP+5,0,0); }
    }
};
enum CheckboxOperation { CHECK_CREATE, CHECK_STATE, CHECK_GROUP, CHECK_LABEL };
struct CheckboxRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    CheckboxOperation operation;
    WCHAR *text;
    long state;
    BOOL failed;
};
static void checkboxRequest(void *argument)
{
    CheckboxRequest *r = (CheckboxRequest *)argument;
    if (r->operation == CHECK_CREATE) {
        AwtComponent *parent = AwtComponent::Get(r->parent);
        if (!parent) { r->failed = TRUE; return; }
        AwtCheckbox *checkbox = new AwtCheckbox(r->peer);
        if (!checkbox->CreateControl(L"BUTTON",r->text,0x4400000B,parent->GetHWnd())) { delete checkbox; r->failed = TRUE; }
        return;
    }
    AwtComponent *checkbox = AwtComponent::Get(r->peer);
    if (!checkbox) { r->failed = TRUE; return; }
    HWND window = checkbox->GetHWnd();
    if (r->operation == CHECK_LABEL) SetWindowTextW(window,r->text ? r->text : L"");
    if (r->operation == CHECK_GROUP) SendMessage(window,BM_SETSTYLE,BS_OWNERDRAW,TRUE);
    if (r->operation == CHECK_STATE) SendMessage(window,BM_SETCHECK,r->state != 0,0);
    InvalidateRect(window,NULL,TRUE);
}
static void invokeCheckbox(CheckboxRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(checkboxRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid checkbox peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WCheckboxPeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    Hsun_awt_windows_WComponentPeer *p = (Hsun_awt_windows_WComponentPeer *)sp[0].p;
    WCHAR *text = awtUnicode(unhand((Hjava_awt_Checkbox *)unhand(p)->target)->label);
    CheckboxRequest r = {p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,CHECK_CREATE,text,0,FALSE};
    invokeCheckbox(&r,ee); free(text); return sp;
}
STUB(setLabel) {
    WCHAR *text = awtUnicode((Hjava_lang_String *)sp[1].p);
    CheckboxRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHECK_LABEL,text,0,FALSE};
    invokeCheckbox(&r,ee); free(text); return sp;
}
STUB(setState) {
    CheckboxRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHECK_STATE,NULL,sp[1].i,FALSE};
    invokeCheckbox(&r,ee); return sp;
}
STUB(setCheckboxGroup) {
    CheckboxRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHECK_GROUP,NULL,0,FALSE};
    invokeCheckbox(&r,ee); return sp;
}
