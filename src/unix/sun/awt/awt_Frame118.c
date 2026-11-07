/* Solaris libawt_g.so MFramePeer.create 5f220 and shell peer contracts. */
#include "awt118.h"
#include "sun_awt_motif_MFramePeer.h"
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <Xm/Protocols.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/XmIm.h>
#include <stdlib.h>

static void close_window(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"handleQuit","()V");
    awt118_callback_done();
}

/* Release 2da90/3123c: include the input method's status/preedit area in
 * bottom insets. Query XIC attributes instead of Solaris Motif private offsets. */
static int input_method_height(Widget widget)
{
    XIC ic;
    XIMStyle style=0;
    XRectangle *area=NULL;
    XVaNestedList attributes;
    WidgetList children;
    Cardinal count,i;
    int height=0,value;
    if (XmIsText(widget) || XmIsTextField(widget)) {
        ic=XmImGetXIC(widget,XmPER_SHELL,NULL,0);
        if (ic && !XGetICValues(ic,XNInputStyle,&style,NULL)) {
            if (style&XIMStatusArea) {
                attributes=XVaCreateNestedList(0,XNAreaNeeded,&area,NULL);
                if (!XGetICValues(ic,XNStatusAttributes,attributes,NULL) && area) {
                    height=area->height; XFree(area); area=NULL;
                }
                XFree(attributes);
            }
            if (style&XIMPreeditArea) {
                attributes=XVaCreateNestedList(0,XNAreaNeeded,&area,NULL);
                if (!XGetICValues(ic,XNPreeditAttributes,attributes,NULL) && area) {
                    if (area->height>height) height=area->height;
                    XFree(area);
                }
                XFree(attributes);
            }
        }
    } else if (XtIsComposite(widget)) {
        XtVaGetValues(widget,XtNchildren,&children,XtNnumChildren,&count,NULL);
        for (i=0;i<count;i++) {
            value=input_method_height(children[i]); if (value>height) height=value;
        }
    }
    return height;
}
void sun_awt_motif_MFramePeer_addTextComponent(Hsun_awt_motif_MFramePeer *self)
{
    AwtPeer *data;
    int height;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->shell) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        height=input_method_height(data->widget);
        if (height) height+=2;
        if (unhand(self)->insets) unhand(unhand(self)->insets)->bottom+=height-data->imHeight;
        data->imHeight=height;
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
static void shell_event(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch)
{
    Hsun_awt_motif_MFramePeer *peer=(Hsun_awt_motif_MFramePeer *)closure;
    Hjava_awt_Component *target=unhand(peer)->target;
    if (event->type==ConfigureNotify) {
        unhand(target)->width=event->xconfigure.width;
        unhand(target)->height=event->xconfigure.height;
        execute_java_dynamic_method(EE(),(HObject *)peer,"handleResize","(II)V",event->xconfigure.width,event->xconfigure.height);
    } else if (event->type==FocusIn || event->type==FocusOut) {
        execute_java_dynamic_method(EE(),(HObject *)peer,event->type==FocusIn ? "handleActivate" : "handleDeactivate","()V");
    }
    awt118_callback_done();
}
void sun_awt_motif_MFramePeer_create(Hsun_awt_motif_MFramePeer *self,Hsun_awt_motif_MComponentPeer *parent,struct Hjava_lang_Object *insets)
{
    AwtPeer *data;
    Hjava_awt_Component *target=unhand(self)->target;
    Classjava_awt_Insets *bounds;
    Atom close;
    AWT_LOCK();
    data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data) {
        unhand(self)->insets=(Hjava_awt_Insets *)insets;
        bounds=unhand(unhand(self)->insets);
        if (bounds->top<0) bounds->top=0;
        if (bounds->left<0) bounds->left=0;
        if (bounds->bottom<0) bounds->bottom=0;
        if (bounds->right<0) bounds->right=0;
        data->shell=XtVaAppCreateShell("AWTapp","XApplication",vendorShellWidgetClass,awt_display,
            XtNvisual,awt_visInfo.visual,XtNdepth,awt_visInfo.depth,XtNcolormap,awt_cmap,
            XtNx,unhand(target)->x,XtNy,unhand(target)->y,
            XtNwidth,unhand(target)->width>0 ? unhand(target)->width : 1,
            XtNheight,unhand(target)->height>0 ? unhand(target)->height : 1,
            XtNallowShellResize,True,XtNmappedWhenManaged,False,XmNdeleteResponse,XmDO_NOTHING,NULL);
        data->widget=XtVaCreateManagedWidget("frame",xmDrawingAreaWidgetClass,data->shell,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,NULL);
        close=XmInternAtom(awt_display,"WM_DELETE_WINDOW",False);
        XmAddWMProtocolCallback(data->shell,close,close_window,self);
        XtAddEventHandler(data->shell,StructureNotifyMask|FocusChangeMask,False,shell_event,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_pSetTitle(Hsun_awt_motif_MFramePeer *self,Hjava_lang_String *title)
{
    AwtPeer *data;
    char *text;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        text=title ? allocCString(title) : NULL;
        XtVaSetValues(data->shell,XtNtitle,text ? text : "",XtNiconName,text ? text : "",NULL);
        free(text);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_pShow(Hsun_awt_motif_MFramePeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) { XtPopup(data->shell,XtGrabNone); XFlush(awt_display); }
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_pHide(Hsun_awt_motif_MFramePeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) XtPopdown(data->shell);
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_pReshape(Hsun_awt_motif_MFramePeer *self,long x,long y,long width,long height)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) XtVaSetValues(data->shell,XtNx,x,XtNy,y,XtNwidth,width>0 ? width : 1,
        XtNheight,height>0 ? height : 1,NULL);
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_pDispose(Hsun_awt_motif_MFramePeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        XtRemoveEventHandler(data->shell,StructureNotifyMask|FocusChangeMask,False,shell_event,self);
        XmRemoveWMProtocolCallback(data->shell,XmInternAtom(awt_display,"WM_DELETE_WINDOW",False),close_window,self);
    }
    awt118_dispose_peer((Hsun_awt_motif_MComponentPeer *)self);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_setResizable(Hsun_awt_motif_MFramePeer *self,long resizable)
{
    AwtPeer *data;
    Dimension width,height;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) {
        XtVaGetValues(data->shell,XtNwidth,&width,XtNheight,&height,NULL);
        XtVaSetValues(data->shell,XtNminWidth,resizable ? 1 : width,XtNminHeight,resizable ? 1 : height,
            XtNmaxWidth,resizable ? 65535 : width,XtNmaxHeight,resizable ? 65535 : height,NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MFramePeer_toBack(Hsun_awt_motif_MFramePeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell && XtIsRealized(data->shell)) XLowerWindow(awt_display,XtWindow(data->shell));
    AWT_UNLOCK();
}
