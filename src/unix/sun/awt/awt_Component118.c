/* Solaris libawt_g.so 3a540/3ba28/63050; release 20028..20910.
 * Native records retain graphics references after widget retirement. */
#include "awt118.h"
#include "java_awt_Component.h"
#include "java_awt_Color.h"
#include "java_awt_Cursor.h"
#include "java_awt_AWTEvent.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <stdlib.h>

static const EventMask peer_events=ExposureMask|FocusChangeMask|KeyPressMask|KeyReleaseMask|
    ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask;

AwtPeer *awt118_peer(Hsun_awt_motif_MComponentPeer *peer)
{ return peer ? (AwtPeer *)unhand(peer)->pData : NULL; }
AwtPeer *awt118_new_peer(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=(AwtPeer *)calloc(1,sizeof(*data));
    if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    else { data->references=1; data->javaPeer=peer; unhand(peer)->pData=(long)data; }
    return data;
}
void awt118_release_peer(AwtPeer *data)
{ if (--data->references==0) free(data); }

void awt118_callback_done(void)
{
    if (exceptionOccurred(EE())) { exceptionDescribe(EE()); exceptionClear(EE()); }
}

static void cleanup_peer(AwtPeer *data)
{
    void (*cleanup)(AwtPeer *)=data->cleanup;
    data->cleanup=NULL;
    if (cleanup) cleanup(data);
}

static void destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    AwtPeer *data=(AwtPeer *)closure;
    awt118_forget_input(data);
    cleanup_peer(data);
    data->widget=NULL;
    data->content=NULL;
}

static void canvas_event(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch)
{
    Hsun_awt_motif_MComponentPeer *peer=(Hsun_awt_motif_MComponentPeer *)closure;
    if (event->type==FocusIn || event->type==FocusOut) awt118_focus_event(peer,event);
    if (event->type==Expose) {
        execute_java_dynamic_method(EE(),(HObject *)peer,"handleExpose","(IIII)V",
            event->xexpose.x,event->xexpose.y,event->xexpose.width,event->xexpose.height);
        awt118_callback_done();
    }
}

void awt118_register_widget(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=awt118_peer(peer);
    awt118_register_input(data);
    XtAddCallback(data->widget,XmNdestroyCallback,destroyed,data);
    XtAddEventHandler(data->widget,peer_events,False,canvas_event,peer);
    if (data->content) XtAddEventHandler(data->content,peer_events,False,canvas_event,peer);
}

void awt118_dispose_peer(Hsun_awt_motif_MComponentPeer *peer)
{
    AwtPeer *data=awt118_peer(peer);
    if (!data) return;
    unhand(peer)->pData=0;
    awt118_forget_input(data);
    cleanup_peer(data);
    if (data->widget) {
        XtRemoveEventHandler(data->widget,peer_events,False,canvas_event,peer);
        if (data->content) XtRemoveEventHandler(data->content,peer_events,False,canvas_event,peer);
        if (XtHasCallbacks(data->widget,XmNactivateCallback)!=XtCallbackNoList)
            XtRemoveAllCallbacks(data->widget,XmNactivateCallback);
        XtRemoveCallback(data->widget,XmNdestroyCallback,destroyed,data);
        XtDestroyWidget(data->shell ? data->shell : data->widget);
        data->widget=NULL; data->shell=NULL; data->content=NULL;
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

/* Solaris release 20264: translate the realized widget's origin. */
struct Hjava_awt_Point *sun_awt_motif_MComponentPeer_pGetLocationOnScreen(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    Position x=0,y=0;
    struct Hjava_awt_Point *point=NULL;
    AWT_LOCK(); data=awt118_peer(self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (!XtIsRealized(data->widget)) SignalError(EE(),"java/lang/InternalError","Widget not visible");
    else {
        XtTranslateCoords(data->widget,0,0,&x,&y);
        point=(struct Hjava_awt_Point *)execute_java_constructor(EE(),"java/awt/Point",NULL,"(II)",x,y);
    }
    AWT_UNLOCK();
    return point;
}

static void traversal(Widget widget,Boolean enabled)
{
    WidgetList children;
    Cardinal count,i;
    XtVaGetValues(widget,XtNchildren,&children,XtNnumChildren,&count,NULL);
    for (i=0;i<count;i++) {
        if (!XtIsSubclass(children[i],xmScrollBarWidgetClass))
            XtVaSetValues(children[i],XmNtraversalOn,enabled,NULL);
        if (XtIsSubclass(children[i],xmDrawingAreaWidgetClass)) traversal(children[i],enabled);
    }
}

/* Solaris release 207a4/20838: focus a canvas without selecting its child. */
void sun_awt_motif_MComponentPeer_requestFocus(Hsun_awt_motif_MComponentPeer *self)
{
    AwtPeer *data;
    Widget shell;
    Boolean canvas;
    AWT_LOCK(); data=awt118_peer(self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        canvas=XtIsSubclass(data->widget,xmDrawingAreaWidgetClass);
        if (canvas) traversal(data->widget,False);
        if (!XmProcessTraversal(data->widget,XmTRAVERSE_CURRENT)) {
            shell=data->widget;
            while (shell && !XtIsShell(shell)) shell=XtParent(shell);
            if (shell) XtSetKeyboardFocus(shell,data->widget);
        }
        if (canvas) traversal(data->widget,True);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pSetForeground(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_Color *color)
{
    AwtPeer *data;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) {
        XtVaSetValues(data->widget,XmNforeground,awt118_pixel(unhand(color)->value),NULL);
        if (data->content) XtVaSetValues(data->content,XmNforeground,awt118_pixel(unhand(color)->value),NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MComponentPeer_pSetBackground(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_Color *color)
{
    AwtPeer *data;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=awt118_peer(self);
    if (data && data->widget) {
        XmChangeColor(data->widget,awt118_pixel(unhand(color)->value));
        if (data->content) XmChangeColor(data->content,awt118_pixel(unhand(color)->value));
    }
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
        XtVaSetValues(data->content ? data->content : data->widget,XmNfontList,list,NULL); XmFontListFree(list);
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
