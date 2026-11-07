/* Dialog creation/modal disabling recovered from 1000AFC0..1000B94F. */
#include "awt_Window.h"
#include <stdlib.h>

class AwtDialog : public AwtFrame {
    HWND *disabled;
    unsigned disabledCount,disabledCapacity;
    AwtDialog *previous;
    BOOL modal;
    static AwtDialog *current;
    static BOOL CALLBACK disableWindow(HWND window,LPARAM argument) {
        AwtDialog *dialog=(AwtDialog *)argument;
        if (window!=dialog->hwnd && !(GetWindowLong(window,GWL_STYLE)&WS_CHILD) && IsWindowEnabled(window)) {
            if (dialog->disabledCount==dialog->disabledCapacity) {
                unsigned capacity=dialog->disabledCapacity+10;
                HWND *windows=(HWND *)realloc(dialog->disabled,capacity*sizeof(HWND));
                if (!windows) return FALSE;
                dialog->disabled=windows; dialog->disabledCapacity=capacity;
            }
            dialog->disabled[dialog->disabledCount++]=window; EnableWindow(window,FALSE);
        }
        return TRUE;
    }
public:
    AwtDialog(Hsun_awt_windows_WComponentPeer *p) : AwtFrame(p), disabled(NULL), disabledCount(0), disabledCapacity(0), previous(NULL), modal(FALSE) { background=GetSysColor(COLOR_BTNFACE); }
    ~AwtDialog() { free(disabled); }
    void HideModal() {
        if (!modal || current!=this) return;
        modal=FALSE; current=previous;
        ShowWindow(hwnd,SW_HIDE);
        for (unsigned i=0;i<disabledCount;i++) if (IsWindow(disabled[i])) EnableWindow(disabled[i],TRUE);
        disabledCount=0;
        HWND activate=previous ? previous->GetHWnd():GetWindow(hwnd,GW_OWNER);
        if (activate) SetForegroundWindow(activate);
    }
    void ShowModal() {
        if (modal || IsDisposed()) return;
        previous=current; current=this; modal=TRUE;
        if (!EnumThreadWindows(GetCurrentThreadId(),disableWindow,(LPARAM)this)) {
            HideModal(); SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return;
        }
        SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)|DS_MODALFRAME);
        SetWindowLong(hwnd,GWL_EXSTYLE,GetWindowLong(hwnd,GWL_EXSTYLE)|WS_EX_DLGMODALFRAME);
        EnableWindow(hwnd,TRUE); ShowWindow(hwnd,SW_SHOW); SetForegroundWindow(hwnd);
        /* The original blocks a non-owner caller and pumps a nested owner
         * loop. Keep the native lease while hide/dispose callbacks run. */
        MSG message;
        while (modal && !IsDisposed()) {
            int result=GetMessage(&message,NULL,0,0);
            if (result<=0) { if (!result) PostQuitMessage((int)message.wParam); break; }
            TranslateMessage(&message); DispatchMessage(&message);
        }
        if (modal) HideModal();
        if (!IsDisposed()) {
            SetWindowLong(hwnd,GWL_STYLE,GetWindowLong(hwnd,GWL_STYLE)&~DS_MODALFRAME);
            SetWindowLong(hwnd,GWL_EXSTYLE,GetWindowLong(hwnd,GWL_EXSTYLE)&~WS_EX_DLGMODALFRAME);
        }
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message==WM_DESTROY) HideModal();
        return AwtFrame::WindowProc(message,w,l);
    }
};
AwtDialog *AwtDialog::current=NULL;
enum DialogOperation { DIALOG_CREATE,DIALOG_SHOW,DIALOG_HIDE };
struct DialogRequest { Hsun_awt_windows_WComponentPeer *peer,*parent; DialogOperation operation; BOOL failed; };
static void dialogRequest(void *argument)
{
    DialogRequest *r=(DialogRequest *)argument;
    if (r->operation==DIALOG_CREATE) {
        AwtComponent *parent=AwtComponent::Get(r->parent);
        if (r->parent && !parent) { r->failed=TRUE; return; }
        AwtDialog *dialog=new AwtDialog(r->peer);
        if (!dialog->Create("SunAwtDialog",0x82C80000,0x101,parent ? parent->GetHWnd():NULL)) { delete dialog; r->failed=TRUE; }
        else dialog->UpdateInsets(NULL);
        return;
    }
    AwtDialog *dialog=(AwtDialog *)AwtComponent::Get(r->peer);
    if (!dialog) { r->failed=TRUE; return; }
    AwtComponentLease lease(dialog);
    if (r->operation==DIALOG_SHOW) dialog->ShowModal(); else dialog->HideModal();
}
static void invokeDialog(DialogRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(dialogRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid dialog peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDialogPeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    Hsun_awt_windows_WComponentPeer *peer=(Hsun_awt_windows_WComponentPeer *)sp[0].p;
    BOOL defaultColor=!unhand((Hjava_awt_Component *)unhand(peer)->target)->background;
    DialogRequest r={peer,(Hsun_awt_windows_WComponentPeer *)sp[1].p,DIALOG_CREATE,FALSE};
    invokeDialog(&r,ee);
    if (!r.failed && defaultColor) execute_java_dynamic_method(ee,(HObject *)peer,"setDefaultColor","(Z)V",TRUE);
    return sp;
}
STUB(_0005fshow) {
    DialogRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,DIALOG_SHOW,FALSE};
    invokeDialog(&r,ee); return sp;
}
STUB(_0005fhide) {
    DialogRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,DIALOG_HIDE,FALSE};
    invokeDialog(&r,ee); return sp;
}
