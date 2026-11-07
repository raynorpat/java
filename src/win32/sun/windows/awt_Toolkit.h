/* Recovered toolkit ownership; private C++ implementation, not a Java layout. */
#ifndef AWT_TOOLKIT_H
#define AWT_TOOLKIT_H
#include <windows.h>
extern "C" {
#include "StubPreamble.h"
#include "sys_api.h"
#include "exceptions.h"
}
class AwtToolkit {
public:
    static AwtToolkit &GetInstance();
    BOOL Initialize(BOOL standalone);
    void EventLoop();
    void Cleanup();
    void Invoke(void (*function)(void *), void *argument);
    HWND GetHWnd() const { return window; }
    DWORD MainThread() const { return thread; }
    DWORD CallerThread() const { return callerThread ? callerThread:thread; }
    BOOL EnsureClipboardRoot();
    void SetClipboardOwner(HObject *owner);
    HObject *TakeClipboardOwner();
private:
    AwtToolkit() : thread(0), callerThread(0), window(NULL), hook(NULL), clipboardRoot(NULL), quit(FALSE), exitCode(0) {}
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK GetMessageHook(int, WPARAM, LPARAM);
    DWORD thread, callerThread;
    HWND window;
    HHOOK hook;
    HObject *volatile *clipboardRoot;
    BOOL quit;
    int exitCode;
};
#endif
