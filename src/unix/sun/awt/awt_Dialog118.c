/* Solaris release 25628..26130 and awt_MToolkit_modalWait: dialog lifetime. */
#include "awt118.h"
#include "sun_awt_motif_MDialogPeer.h"
#include "sun_awt_motif_MFramePeer.h"
#include "java_awt_Dialog.h"
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include <X11/Shell.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>

static AwtPeer *modal;
int awt118_modal_allows(Widget widget)
{
    if (!modal) return 1;
    while (widget) {
        if (widget==modal->shell) return 1;
        widget=XtParent(widget);
    }
    return 0;
}
void awt118_end_modal(AwtPeer *data)
{
    AwtPeer **link=&modal;
    while (*link && *link!=data) link=&(*link)->modalPrevious;
    if (*link) *link=data->modalPrevious;
    data->modalPrevious=NULL; data->modalActive=0;
    monitorNotifyAll((unsigned int)awt_lock);
}
static void shell_destroyed(Widget widget,XtPointer closure,XtPointer call)
{ AwtPeer *data=(AwtPeer *)closure; awt118_end_modal(data); data->shell=NULL; }
static void close_dialog(Widget widget,XtPointer closure,XtPointer call)
{ execute_java_dynamic_method(EE(),(HObject *)closure,"handleQuit","()V"); awt118_callback_done(); }
void sun_awt_motif_MDialogPeer_create(Hsun_awt_motif_MDialogPeer *self,Hsun_awt_motif_MComponentPeer *parent,struct Hjava_lang_Object *insets)
{
    AwtPeer *data,*owner;
    Hjava_awt_Component *target=unhand(self)->target;
    Classjava_awt_Insets *bounds;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->shell || !insets) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        unhand(self)->insets=(Hjava_awt_Insets *)insets; bounds=unhand(unhand(self)->insets);
        if (bounds->top<0) bounds->top=0;
        if (bounds->bottom<0) bounds->bottom=0;
        if (bounds->left<0) bounds->left=0;
        if (bounds->right<0) bounds->right=0;
        data->cleanup=awt118_end_modal;
        data->shell=XtVaCreatePopupShell("dialog",xmDialogShellWidgetClass,owner->shell,
            XtNvisual,awt_visInfo.visual,XtNdepth,awt_visInfo.depth,XtNcolormap,awt_cmap,
            XtNtransientFor,owner->shell,XtNallowShellResize,True,XtNmappedWhenManaged,False,
            XtNx,unhand(target)->x,XtNy,unhand(target)->y,
            XtNwidth,unhand(target)->width>0 ? unhand(target)->width : 1,
            XtNheight,unhand(target)->height>0 ? unhand(target)->height : 1,
            XmNdeleteResponse,XmDO_NOTHING,NULL);
        data->container=XtVaCreateManagedWidget("main",xmFormWidgetClass,data->shell,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,NULL);
        data->widget=XtVaCreateManagedWidget("dialogClient",xmDrawingAreaWidgetClass,data->container,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,
            XmNtopAttachment,XmATTACH_FORM,XmNbottomAttachment,XmATTACH_FORM,
            XmNleftAttachment,XmATTACH_FORM,XmNrightAttachment,XmATTACH_FORM,NULL);
        XtAddCallback(data->shell,XtNdestroyCallback,shell_destroyed,data);
        XtAddEventHandler(data->shell,StructureNotifyMask|FocusChangeMask|PropertyChangeMask,False,awt118_shell_event,self);
        XmAddWMProtocolCallback(data->shell,XmInternAtom(awt_display,"WM_DELETE_WINDOW",False),close_dialog,self);
        awt118_initial_extents(data,bounds);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MDialogPeer_pShow(Hsun_awt_motif_MDialogPeer *self)
{
    AwtPeer *data;
    int isModal;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        isModal=unhand((Hjava_awt_Dialog *)unhand(self)->target)->modal;
        if (data->modalActive) { XRaiseWindow(awt_display,XtWindow(data->shell)); AWT_UNLOCK(); return; }
        if (isModal) {
            data->modalPrevious=modal; modal=data; data->modalActive=1; data->references++;
            XtVaSetValues(data->container,XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL,NULL);
        }
        XtSetMappedWhenManaged(data->shell,True);
        XtPopup(data->shell,isModal ? XtGrabExclusive : XtGrabNone); XFlush(awt_display);
        if (isModal) {
            while (data->modalActive && data->shell) monitorWait((unsigned int)awt_lock,-1);
            awt118_release_peer(data);
        }
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MDialogPeer_pHide(Hsun_awt_motif_MDialogPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data) { awt118_end_modal(data); if (data->shell) XtPopdown(data->shell); }
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MDialogPeer_pDispose(Hsun_awt_motif_MDialogPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        XtRemoveEventHandler(data->shell,StructureNotifyMask|FocusChangeMask|PropertyChangeMask,False,awt118_shell_event,self);
        XmRemoveWMProtocolCallback(data->shell,XmInternAtom(awt_display,"WM_DELETE_WINDOW",False),close_dialog,self);
        XtPopdown(data->shell);
    }
    awt118_dispose_peer((Hsun_awt_motif_MComponentPeer *)self); XFlush(awt_display); AWT_UNLOCK();
}
/* These operations use the audited common component/insets prefix. */
void sun_awt_motif_MDialogPeer_pSetTitle(Hsun_awt_motif_MDialogPeer *self,Hjava_lang_String *title)
{ sun_awt_motif_MFramePeer_pSetTitle((Hsun_awt_motif_MFramePeer *)self,title); }
void sun_awt_motif_MDialogPeer_pReshape(Hsun_awt_motif_MDialogPeer *self,long x,long y,long width,long height)
{ sun_awt_motif_MFramePeer_pReshape((Hsun_awt_motif_MFramePeer *)self,x,y,width,height); }
void sun_awt_motif_MDialogPeer_setResizable(Hsun_awt_motif_MDialogPeer *self,long resizable)
{ sun_awt_motif_MFramePeer_setResizable((Hsun_awt_motif_MFramePeer *)self,resizable); }
void sun_awt_motif_MDialogPeer_addTextComponent(Hsun_awt_motif_MDialogPeer *self)
{ sun_awt_motif_MFramePeer_addTextComponent((Hsun_awt_motif_MFramePeer *)self); }
void sun_awt_motif_MDialogPeer_toBack(Hsun_awt_motif_MDialogPeer *self)
{ sun_awt_motif_MFramePeer_toBack((Hsun_awt_motif_MFramePeer *)self); }
