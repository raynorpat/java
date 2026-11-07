/* Window contracts recovered from winawt_g 10023E70 and 10024CCE..10025100. */
#include "awt_Window.h"
#include <stdlib.h>

void AwtWindow::UpdateInsets(Hjava_awt_Insets *extra)
{
    RECT outer, client;
    GetWindowRect(hwnd, &outer); GetClientRect(hwnd, &client);
    MapWindowPoints(hwnd, NULL, (POINT *)&client, 2);
    Hjava_awt_Insets *values[2] = {
        unhand((Hsun_awt_windows_WWindowPeer *)peer)->insets_, extra };
    for (int i = 0; i < 2; i++) if (values[i]) {
        unhand(values[i])->top = client.top - outer.top;
        unhand(values[i])->left = client.left - outer.left;
        unhand(values[i])->bottom = outer.bottom - client.bottom;
        unhand(values[i])->right = outer.right - client.right;
    }
}

enum WindowOperation { FRONT, BACK, TITLE, RESIZABLE, INSETS, CREATE };
struct WindowRequest {
    Hsun_awt_windows_WWindowPeer *peer;
    WindowOperation operation;
    void *value;
    BOOL failed;
};
static void windowRequest(void *argument)
{
    WindowRequest *r = (WindowRequest *)argument;
    AwtComponent *c = AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->peer);
    if (r->operation == CREATE) {
        AwtComponent *parent = AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->value);
        if (!parent) { r->failed = TRUE; return; }
        AwtWindow *window = new AwtWindow((Hsun_awt_windows_WComponentPeer *)r->peer);
        if (!window->Create("SunAwtWindow", 0x82000000, 0, parent->GetHWnd())) {
            delete window; r->failed = TRUE;
        } else window->UpdateInsets(NULL);
        return;
    }
    if (!c) { r->failed = TRUE; return; }
    HWND window = c->GetHWnd();
    switch (r->operation) {
    case FRONT: SetWindowPos(window, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(window); break;
    case BACK: SetWindowPos(window, HWND_BOTTOM, 0,0,0,0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); break;
    case TITLE: SetWindowTextW(window, (WCHAR *)r->value); break;
    case RESIZABLE: {
        LONG style = GetWindowLong(window, GWL_STYLE);
        SetWindowLong(window, GWL_STYLE, r->value ? style | WS_THICKFRAME : style & ~WS_THICKFRAME);
        SetWindowPos(window, NULL,0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        c->UpdateInsets(NULL); break;
    }
    case INSETS: c->UpdateInsets((Hjava_awt_Insets *)r->value); break;
    }
}
static void invokeWindow(WindowRequest *r)
{
    AwtToolkit::GetInstance().Invoke(windowRequest, r);
    if (r->failed) SignalError(NULL,"java/lang/NullPointerException","invalid peer");
}
extern "C" void sun_awt_windows_WWindowPeer_toFront(Hsun_awt_windows_WWindowPeer *p)
{ WindowRequest r = {p,FRONT,NULL,FALSE}; invokeWindow(&r); }
extern "C" void sun_awt_windows_WWindowPeer_toBack(Hsun_awt_windows_WWindowPeer *p)
{ WindowRequest r = {p,BACK,NULL,FALSE}; invokeWindow(&r); }
extern "C" void sun_awt_windows_WWindowPeer__setTitle(Hsun_awt_windows_WWindowPeer *p,Hjava_lang_String *s)
{
    if (!s) { SignalError(NULL,"java/lang/NullPointerException","null title"); return; }
    WCHAR *text = awtUnicode(s);
    if (!text) { SignalError(NULL,"java/lang/OutOfMemoryError",NULL); return; }
    WindowRequest r = {p,TITLE,text,FALSE}; invokeWindow(&r); free(text);
}
extern "C" void sun_awt_windows_WWindowPeer__setResizable(Hsun_awt_windows_WWindowPeer *p,long value)
{ WindowRequest r = {p,RESIZABLE,(void *)value,FALSE}; invokeWindow(&r); }
extern "C" void sun_awt_windows_WWindowPeer_updateInsets(Hsun_awt_windows_WWindowPeer *p,Hjava_awt_Insets *value)
{ WindowRequest r = {p,INSETS,value,FALSE}; invokeWindow(&r); }
extern "C" void sun_awt_windows_WWindowPeer_create(Hsun_awt_windows_WWindowPeer *p,Hsun_awt_windows_WComponentPeer *parent)
{ WindowRequest r = {p,CREATE,parent,FALSE}; invokeWindow(&r); }
extern "C" Hjava_awt_Component *sun_awt_windows_WWindowPeer_getContainerElement(
    Hsun_awt_windows_WWindowPeer *,Hjava_awt_Container *container,long index)
{
    if (!container) { SignalError(NULL,"java/lang/NullPointerException",NULL); return NULL; }
    if (!unhand(container)->ncomponents) return NULL;
    if (index < 0 || index >= unhand(container)->ncomponents) {
        SignalError(NULL,"java/lang/ArrayIndexOutOfBoundsException",NULL); return NULL;
    }
    return (Hjava_awt_Component *)unhand(unhand(container)->component)->body[index];
}
