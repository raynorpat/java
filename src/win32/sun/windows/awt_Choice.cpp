/* Choice contracts recovered from winawt_g10004135..10004889. */
#include "awt_ItemComponent.h"
#include <stdlib.h>

class AwtChoice : public AwtItemComponent {
public:
    AwtChoice(Hsun_awt_windows_WComponentPeer *p) : AwtItemComponent(p) {}
    virtual void Command(WORD code) {
        if (started && code == CBN_SELCHANGE) PostMessage(hwnd,WM_APP+4,SendMessage(hwnd,CB_GETCURSEL,0,0),0);
    }
    virtual LRESULT WindowProc(UINT message,WPARAM w,LPARAM l) {
        if (message == WM_APP+4) {
            if (started) {
                execute_java_dynamic_method(EE(),(HObject *)peer,"handleAction","(I)V",(int)w);
                if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
            }
            return 0;
        }
        return AwtComponent::WindowProc(message,w,l);
    }
    void Reshape(int x,int y,int width,int height) {
        int drop = execute_java_dynamic_method(EE(),(HObject *)peer,"getDropDownHeight","()I");
        RECT rect; GetWindowRect(hwnd,&rect); int closed = rect.bottom-rect.top;
        if (closed > 0 && height > closed) y += (height-closed)/2;
        HWND parent = GetParent(hwnd);
        if (parent && !(GetWindowLong(parent,GWL_STYLE)&WS_CHILD)) {
            RECT outer; POINT origin = {0,0}; GetWindowRect(parent,&outer); ClientToScreen(parent,&origin);
            x -= origin.x-outer.left; y -= origin.y-outer.top;
        }
        SetWindowPos(hwnd,NULL,x,y,width,closed+drop+3*GetSystemMetrics(SM_CYBORDER),SWP_NOACTIVATE|SWP_NOZORDER);
    }
    void Resize() { Classjava_awt_Component *t = unhand(GetTarget()); Reshape(t->x,t->y,t->width,t->height); }
};

enum ChoiceOperation { CHOICE_CREATE, CHOICE_ADD, CHOICE_REMOVE, CHOICE_SELECT, CHOICE_RESHAPE };
struct ChoiceRequest {
    Hsun_awt_windows_WComponentPeer *peer,*parent;
    ChoiceOperation operation;
    WCHAR *text;
    long values[4];
    BOOL failed;
};
static void choiceRequest(void *argument)
{
    ChoiceRequest *r = (ChoiceRequest *)argument;
    if (r->operation == CHOICE_CREATE) {
        AwtComponent *parent = AwtComponent::Get(r->parent);
        if (!parent) { r->failed = TRUE; return; }
        AwtChoice *choice = new AwtChoice(r->peer);
        if (!choice->CreateControl(L"COMBOBOX",L"",0x44200013,parent->GetHWnd())) { delete choice; r->failed = TRUE; }
        return;
    }
    AwtChoice *choice = (AwtChoice *)AwtComponent::Get(r->peer);
    if (!choice) { r->failed = TRUE; return; }
    HWND window = choice->GetHWnd();
    switch (r->operation) {
    case CHOICE_ADD: SendMessage(window,CB_INSERTSTRING,r->values[0],(LPARAM)r->text); choice->Resize(); break;
    case CHOICE_REMOVE: SendMessage(window,CB_DELETESTRING,r->values[0],0); choice->Resize(); break;
    case CHOICE_SELECT: SendMessage(window,CB_SETCURSEL,r->values[0],0); break;
    case CHOICE_RESHAPE: choice->Reshape(r->values[0],r->values[1],r->values[2],r->values[3]); break;
    }
}
static void invokeChoice(ChoiceRequest *r,struct execenv *ee)
{
    AwtToolkit::GetInstance().Invoke(choiceRequest,r);
    if (r->failed) SignalError(ee,"java/lang/NullPointerException","invalid choice peer");
}
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WChoicePeer_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(create) {
    ChoiceRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_windows_WComponentPeer *)sp[1].p,CHOICE_CREATE,NULL,{0},FALSE};
    invokeChoice(&r,ee); return sp;
}
STUB(addItem) {
    if (!sp[1].p) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    WCHAR *text = awtUnicode((Hjava_lang_String *)sp[1].p);
    ChoiceRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHOICE_ADD,text,{sp[2].i},FALSE};
    invokeChoice(&r,ee); free(text); return sp;
}
STUB(select) {
    ChoiceRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHOICE_SELECT,NULL,{sp[1].i},FALSE};
    invokeChoice(&r,ee); return sp;
}
STUB(remove) {
    ChoiceRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHOICE_REMOVE,NULL,{sp[1].i},FALSE};
    invokeChoice(&r,ee); return sp;
}
STUB(reshape) {
    ChoiceRequest r = {(Hsun_awt_windows_WComponentPeer *)sp[0].p,NULL,CHOICE_RESHAPE,NULL,{sp[1].i,sp[2].i,sp[3].i,sp[4].i},FALSE};
    invokeChoice(&r,ee); return sp;
}
