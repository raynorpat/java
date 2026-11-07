/* Label creation1001AB45, painting1001AC91/1001AEC0, setters1001B0DB/1001B189. */
#include "awt_Component.h"
#include "awt_Font.h"
extern "C" {
#include "java_awt_Label.h"
}
#include <stdlib.h>

class AwtLabel : public AwtComponent {
public:
    AwtLabel(Hsun_awt_windows_WComponentPeer *p) : AwtComponent(p) { background = GetSysColor(COLOR_BTNFACE); }
    virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) {
        if (message == WM_PAINT) {
            PAINTSTRUCT paint; HDC dc = BeginPaint(hwnd,&paint);
            if (dc) {
                RECT rect; GetClientRect(hwnd,&rect); int saved = SaveDC(dc);
                HBRUSH brush = CreateSolidBrush(background); FillRect(dc,&rect,brush); DeleteObject(brush);
                SelectObject(dc,font ? font : GetStockObject(DEFAULT_GUI_FONT)); SetBkMode(dc,TRANSPARENT);
                Hjava_lang_String *text = unhand((Hjava_awt_Label *)GetTarget())->text;
                Hjava_awt_Font *javaFont = (Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)GetTarget(),"getFont","()Ljava/awt/Font;");
                if (IsDisposed()) return 0;
                if (text) {
                    SIZE size = awtFontText(dc,javaFont,text,0,0,FALSE);
                    if (IsDisposed()) return 0;
                    IntersectClipRect(dc,rect.left,rect.top,rect.right,rect.bottom);
                    long alignment = unhand((Hjava_awt_Label *)GetTarget())->alignment;
                    int x = alignment == 1 ? (rect.right-size.cx)/2 : alignment == 2 ? rect.right-size.cx : 0;
                    int y = (rect.bottom-size.cy)/2;
                    if (!IsWindowEnabled(hwnd)) {
                        SetTextColor(dc,GetSysColor(COLOR_3DHILIGHT));
                        awtFontText(dc,javaFont,text,x+1,y+1,TRUE);
                        if (IsDisposed()) return 0;
                        SetTextColor(dc,GetSysColor(COLOR_GRAYTEXT));
                    } else SetTextColor(dc,foreground);
                    awtFontText(dc,javaFont,text,x,y,TRUE);
                    if (IsDisposed()) return 0;
                }
                RestoreDC(dc,saved);
                if (started && !paintPending) { paintPending = TRUE; PostMessage(hwnd,WM_APP+5,0,0); }
            }
            EndPaint(hwnd,&paint); return 0;
        }
        if (message == WM_APP+5) {
            paintPending = FALSE;
            if (started) {
                RECT rect; GetClientRect(hwnd,&rect);
                execute_java_dynamic_method(EE(),(HObject *)peer,"handlePaint","(IIII)V",0,0,rect.right,rect.bottom);
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
};
enum LabelOperation { LABEL_CREATE, LABEL_TEXT, LABEL_ALIGN };
struct LabelRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    LabelOperation operation;
    WCHAR *text;
    BOOL failed;
};
static void labelRequest(void *argument)
{
    LabelRequest *r = (LabelRequest *)argument;
    if (r->operation == LABEL_CREATE) {
        AwtComponent *parent = AwtComponent::Get(r->parent);
        if (!parent) { r->failed = TRUE; return; }
        AwtLabel *label = new AwtLabel(r->peer);
        if (!label->Create("SunAwtLabel",0x44000000,0,parent->GetHWnd())) { delete label; r->failed = TRUE; }
        return;
    }
    AwtComponent *label = AwtComponent::Get(r->peer);
    if (!label) { r->failed = TRUE; return; }
    AwtComponentLease lease(label);
    if (r->operation == LABEL_TEXT) SetWindowTextW(label->GetHWnd(),r->text ? r->text : L"");
    InvalidateRect(label->GetHWnd(),NULL,TRUE);
}
static void invokeLabel(LabelRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(labelRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid label peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WLabelPeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    LabelRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,
        (Hsun_awt_windows_WComponentPeer *)sp[1].p,LABEL_CREATE,NULL,FALSE};
    invokeLabel(&r,ee); return sp;
}
STUB(setText) {
    WCHAR *text = awtUnicode((Hjava_lang_String *)sp[1].p);
    LabelRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,LABEL_TEXT,text,FALSE};
    invokeLabel(&r,ee); free(text); return sp;
}
STUB(setAlignment) {
    LabelRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,LABEL_ALIGN,NULL,FALSE};
    invokeLabel(&r,ee); return sp;
}
