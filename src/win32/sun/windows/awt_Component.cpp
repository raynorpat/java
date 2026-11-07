/* Reconstructed Win32 peer foundation. See Phase4 evidence for addresses. */
#include "awt_Component.h"
#include "awt_Graphics.h"
#include "awt_Font.h"
#include <stdlib.h>
#include <string.h>

WCHAR *awtUnicode(Hjava_lang_String *string)
{
    if (!string) return NULL;
    int length = javaStringLength(string);
    WCHAR *value = (WCHAR *)malloc((length + 1) * sizeof(WCHAR));
    if (value) { javaString2unicode(string, (unicode *)value, length); value[length] = 0; }
    return value;
}

AwtComponent *AwtComponent::components = NULL;
AwtComponent::AwtComponent(Hsun_awt_windows_WComponentPeer *p)
    : foreground(GetSysColor(COLOR_WINDOWTEXT)), background(GetSysColor(COLOR_WINDOW)),
      font(NULL), cursor(LoadCursor(NULL, IDC_ARROW)), started(FALSE), surface(NULL), paintPending(FALSE), hwnd(NULL), peer(p), next(NULL), originalProc(NULL) {}
AwtComponent::~AwtComponent() { if (font) DeleteObject(font); }
AwtComponent *AwtComponent::Get(Hsun_awt_windows_WComponentPeer *p)
{ return p ? (AwtComponent *)unhand(p)->pData : NULL; }

BOOL AwtComponent::Create(const char *name, DWORD style, DWORD exStyle, HWND parent)
{
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = name;
    if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return FALSE;
    Classjava_awt_Component *target = unhand(GetTarget());
    hwnd = CreateWindowExA(exStyle, name, "", style, target->x, target->y,
        target->width, target->height, parent, NULL, wc.hInstance, this);
    if (!hwnd) return FALSE;
    unhand(peer)->pData = (long)this;
    next = components; components = this;
    return TRUE;
}

void AwtComponent::Cleanup()
{ while (components) components->Dispose(); }

BOOL AwtComponent::CreateControl(const WCHAR *name, const WCHAR *text, DWORD style, HWND parent)
{
    Classjava_awt_Component *target = unhand(GetTarget());
    hwnd = CreateWindowExW(0,name,text,style,target->x,target->y,target->width,target->height,
        parent,NULL,GetModuleHandle(NULL),NULL);
    if (!hwnd) return FALSE;
    SetWindowLong(hwnd,GWL_USERDATA,(LONG)this);
    originalProc = (WNDPROC)SetWindowLongW(hwnd,GWL_WNDPROC,(LONG)WndProc);
    unhand(peer)->pData = (long)this; next = components; components = this;
    return TRUE;
}

void AwtComponent::Dispose()
{
    /* Retire on the owner thread after preceding synchronous requests. The
     * original deletes after a 500ms wait, risking a queued use-after-free. */
    started = FALSE;
    AwtComponent **link = &components;
    while (*link && *link != this) link = &(*link)->next;
    if (*link) *link = next;
    if (surface) { surface->component = NULL; surface->Release(); surface = NULL; }
    if (IsWindow(hwnd)) DestroyWindow(hwnd);
    hwnd = NULL;
    unhand(peer)->pData = 0;
    delete this;
}

LRESULT CALLBACK AwtComponent::WndProc(HWND window, UINT message, WPARAM w, LPARAM l)
{
    AwtComponent *component = (AwtComponent *)GetWindowLongA(window, GWL_USERDATA);
    if (message == WM_NCCREATE) {
        component = (AwtComponent *)((CREATESTRUCT *)l)->lpCreateParams;
        component->hwnd = window;
        SetWindowLongA(window, GWL_USERDATA, (LONG)component);
    }
    if (!component) return DefWindowProcA(window, message, w, l);
    LRESULT result = component->WindowProc(message, w, l);
    if (message == WM_NCDESTROY) {
        component->started = FALSE;
        if (component->surface) {
            component->surface->component = NULL;
            component->surface->Release(); component->surface = NULL;
        }
        component->hwnd = NULL;
        SetWindowLongA(window, GWL_USERDATA, 0);
    }
    return result;
}

LRESULT AwtComponent::WindowProc(UINT message, WPARAM w, LPARAM l)
{
    if (message == WM_COMMAND && l) {
        AwtComponent *child = (AwtComponent *)GetWindowLong((HWND)l,GWL_USERDATA);
        if (child) { child->Command(HIWORD(w)); return 0; }
    }
    if (message == WM_DRAWITEM && l) {
        DRAWITEMSTRUCT *item = (DRAWITEMSTRUCT *)l;
        AwtComponent *child = (AwtComponent *)GetWindowLong(item->hwndItem,GWL_USERDATA);
        if (child) { child->DrawItem(item); return TRUE; }
    }
    if (started && (message == WM_SETFOCUS || message == WM_KILLFOCUS)) {
        BOOL temporary = FALSE;
        if (message == WM_KILLFOCUS) {
            HWND other = (HWND)w, current = hwnd, otherRoot = other, currentRoot = current;
            for (; other && (GetWindowLong(other,GWL_STYLE)&WS_CHILD); other = GetParent(other)) otherRoot = other;
            for (; current && (GetWindowLong(current,GWL_STYLE)&WS_CHILD); current = GetParent(current)) currentRoot = current;
            temporary = currentRoot != otherRoot;
        }
        PostMessage(hwnd,WM_APP+3,message == WM_SETFOCUS ? 1004 : 1005,temporary);
    }
    if (message == WM_APP+3) {
        if (started) {
            HObject *event = (HObject *)execute_java_constructor(EE(),"java/awt/event/FocusEvent",NULL,
                "(Ljava/awt/Component;IZ)",GetTarget(),(int)w,(int)l);
            if (event && !exceptionOccurred(EE()))
                execute_java_dynamic_method(EE(),(HObject *)peer,"postEvent","(Ljava/awt/AWTEvent;)V",event);
            if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
        }
        return 0;
    }
    if (message == WM_PAINT && !originalProc) {
        RECT rect;
        if (GetUpdateRect(hwnd,&rect,FALSE)) {
            ValidateRect(hwnd,NULL);
            if (started) {
                if (paintPending) UnionRect(&pendingPaint,&pendingPaint,&rect);
                else { pendingPaint = rect; paintPending = TRUE; PostMessage(hwnd,WM_APP+1,0,0); }
            }
        }
        return 0;
    }
    if (message == WM_APP+1) {
        if (started && paintPending) {
            RECT rect = pendingPaint; paintPending = FALSE;
            /* Java coordinates of a top-level include its non-client insets. */
            if (!(GetWindowLong(hwnd,GWL_STYLE) & WS_CHILD)) {
                RECT outer; POINT origin = {0,0}; GetWindowRect(hwnd,&outer);
                ClientToScreen(hwnd,&origin); OffsetRect(&rect,origin.x-outer.left,origin.y-outer.top);
            }
            execute_java_dynamic_method(EE(),(HObject *)peer,"handleExpose","(IIII)V",
                rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top);
            if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
        }
        return 0;
    }
    if (message == WM_SETCURSOR && LOWORD(l) == HTCLIENT) { SetCursor(cursor); return TRUE; }
    if (message == WM_ERASEBKGND) {
        RECT rect; GetClientRect(hwnd, &rect);
        HBRUSH brush = CreateSolidBrush(background);
        FillRect((HDC)w, &rect, brush); DeleteObject(brush); return TRUE;
    }
    return originalProc ? CallWindowProcW(originalProc,hwnd,message,w,l) : DefWindowProcA(hwnd,message,w,l);
}

enum ComponentOperation { SHOW, HIDE, ENABLE, DISABLE, DISPOSE, RESHAPE,
    FOREGROUND, BACKGROUND, FOCUS, START, CURSOR, FONT, ZORDER, BEGIN_VALIDATE, END_VALIDATE, LOCATION };
struct ComponentRequest {
    Hsun_awt_windows_WComponentPeer *peer;
    ComponentOperation operation;
    long values[4];
    void *object;
    BOOL missing;
};
static void componentRequest(void *argument)
{
    ComponentRequest *r = (ComponentRequest *)argument;
    AwtComponent *c = AwtComponent::Get(r->peer);
    if (!c) { r->missing = TRUE; return; }
    HWND window = c->GetHWnd();
    switch (r->operation) {
    case SHOW: ShowWindow(window, SW_SHOWNORMAL); break;
    case HIDE: ShowWindow(window, SW_HIDE); break;
    case ENABLE: EnableWindow(window, TRUE); break;
    case DISABLE: EnableWindow(window, FALSE); break;
    case DISPOSE: c->Dispose(); break;
    case RESHAPE: {
        long x = r->values[0], y = r->values[1];
        HWND parent = GetParent(window);
        if ((GetWindowLong(window,GWL_STYLE) & WS_CHILD) && parent &&
            !(GetWindowLong(parent,GWL_STYLE) & WS_CHILD)) {
            RECT outer; POINT origin = {0,0}; GetWindowRect(parent,&outer);
            ClientToScreen(parent,&origin); x -= origin.x-outer.left; y -= origin.y-outer.top;
        }
        SetWindowPos(window, NULL, x, y, r->values[2], r->values[3],
                     SWP_NOACTIVATE | SWP_NOZORDER); c->UpdateInsets(NULL); break;
    }
    case FOREGROUND:
    case BACKGROUND: {
        long value = r->values[0];
        COLORREF color = RGB((value >> 16) & 255, (value >> 8) & 255, value & 255);
        if (r->operation == FOREGROUND) c->foreground = color; else c->background = color;
        InvalidateRect(window, NULL, TRUE); break;
    }
    case FOCUS: SetFocus(window); break;
    case START: c->started = TRUE; EnableWindow(window, unhand(c->GetTarget())->enabled);
        InvalidateRect(window,NULL,TRUE); break;
    case CURSOR: {
        static const WORD ids[] = {32512,32515,32513,32514,32643,32642,32642,
                                  32643,32645,32645,32644,32644,32649,32646};
        int type = r->values[0];
        c->cursor = LoadCursor(NULL, MAKEINTRESOURCE(type >= 0 && type < 14 ? ids[type] : 32512));
        if (!c->cursor) c->cursor = LoadCursor(NULL, IDC_ARROW);
        break;
    }
    case FONT: {
        HFONT font = CreateFontIndirectW((LOGFONTW *)r->object);
        if (font) { SendMessage(window, WM_SETFONT, (WPARAM)font, TRUE);
            if (c->font) DeleteObject(c->font); c->font = font; }
        break;
    }
    case ZORDER: {
        AwtComponent *after = AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)r->object);
        SetWindowPos(window, after ? after->GetHWnd() : HWND_TOP, 0,0,0,0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); break;
    }
    case BEGIN_VALIDATE: SendMessage(window, WM_SETREDRAW, FALSE, 0); break;
    case END_VALIDATE: SendMessage(window, WM_SETREDRAW, TRUE, 0); InvalidateRect(window,NULL,TRUE); break;
    case LOCATION: {
        RECT rect; GetWindowRect(window, &rect);
        r->values[0] = rect.left; r->values[1] = rect.top; break;
    }
    }
}
static void invokeComponent(ComponentRequest *r)
{
    AwtToolkit::GetInstance().Invoke(componentRequest, r);
    if (r->missing && r->operation != DISPOSE && r->operation != BEGIN_VALIDATE && r->operation != END_VALIDATE)
        SignalError(NULL, "java/lang/NullPointerException", "invalid peer");
}
#define SIMPLE_METHOD(name, op) extern "C" void sun_awt_windows_WComponentPeer_##name(Hsun_awt_windows_WComponentPeer *p) \
{ ComponentRequest r = {p, op, {0}, NULL, FALSE}; invokeComponent(&r); }
SIMPLE_METHOD(show,SHOW) SIMPLE_METHOD(hide,HIDE) SIMPLE_METHOD(enable,ENABLE)
SIMPLE_METHOD(disable,DISABLE) SIMPLE_METHOD(_dispose,DISPOSE) SIMPLE_METHOD(requestFocus,FOCUS)
SIMPLE_METHOD(start,START) SIMPLE_METHOD(_beginValidate,BEGIN_VALIDATE) SIMPLE_METHOD(endValidate,END_VALIDATE)
extern "C" void sun_awt_windows_WComponentPeer_reshape(Hsun_awt_windows_WComponentPeer *p,long x,long y,long w,long h)
{ ComponentRequest r = {p,RESHAPE,{x,y,w,h},NULL,FALSE}; invokeComponent(&r); }
extern "C" void sun_awt_windows_WComponentPeer__setForeground(Hsun_awt_windows_WComponentPeer *p,long color)
{ ComponentRequest r = {p,FOREGROUND,{color},NULL,FALSE}; invokeComponent(&r); }
extern "C" void sun_awt_windows_WComponentPeer__setBackground(Hsun_awt_windows_WComponentPeer *p,long color)
{ ComponentRequest r = {p,BACKGROUND,{color},NULL,FALSE}; invokeComponent(&r); }
extern "C" void sun_awt_windows_WComponentPeer_setCursor(Hsun_awt_windows_WComponentPeer *p,Hjava_awt_Cursor *cursor)
{
    if (!cursor) { SignalError(NULL,"java/lang/NullPointerException","null cursor"); return; }
    ComponentRequest r = {p,CURSOR,{unhand(cursor)->type},NULL,FALSE}; invokeComponent(&r);
}
extern "C" void sun_awt_windows_WComponentPeer_setFont(Hsun_awt_windows_WComponentPeer *p,Hjava_awt_Font *font)
{
    if (!font) { SignalError(NULL,"java/lang/NullPointerException","null font"); return; }
    LOGFONTW info;
    if (!awtFontInfo(font,NULL,&info)) { SignalError(NULL,"java/lang/OutOfMemoryError",NULL); return; }
    ComponentRequest r = {p,FONT,{0},&info,FALSE}; invokeComponent(&r);
}
extern "C" void sun_awt_windows_WComponentPeer_setZOrderPosition(Hsun_awt_windows_WComponentPeer *p,Hsun_awt_windows_WComponentPeer *after)
{ ComponentRequest r = {p,ZORDER,{0},after,FALSE}; invokeComponent(&r); }
extern "C" Hjava_awt_Point *sun_awt_windows_WComponentPeer_getLocationOnScreen(Hsun_awt_windows_WComponentPeer *p)
{
    ComponentRequest r = {p,LOCATION,{0},NULL,FALSE}; invokeComponent(&r);
    if (r.missing) return NULL;
    return (Hjava_awt_Point *)execute_java_constructor(EE(),"java/awt/Point",NULL,"(II)",r.values[0],r.values[1]);
}
extern "C" void sun_awt_windows_WComponentPeer_handleEvent(Hsun_awt_windows_WComponentPeer *p,Hjava_awt_AWTEvent *event)
{
    if (!event) { SignalError(NULL,"java/lang/NullPointerException","null event"); return; }
    /* Java-generated non-input events carry no saved native MSG. Input
     * message replay is implemented with the toolkit input hook. */
}
