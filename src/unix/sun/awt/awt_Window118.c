/* Solaris release 2af14..2b594: undecorated owned windows. */
#include "awt118.h"
#include "sun_awt_motif_MWindowPeer.h"
#include "java_awt_Component.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>

static void shell_destroyed(Widget widget,XtPointer closure,XtPointer call)
{ ((AwtPeer *)closure)->shell=NULL; }
void sun_awt_motif_MWindowPeer_create(Hsun_awt_motif_MWindowPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Hjava_awt_Component *target=unhand(self)->target;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->shell) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        data->shell=XtVaCreatePopupShell("window",transientShellWidgetClass,owner->shell,
            XtNvisual,awt_visInfo.visual,XtNdepth,awt_visInfo.depth,XtNcolormap,awt_cmap,
            XtNtransientFor,owner->shell,XtNoverrideRedirect,True,XtNallowShellResize,True,
            XtNx,unhand(target)->x,XtNy,unhand(target)->y,
            XtNwidth,unhand(target)->width>0 ? unhand(target)->width : 1,
            XtNheight,unhand(target)->height>0 ? unhand(target)->height : 1,
            XmNmwmDecorations,0,XmNmwmFunctions,0,NULL);
        data->container=XtVaCreateManagedWidget("main",xmFormWidgetClass,data->shell,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,NULL);
        data->widget=XtVaCreateManagedWidget("windowClient",xmDrawingAreaWidgetClass,data->container,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,
            XmNtopAttachment,XmATTACH_FORM,XmNbottomAttachment,XmATTACH_FORM,
            XmNleftAttachment,XmATTACH_FORM,XmNrightAttachment,XmATTACH_FORM,NULL);
        awt118_create_warning(data);
        XtAddCallback(data->shell,XtNdestroyCallback,shell_destroyed,data);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MWindowPeer_pShow(Hsun_awt_motif_MWindowPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        XtPopup(data->shell,XtGrabNone); XRaiseWindow(awt_display,XtWindow(data->shell)); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MWindowPeer_pHide(Hsun_awt_motif_MWindowPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) XtPopdown(data->shell);
    AWT_UNLOCK();
}
void sun_awt_motif_MWindowPeer_pReshape(Hsun_awt_motif_MWindowPeer *self,long x,long y,long width,long height)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) XtVaSetValues(data->shell,XtNx,x,XtNy,y,
        XtNwidth,width>0 ? width : 1,XtNheight,height>0 ? height : 1,NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MWindowPeer_pDispose(Hsun_awt_motif_MWindowPeer *self)
{ AWT_LOCK(); awt118_dispose_peer((Hsun_awt_motif_MComponentPeer *)self); XFlush(awt_display); AWT_UNLOCK(); }
void sun_awt_motif_MWindowPeer_toBack(Hsun_awt_motif_MWindowPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell && XtIsRealized(data->shell)) XLowerWindow(awt_display,XtWindow(data->shell));
    AWT_UNLOCK();
}
