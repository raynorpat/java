/* Frame creation recovered from winawt_g 10011561 -> 10010C30. */
#include "awt_Window.h"
#include "awt_Menu.h"
#include "awt_Image.h"
#include <stdlib.h>
AwtFrame::~AwtFrame() { if (icon) DestroyIcon(icon); }
/* WWindowPeer.setDefaultFocus and Component.requestFocus, without entering
 * a synchronized native peer method on the HWND owner thread. Another Java
 * thread may hold that peer monitor while waiting for hide/dispose Invoke. */
static Hjava_awt_Component *defaultFocus(Hjava_awt_Container *container,ClassClass *containerClass)
{
    for (int i=0;i<unhand(container)->ncomponents;i++) {
        HArrayOfObject *children=unhand(container)->component;
        if (!children || i>=(int)obj_length(children)) break;
        Hjava_awt_Component *volatile child=(Hjava_awt_Component *)unhand(children)->body[i];
        if (!child || !unhand(child)->visible || !unhand(child)->enabled) continue;
        BOOL traversable=execute_java_dynamic_method(EE(),(HObject *)child,"isFocusTraversable","()Z");
        if (exceptionOccurred(EE())) return NULL;
        if (traversable) return child;
        if (is_instance_of((HObject *)child,containerClass,EE())) {
            Hjava_awt_Component *found=defaultFocus((Hjava_awt_Container *)child,containerClass);
            if (found || exceptionOccurred(EE())) return found;
        }
    }
    return NULL;
}
static void restoreDefaultFocus(Hjava_awt_Component *target)
{
    ClassClass *containerClass=FindClass(EE(),"java/awt/Container",TRUE);
    if (!containerClass) return;
    Hjava_awt_Component *volatile selected=defaultFocus((Hjava_awt_Container *)target,containerClass);
    if (exceptionOccurred(EE())) return;
    if (!selected) {
        selected=target;
        while (unhand(selected)->peer && is_instance_of((HObject *)selected,containerClass,EE())) {
            Classjava_awt_Container *container=unhand((Hjava_awt_Container *)selected);
            if (!container->ncomponents || !container->component) break;
            Hjava_awt_Component *child=(Hjava_awt_Component *)unhand(container->component)->body[0];
            if (!child || !unhand(child)->visible || !unhand(child)->enabled) break;
            selected=child;
        }
    }
    Hjava_awt_Component *volatile heavyweight=selected;
    AwtComponent *component=NULL;
    ClassClass *nativePeerClass=FindClass(EE(),"sun/awt/windows/WComponentPeer",TRUE);
    ClassClass *lightPeerClass=FindClass(EE(),"java/awt/peer/LightweightPeer",TRUE);
    if (!nativePeerClass || !lightPeerClass) return;
    while (heavyweight && unhand(heavyweight)->peer) {
        HObject *p=(HObject *)unhand(heavyweight)->peer;
        if (is_instance_of(p,nativePeerClass,EE())) {
            component=AwtComponent::Get((Hsun_awt_windows_WComponentPeer *)p); break;
        }
        if (!is_instance_of(p,lightPeerClass,EE())) return;
        heavyweight=(Hjava_awt_Component *)unhand(heavyweight)->parent;
    }
    if (!component) return;
    AwtComponentLease lease(component);
    if (heavyweight!=selected) {
        HObject *dispatcher=(HObject *)unhand((Hjava_awt_Container *)heavyweight)->dispatcher;
        if (!dispatcher || !execute_java_dynamic_method(EE(),dispatcher,"setFocusRequest","(Ljava/awt/Component;)Z",(HObject *)selected)) return;
    }
    if (component->IsDisposed() || exceptionOccurred(EE())) return;
    SetFocus(component->GetHWnd());
    HObject *toolkit=(HObject *)execute_java_static_method(EE(),
        FindClass(EE(),"java/awt/Toolkit",TRUE),"getDefaultToolkit","()Ljava/awt/Toolkit;");
    /* The internal focus path must not apply the public queue-access security
     * check to applets. This is Toolkit.getEventQueue's protected delegation. */
    HObject *queue=toolkit ? (HObject *)execute_java_dynamic_method(EE(),toolkit,"getSystemEventQueueImpl","()Ljava/awt/EventQueue;"):NULL;
    if (queue && !exceptionOccurred(EE()))
        execute_java_dynamic_method(EE(),queue,"changeKeyEventFocus","(Ljava/lang/Object;)V",(HObject *)heavyweight);
}
/* Activation/focus restoration: 10010FA4. Defer Java work until the
 * activating SendMessage/ShowWindow caller has released its monitors. */
LRESULT AwtFrame::WindowProc(UINT message,WPARAM w,LPARAM l)
{
    if (started && message==WM_ACTIVATE) {
        BOOL active=LOWORD(w)!=WA_INACTIVE;
        PostMessage(hwnd,WM_APP+2,active ? 205:206,0);
        if (active) PostMessage(hwnd,WM_APP+8,0,0);
    }
    if (message==WM_APP+8) {
        if (started && GetActiveWindow()==hwnd) {
            Hsun_awt_windows_WComponentPeer *focus=(Hsun_awt_windows_WComponentPeer *)
                execute_java_dynamic_method(EE(),(HObject *)peer,"getFocusPeer","()Lsun/awt/windows/WComponentPeer;");
            AwtComponent *component=AwtComponent::Get(focus);
            if (component) SetFocus(component->GetHWnd());
            else if (!exceptionOccurred(EE())) restoreDefaultFocus(GetTarget());
            if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
        }
        return 0;
    }
    return AwtWindow::WindowProc(message,w,l);
}
struct FrameRequest {
    Hsun_awt_windows_WComponentPeer *peer, *parent;
    BOOL failed;
};
static void createFrame(void *argument)
{
    FrameRequest *r = (FrameRequest *)argument;
    AwtComponent *parent = AwtComponent::Get(r->parent);
    if (r->parent && !parent) { r->failed = TRUE; return; }
    AwtFrame *frame = new AwtFrame(r->peer);
    frame->background = GetSysColor(COLOR_WINDOWFRAME);
    if (!frame->Create("SunAwtFrame", 0x02CF0000, 0x100, parent ? parent->GetHWnd() : NULL)) {
        delete frame; r->failed = TRUE;
    } else frame->UpdateInsets(NULL);
}
extern "C" void sun_awt_windows_WFramePeer_create(Hsun_awt_windows_WFramePeer *p,Hsun_awt_windows_WComponentPeer *parent)
{
    if (!p || !unhand(p)->target) { SignalError(NULL,"java/lang/NullPointerException","invalid peer"); return; }
    FrameRequest r = {(Hsun_awt_windows_WComponentPeer *)p,parent,FALSE};
    AwtToolkit::GetInstance().Invoke(createFrame,&r);
    if (r.failed) SignalError(NULL,"java/lang/InternalError","cannot create frame");
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer_setMenuBar0_stub(stack_item *sp,struct execenv *)
{
    awtSetMenuBar((Hsun_awt_windows_WComponentPeer *)sp[0].p,(HObject *)sp[1].p);
    return sp;
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer_create_stub(stack_item *sp,struct execenv *)
{
    sun_awt_windows_WFramePeer_create((Hsun_awt_windows_WFramePeer *)sp[0].p,
        (Hsun_awt_windows_WComponentPeer *)sp[1].p);
    return sp;
}
/* Icon ownership and WM_SETICON recovered from 100112D7/10017643. Each
 * frame owns its icon; copying the image avoids Sun's stale cached HICON
 * when the same Image is assigned again after replacement. */
struct IconRequest {
    Hsun_awt_windows_WComponentPeer *peer;
    Hsun_awt_image_ImageRepresentation *image;
    BOOL failed;
};
static void setFrameIcon(void *argument)
{
    IconRequest *r=(IconRequest *)argument;
    AwtFrame *frame=(AwtFrame *)AwtComponent::Get(r->peer);
    if (!frame) {r->failed=TRUE;return;}
    AwtComponentLease lease(frame);
    AwtSurface *surface=awtImageSurface(r->image,FALSE);
    HICON icon=NULL;
    if (surface && surface->bits && surface->width>0 && surface->height>0) {
        AwtSurfaceLock lock(surface);
        if (frame->IsDisposed()) return;
        int width=GetSystemMetrics(SM_CXICON),height=GetSystemMetrics(SM_CYICON);
        BITMAPINFO info={0};void *bits=NULL;
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
        info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
        HBITMAP color=CreateDIBSection(NULL,&info,DIB_RGB_COLORS,&bits,NULL,0);
        int stride=((width+15)/16)*2;
        BYTE *mask=(BYTE *)calloc(stride,height);
        if (color && mask) {
            for(int y=0;y<height;y++) for(int x=0;x<width;x++) {
                int sx=x*surface->width/width,sy=y*surface->height/height;
                BOOL transparent=surface->alpha && !surface->alpha[sy*surface->width+sx];
                ((DWORD *)bits)[y*width+x]=transparent?0:surface->Pixel(sx,sy);
                if (transparent)
                    mask[y*stride+x/8]|=0x80>>(x&7);
            }
            HBITMAP mono=CreateBitmap(width,height,1,1,mask);
            if (mono) {
                ICONINFO values={0};values.fIcon=TRUE;values.hbmMask=mono;values.hbmColor=color;
                icon=CreateIconIndirect(&values);DeleteObject(mono);
            }
        }
        if(color)DeleteObject(color);free(mask);
    }
    if (frame->IsDisposed()) {if(icon)DestroyIcon(icon);return;}
    if (IsIconic(frame->GetHWnd())) InvalidateRect(frame->GetHWnd(),NULL,TRUE);
    HICON old=frame->icon;frame->icon=icon;
    SendMessage(frame->GetHWnd(),WM_SETICON,ICON_BIG,(LPARAM)(icon?icon:LoadIcon(NULL,IDI_APPLICATION)));
    if(old)DestroyIcon(old);
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFramePeer__0005fsetIconImage_stub(stack_item *sp,struct execenv *ee)
{
    if (!sp[1].p) {SignalError(ee,"java/lang/NullPointerException","null ImageRepresentation");return sp;}
    IconRequest r={(Hsun_awt_windows_WComponentPeer *)sp[0].p,(Hsun_awt_image_ImageRepresentation *)sp[1].p,FALSE};
    AwtToolkit::GetInstance().Invoke(setFrameIcon,&r);
    if(r.failed)SignalError(ee,"java/lang/NullPointerException","invalid peer");return sp;
}
