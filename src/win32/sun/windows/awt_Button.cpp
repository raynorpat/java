/* Button creation10002435, BN_CLICKED100025B5, owner drawing100025F2. */
#include "awt_Component.h"
#include "awt_Font.h"
extern "C" {
#include "java_awt_Button.h"
}
#include <stdlib.h>

class AwtButton : public AwtComponent {
public:
    AwtButton(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) {
        foreground = GetSysColor(COLOR_BTNTEXT); background = GetSysColor(COLOR_BTNFACE);
    }
    virtual void Command(WORD code) {
        if (started && code == BN_CLICKED) PostMessage(hwnd,WM_APP+4,0,0);
    }
    virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) {
        if (message == WM_APP+4 || message == WM_APP+5) {
            if (started) {
                if (message == WM_APP+4)
                    execute_java_dynamic_method(EE(),(HObject *)peer,"handleAction","()V");
                else {
                    RECT rect; GetClientRect(hwnd,&rect);
                    execute_java_dynamic_method(EE(),(HObject *)peer,"handlePaint","(IIII)V",0,0,rect.right,rect.bottom);
                    paintPending = FALSE;
                }
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
    virtual void DrawItem(DRAWITEMSTRUCT *item) {
        HDC dc = item->hDC; int saved = SaveDC(dc);
        RECT rect = item->rcItem;
        BOOL pressed = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH brush = CreateSolidBrush(background); FillRect(dc,&rect,brush); DeleteObject(brush);
        DrawEdge(dc,&rect,pressed ? EDGE_SUNKEN : EDGE_RAISED,BF_RECT|BF_SOFT);
        SelectObject(dc,font ? font : GetStockObject(DEFAULT_GUI_FONT)); SetBkMode(dc,TRANSPARENT);
        Hjava_lang_String *text = unhand((Hjava_awt_Button *)GetTarget())->label;
        Hjava_awt_Font *javaFont = (Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)GetTarget(),"getFont","()Ljava/awt/Font;");
        if (IsDisposed()) return;
        if (text) {
            SIZE size = awtFontText(dc,javaFont,text,0,0,FALSE);
            if (IsDisposed()) return;
            IntersectClipRect(dc,rect.left,rect.top,rect.right,rect.bottom);
            int x = (rect.left+rect.right-size.cx)/2+pressed;
            int y = (rect.top+rect.bottom-size.cy)/2+pressed;
            if (!IsWindowEnabled(hwnd)) {
                SetTextColor(dc,GetSysColor(COLOR_3DHILIGHT));
                awtFontText(dc,javaFont,text,x+1,y+1,TRUE);
                if (IsDisposed()) return;
                SetTextColor(dc,GetSysColor(COLOR_GRAYTEXT));
            } else SetTextColor(dc,foreground);
            awtFontText(dc,javaFont,text,x,y,TRUE);
            if (IsDisposed()) return;
        }
        if (item->itemState & ODS_FOCUS) { InflateRect(&rect,-3,-3); DrawFocusRect(dc,&rect); }
        RestoreDC(dc,saved);
        if (started && !paintPending) { paintPending = TRUE; PostMessage(hwnd,WM_APP+5,0,0); }
    }
};
struct ButtonRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    WCHAR *text;
    BOOL create, failed;
};
static void buttonRequest(void *argument)
{
    ButtonRequest *r = (ButtonRequest *)argument;
    if (r->create) {
        AwtComponent *parent = AwtComponent::Get(r->parent);
        if (!parent) { r->failed = TRUE; return; }
        AwtButton *button = new AwtButton(r->peer);
        if (!button->CreateControl(L"BUTTON",r->text,0x4400000B,parent->GetHWnd())) {
            delete button; r->failed = TRUE;
        }
    } else {
        AwtComponent *button = AwtComponent::Get(r->peer);
        if (!button) { r->failed = TRUE; return; }
    AwtComponentLease lease(button);
        SetWindowTextW(button->GetHWnd(),r->text ? r->text : L"");
        InvalidateRect(button->GetHWnd(),NULL,TRUE);
    }
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WButtonPeer_create_stub(stack_item *sp,struct execenv *ee)
{
    Hsun_awt_windows_WComponentPeer *p = (Hsun_awt_windows_WComponentPeer *)sp[0].p;
    Hjava_awt_Button *target = (Hjava_awt_Button *)unhand(p)->target;
    WCHAR *text = awtUnicode(unhand(target)->label);
    ButtonRequest r = {p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,text,TRUE,FALSE};
    AwtToolkit::GetInstance().Invoke(buttonRequest,&r); free(text);
    if (r.failed) SignalError(ee,"java/lang/NullPointerException","invalid button parent");
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WButtonPeer_setLabel_stub(stack_item *sp,struct execenv *ee)
{
    WCHAR *text = awtUnicode((Hjava_lang_String *)sp[1].p);
    ButtonRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,text,FALSE,FALSE};
    AwtToolkit::GetInstance().Invoke(buttonRequest,&r); free(text);
    if (r.failed) SignalError(ee,"java/lang/NullPointerException","invalid button peer");
    return sp;
}
