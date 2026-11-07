/* Solaris libawt_g.so 3a540/3ba28/63050; release 20028..20910.
 * Native records retain graphics references after widget retirement. */
#include "awt118.h"
#include "java_awt_Component.h"
#include "java_awt_Color.h"
#include "java_awt_Cursor.h"
#include "java_awt_AWTEvent.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <stdlib.h>

AwtPeer *awt118_peer(Hsun_awt_motif_MComponentPeer *peer)
{ return peer ? (AwtPeer *)unhand(peer)->pData : NULL; }
AwtPeer *awt118_new_peer(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=(AwtPeer *)calloc(1,sizeof(*data));
    if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    else { data->references=1; unhand(peer)->pData=(long)data; }
    return data;
}
void awt118_release_peer(AwtPeer *data)
{ if (--data->references==0) free(data); }

void awt118_callback_done(void)
{
    if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
}

static void destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    AwtPeer *data=(AwtPeer *)closure;
    data->widget=NULL;
}

static void canvas_event(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch)
{
    Hsun_awt_motif_MComponentPeer *peer=(Hsun_awt_motif_MComponentPeer *)closure;
    if (event->type==Expose) {
        execute_java_dynamic_method(EE(),(HObject *)peer,"handleExpose","(IIII)V",
            event->xexpose.x,event->xexpose.y,event->xexpose.width,event->xexpose.height);
        awt118_callback_done();
    }
}

void awt118_register_widget(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=awt118_peer(peer);
    XtAddCallback(data->widget,XmNdestroyCallback,destroyed,data);
    XtAddEventHandler(data->widget,ExposureMask,False,canvas_event,peer);
}

void awt118_dispose_peer(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=awt118_peer(peer);
    if (!data) return;
    unhand(peer)->pData=0;
    if (data->widget) {
        XtRemoveEventHandler(data->widget,ExposureMask,False,canvas_event,peer);
        if (XtHasCallbacks(data->widget,XmNactivateCallback)!=XtCallbackNoList)
            XtRemoveAllCallbacks(data->widget,XmNactivateCallback);
        XtRemoveCallback(data->widget,XmNdestroyCallback,destroyed,data);
        XtDestroyWidget(data->shell ? data->shell : data->widget);
        data->widget=NULL; data->shell=NULL;
    }
    if (data->cursor) XFreeCursor(awt_display,data->cursor);
    data->cursor=None;
    awt118_release_peer(data);
}

static unsigned long channel(unsigned long value,unsigned long mask)
{
    int shift=0;
    if (!mask) return 0;
    while (!(mask&1)) { shift++; mask>>=1; }
    return ((value*mask+127)/255)<<shift;
}
unsigned long awt118_pixel(unsigned long rgb)
{
    return channel((rgb>>16)&255,awt_visInfo.red_mask) |
        channel((rgb>>8)&255,awt_visInfo.green_mask) | channel(rgb&255,awt_visInfo.blue_mask);
}

void sun_awt_motif_MComponentPeer_pInitialize(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) awt118_register_widget(self);
    else SignalError(EE(),"java/lang/NullPointerException",NULL);
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pShow(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) { XtSetMappedWhenManaged(data->widget,True); XtManageChild(data->widget); XFlush(awt_display); }
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pHide(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) XtSetMappedWhenManaged(data->widget,False);
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pEnable(Hsun_awt_motif_MComponentPeer *self)
{ AwtPeer *d; AWT_LOCK(); d=awt118_peer(self); if(d && d->widget) XtSetSensitive(d->widget,True); AWT_UNLOCK(); }
void sun_awt_motif_MComponentPeer_pDisable(Hsun_awt_motif_MComponentPeer *self)
{ AwtPeer *d; AWT_LOCK(); d=awt118_peer(self); if(d && d->widget) XtSetSensitive(d->widget,False); AWT_UNLOCK(); }
void sun_awt_motif_MComponentPeer_pReshape(Hsun_awt_motif_MComponentPeer *self,long x,long y,long width,long height)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) XtVaSetValues(data->widget,XmNx,x,XmNy,y,
        XmNwidth,width>0 ? width : 1,XmNheight,height>0 ? height : 1,NULL);
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pDispose(Hsun_awt_motif_MComponentPeer *self)
{ AWT_LOCK(); awt118_dispose_peer(self); XFlush(awt_display); AWT_UNLOCK(); }
void sun_awt_motif_MComponentPeer_pSetForeground(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_Color *color)
{
    AwtPeer *data;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) XtVaSetValues(data->widget,XmNforeground,awt118_pixel(unhand(color)->value),NULL);
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pSetBackground(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_Color *color)
{
    AwtPeer *data;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) XmChangeColor(data->widget,awt118_pixel(unhand(color)->value));
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pSetFont(Hsun_awt_motif_MComponentPeer *self,struct Hjava_awt_Font *font)
{
    AwtPeer *data;
    XFontStruct *xfont;
    XmFontList list;
    AWT_LOCK(); data=awt118_peer(self); xfont=awt118_font(font);
    if (data && data->widget && xfont) {
        list=XmFontListCreate(xfont,XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(data->widget,XmNfontList,list,NULL); XmFontListFree(list);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_setCursor(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_Cursor *cursor)
{
    static const unsigned int shapes[]={68,34,152,150,12,14,134,136,138,16,70,96,60,52};
    AwtPeer *data;
    int type=cursor ? unhand(cursor)->type : 0;
    if (type<0 || type>13) { SignalError(EE(),"java/lang/IllegalArgumentException","Unknown cursor type"); return; }
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) {
        if (data->cursor) XFreeCursor(awt_display,data->cursor);
        data->cursor=cursor ? XCreateFontCursor(awt_display,shapes[type]) : None;
        if (XtIsRealized(data->widget)) XDefineCursor(awt_display,XtWindow(data->widget),data->cursor);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pMakeCursorVisible(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget && XtIsRealized(data->widget)) XDefineCursor(awt_display,XtWindow(data->widget),data->cursor);
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_nativeHandleEvent(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_AWTEvent *event)
{
    XEvent *native;
    if (!event) return;
    AWT_LOCK(); native=(XEvent *)unhand(event)->data;
    if (native) {
        if (!unhand(event)->consumed && XtWindowToWidget(awt_display,native->xany.window)) XtDispatchEvent(native);
        free(native); unhand(event)->data=0;
    }
    AWT_UNLOCK();
}

void sun_awt_motif_MCanvasPeer_create(Hsun_awt_motif_MComponentPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer(self))!=NULL) {
        data->widget=XtVaCreateManagedWidget("canvas",xmDrawingAreaWidgetClass,owner->widget,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,
            XmNwidth,1,XmNheight,1,XmNmappedWhenManaged,False,NULL);
    }
    AWT_UNLOCK();
}
