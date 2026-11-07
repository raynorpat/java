/* WDrawingSurfaceInfo: winawt_g 1000BC50..1000BF55. */
#include "awt_Image.h"
extern "C" {
#include "sun_awt_windows_WDrawingSurfaceInfo.h"
}
struct DrawingLock {
    AwtSurface *surface;
    HDC dc;
    HWND window;
    DWORD thread;
    int saved;
};
struct DrawingRequest {
    Hsun_awt_windows_WDrawingSurfaceInfo *peer;
    DrawingLock *lock;
    int operation;
    long result;
    char *error;
};
static void drawingRequest(void *argument)
{
    DrawingRequest *r=(DrawingRequest *)argument;
    Classsun_awt_windows_WDrawingSurfaceInfo *info=unhand(r->peer);
    if (r->operation==1) {
        if (r->lock->window) ReleaseDC(r->lock->window,r->lock->dc);
        r->lock->surface->Release(); delete r->lock; return;
    }
    if (r->operation==2 || r->operation==3) {
        AwtSurface *surface=info->imgrep ? awtImageSurface(info->imgrep,TRUE):NULL;
        if (surface) r->result=r->operation==2 ? (long)surface->bitmap:
            unhand(info->imgrep)->offscreen ? 0:(long)surface->bits;
        return;
    }
    if (info->pData) { r->error="drawing surface already locked"; return; }
    AwtSurface *surface=NULL; HWND window=NULL; HDC dc=NULL;
    if (info->peer) {
        AwtComponent *component=AwtComponent::Get(info->peer);
        if (!component || !IsWindow(component->GetHWnd())) { r->error="disposed component"; return; }
        Classjava_awt_Component *target=unhand(component->GetTarget());
        if (target->width!=info->w || target->height!=info->h) {
            info->w=target->width; info->h=target->height; ++info->state;
        }
        if (!component->surface) component->surface=new AwtSurface(component);
        surface=component->surface; window=component->GetHWnd(); dc=GetDC(window);
    } else if (info->imgrep) {
        surface=awtImageSurface(info->imgrep,TRUE);
        if (surface) dc=surface->dc;
    }
    if (!surface || !dc) { r->error="null drawing surface"; return; }
    ++surface->references;
    DrawingLock *lock=new DrawingLock;
    lock->surface=surface; lock->window=window; lock->dc=dc; lock->saved=0;
    lock->thread=0; r->lock=lock; info->pData=(long)lock; r->result=info->state;
}
static DrawingLock *lockedSurface(stack_item *sp,ExecEnv *ee)
{
    Hsun_awt_windows_WDrawingSurfaceInfo *peer=(Hsun_awt_windows_WDrawingSurfaceInfo *)sp[0].p;
    DrawingLock *lock=peer ? (DrawingLock *)unhand(peer)->pData:NULL;
    if (!lock) SignalError(ee,"java/lang/NullPointerException","null drawing surface DC");
    return lock;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_lock_stub(stack_item *sp,ExecEnv *ee)
{
    DrawingRequest request={(Hsun_awt_windows_WDrawingSurfaceInfo *)sp[0].p,NULL,0,0,NULL};
    if (!request.peer) { SignalError(ee,"java/lang/NullPointerException","null drawing surface"); return sp+1; }
    AwtToolkit::GetInstance().Invoke(drawingRequest,&request);
    if (request.error) SignalError(ee,"java/lang/NullPointerException",request.error);
    else {
        /* The client thread owns this critical section across lock/unlock.
         * Unlock releases it before any synchronous toolkit cleanup request. */
        EnterCriticalSection(&request.lock->surface->nativeLock);
        request.lock->thread=GetCurrentThreadId();
        request.lock->surface->nativeOwner=request.lock->thread;
        ++request.lock->surface->nativeLocks;
        request.lock->saved=SaveDC(request.lock->dc);
        SetBkMode(request.lock->dc,TRANSPARENT);
    }
    sp[0].i=request.result; return sp+1;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_unlock_stub(stack_item *sp,ExecEnv *ee)
{
    DrawingLock *lock=lockedSurface(sp,ee);
    if (!lock) return sp;
    if (lock->thread!=GetCurrentThreadId()) {
        SignalError(ee,"java/lang/IllegalMonitorStateException","drawing surface locked by another thread"); return sp;
    }
    Hsun_awt_windows_WDrawingSurfaceInfo *peer=(Hsun_awt_windows_WDrawingSurfaceInfo *)sp[0].p;
    unhand(peer)->pData=0; RestoreDC(lock->dc,lock->saved); GdiFlush();
    if (!--lock->surface->nativeLocks) lock->surface->nativeOwner=0;
    LeaveCriticalSection(&lock->surface->nativeLock);
    DrawingRequest request={peer,lock,1,0,NULL}; AwtToolkit::GetInstance().Invoke(drawingRequest,&request);
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getHDC_stub(stack_item *sp,ExecEnv *ee)
{ DrawingLock *lock=lockedSurface(sp,ee); sp[0].i=lock ? (long)lock->dc:0; return sp+1; }
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getHWnd_stub(stack_item *sp,ExecEnv *ee)
{ DrawingLock *lock=lockedSurface(sp,ee); sp[0].i=lock && lock->surface->component ? (long)lock->window:0; return sp+1; }
static stack_item *imageHandle(stack_item *sp,ExecEnv *ee,int operation)
{
    Hsun_awt_windows_WDrawingSurfaceInfo *peer=(Hsun_awt_windows_WDrawingSurfaceInfo *)sp[0].p;
    if (!peer) { SignalError(ee,"java/lang/NullPointerException","null drawing surface"); return sp+1; }
    Classsun_awt_windows_WDrawingSurfaceInfo *info=unhand(peer);
    DrawingLock *lock=(DrawingLock *)info->pData;
    if (lock) sp[0].i=operation==2 ? (long)lock->surface->bitmap:
        info->imgrep && !unhand(info->imgrep)->offscreen ? (long)lock->surface->bits:0;
    else {
        DrawingRequest request={peer,NULL,operation,0,NULL};
        AwtToolkit::GetInstance().Invoke(drawingRequest,&request); sp[0].i=request.result;
    }
    return sp+1;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getHBitmap_stub(stack_item *sp,ExecEnv *ee)
{ return imageHandle(sp,ee,2); }
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getPBits_stub(stack_item *sp,ExecEnv *ee)
{ return imageHandle(sp,ee,3); }
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getDepth_stub(stack_item *sp,ExecEnv *)
{ AwtPixelFormat format; awtDisplayPixelFormat(&format); sp[0].i=format.depth; return sp+1; }
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDrawingSurfaceInfo_getHPalette_stub(stack_item *sp,ExecEnv *)
{ sp[0].i=(long)awtImagePalette(); return sp+1; }

