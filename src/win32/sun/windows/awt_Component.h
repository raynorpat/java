/* Component ownership recovered from winawt_g, awt_Component.cpp. */
#ifndef AWT_COMPONENT_H
#define AWT_COMPONENT_H
#include "awt_Toolkit.h"
extern "C" {
#include "sun_awt_windows_WComponentPeer.h"
#include "java_awt_Component.h"
#include "java_awt_Font.h"
#include "java_awt_Cursor.h"
#include "java_awt_AWTEvent.h"
}
struct Hjava_awt_Insets;
struct AwtSurface;
class AwtComponent {
public:
    AwtComponent(Hsun_awt_windows_WComponentPeer *peer);
    virtual ~AwtComponent();
    BOOL Create(const char *name, DWORD style, DWORD exStyle, HWND parent);
    BOOL CreateControl(const WCHAR *name, const WCHAR *text, DWORD style, HWND parent, DWORD exStyle = 0);
    virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l);
    virtual void Command(WORD code) {}
    virtual void Scroll(WORD code,WORD position) {}
    virtual void DrawItem(DRAWITEMSTRUCT *item) {}
    virtual void MeasureItem(MEASUREITEMSTRUCT *item) {}
    virtual void UpdateInsets(Hjava_awt_Insets *insets) {}
    void Dispose();
    BOOL InputMessage(UINT message, WPARAM w, LPARAM l);
    void ClearInput();
    void ReplayInput(Hjava_awt_AWTEvent *event);
    void Retain() { InterlockedIncrement(&references); }
    void Release() { if (!InterlockedDecrement(&references)) delete this; }
    BOOL IsDisposed() const { return disposed; }
    static void LockPeers();
    static void UnlockPeers();
    static void Cleanup();
    HWND GetHWnd() const { return hwnd; }
    Hsun_awt_windows_WComponentPeer *GetPeer() const { return peer; }
    Hjava_awt_Component *GetTarget() const { return (Hjava_awt_Component *)unhand(peer)->target; }
    static AwtComponent *Get(Hsun_awt_windows_WComponentPeer *peer);
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    COLORREF foreground, background;
    HFONT font;
    HCURSOR cursor;
    BOOL started;
    AwtSurface *surface;
    RECT pendingPaint;
    BOOL paintPending;
    BOOL mouseInside, mouseDragged, suppressTyped;
    DWORD lastClick;
    POINT clickPoint;
    int clickCount;
protected:
    void Unlink();
    HWND hwnd;
    Hsun_awt_windows_WComponentPeer *peer;
    AwtComponent *next;
    static AwtComponent *components;
    WNDPROC originalProc;
    UINT controlID;
    static UINT nextControlID;
    LONG references;
    BOOL disposed;
};
/* Native callbacks can run Java that removes this peer. Keep both the C++
 * object and its Java handle alive until the complete native use ends. */
class AwtComponentLease {
    AwtComponent *component;
    HObject *volatile javaPeer;
public:
    AwtComponentLease(AwtComponent *c) : component(c), javaPeer(c ? (HObject *)c->GetPeer() : NULL) { if (c) c->Retain(); }
    ~AwtComponentLease() { if (javaPeer && component) component->Release(); }
private:
    AwtComponentLease(const AwtComponentLease &);
    AwtComponentLease &operator=(const AwtComponentLease &);
};
class AwtComponentLock {
public:
    AwtComponentLock() { AwtComponent::LockPeers(); }
    ~AwtComponentLock() { AwtComponent::UnlockPeers(); }
private:
    AwtComponentLock(const AwtComponentLock &);
    AwtComponentLock &operator=(const AwtComponentLock &);
};
WCHAR *awtUnicode(Hjava_lang_String *string);
#endif
