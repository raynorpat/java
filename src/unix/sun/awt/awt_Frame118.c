/* Solaris libawt_g.so MFramePeer.create 5f220 and shell peer contracts. */
#include "awt118.h"
#include "awt_Image118.h"
#include "sun_awt_motif_MFramePeer.h"
#include "sun_awt_motif_MEmbeddedFramePeer.h"
#include "java_awt_Component.h"
#include "java_awt_Insets.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Protocols.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/XmIm.h>
#include <stdlib.h>

static void free_icon(AwtPeer *data)
{
    if (data->iconWindow) XDestroyWindow(awt_display,data->iconWindow);
    if (data->iconPixmap) XFreePixmap(awt_display,data->iconPixmap);
    data->iconWindow=None; data->iconPixmap=None;
}
static void frame_shell_destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    AwtPeer *data=(AwtPeer *)closure;
    data->shell=NULL; data->container=NULL; data->menuBar=NULL;
}

/* Release 2d1dc: honor existing icon geometry, otherwise WM_ICON_SIZE ranges. */
struct Hjava_awt_Dimension *sun_awt_motif_MFramePeer_pGetIconImageSize(Hsun_awt_motif_MFramePeer *self,long width,long height)
{
    AwtPeer *data;
    XIconSize *sizes=NULL;
    int count=0,i,x,y,found=0;
    unsigned w=0,h=0,border,depth;
    unsigned long long best=~0ULL;
    Window root;
    struct Hjava_awt_Dimension *result=NULL;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->shell) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (data->iconWindow) XGetGeometry(awt_display,data->iconWindow,&root,&x,&y,&w,&h,&border,&depth);
    else if (XGetIconSizes(awt_display,RootWindow(awt_display,awt_screen),&sizes,&count) && count>0) {
        for (i=0;i<count;i++) {
            XIconSize *size=sizes+i;
            if (width>=size->min_width && width<=size->max_width && height>=size->min_height && height<=size->max_height) {
                unsigned cw=width-(width-size->min_width)%(size->width_inc>0 ? size->width_inc : 1);
                unsigned ch=height-(height-size->min_height)%(size->height_inc>0 ? size->height_inc : 1);
                unsigned long long distance=(unsigned long long)cw*cw+(unsigned long long)ch*ch;
                found=1;
                if (cw==width && ch==height) { w=cw; h=ch; break; }
                if (distance<best) { best=distance; w=cw; h=ch; }
            }
        }
        if (!found) {
            if (width>=sizes->max_width && height>=sizes->max_height) { w=sizes->max_width; h=sizes->max_height; }
            else if (width<sizes->min_width || height<sizes->min_height) {
                w=(sizes->min_width+sizes->max_width)/2; h=(sizes->min_height+sizes->max_height)/2;
            } else { w=sizes->min_width; h=sizes->min_height; }
        }
    }
    if (sizes) XFree(sizes);
    if (w && h) result=(struct Hjava_awt_Dimension *)execute_java_constructor(EE(),"java/awt/Dimension",NULL,"(II)",w,h);
    AWT_UNLOCK(); return result;
}

/* Release 2d43c: publish both an icon window and pixmap through WM_HINTS.
 * Own a copy so flushing the Java image cannot invalidate a WM's drawable. */
void sun_awt_motif_MFramePeer_pSetIconImage(Hsun_awt_motif_MFramePeer *self,Hsun_awt_image_ImageRepresentation *rep)
{
    AwtPeer *data;
    AwtImage *image;
    Pixmap pixmap;
    GC gc;
    XSetWindowAttributes attributes;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    image=rep ? awt118_image(rep,NULL) : NULL;
    if (!data || !data->shell || !image) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        pixmap=XCreatePixmap(awt_display,RootWindow(awt_display,awt_screen),image->width,image->height,awt_visInfo.depth);
        gc=XCreateGC(awt_display,pixmap,0,NULL);
        XCopyArea(awt_display,image->pixmap,pixmap,gc,0,0,image->width,image->height,0,0); XFreeGC(awt_display,gc);
        if (!data->iconWindow) {
            attributes.border_pixel=awt118_pixel(0); attributes.colormap=awt_cmap;
            data->iconWindow=XCreateWindow(awt_display,RootWindow(awt_display,awt_screen),0,0,image->width,image->height,
                0,awt_visInfo.depth,InputOutput,awt_visInfo.visual,CWBorderPixel|CWColormap,&attributes);
        }
        XtVaSetValues(data->shell,XtNiconWindow,data->iconWindow,XtNiconPixmap,pixmap,NULL);
        XSetWindowBackgroundPixmap(awt_display,data->iconWindow,pixmap); XClearWindow(awt_display,data->iconWindow);
        if (data->iconPixmap) XFreePixmap(awt_display,data->iconPixmap);
        data->iconPixmap=pixmap; XFlush(awt_display);
    }
    AWT_UNLOCK();
}

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

/* Release 2deb8 adopts the browser-supplied Xt shell; ordinary create at
 * 2e130 intentionally does nothing because Java uses EFcreate/NEFcreate. */
void sun_awt_motif_MEmbeddedFramePeer_create(Hsun_awt_motif_MEmbeddedFramePeer *self,
    Hsun_awt_motif_MComponentPeer *parent,struct Hjava_lang_Object *insets)
{}
void sun_awt_motif_MEmbeddedFramePeer_NEFcreate(Hsun_awt_motif_MEmbeddedFramePeer *self,
    Hsun_awt_motif_MComponentPeer *parent,struct Hjava_lang_Object *insets,long handle)
{
    AwtPeer *data;
    Hjava_awt_Insets *bounds=(Hjava_awt_Insets *)insets;
    AWT_LOCK();
    if(!handle || !unhand(self)->target || !bounds) SignalError(EE(),"java/lang/NullPointerException","embedded shell");
    else {
        data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self);
        if(data) {
            unhand(self)->insets=bounds;
            unhand(bounds)->top=unhand(bounds)->left=unhand(bounds)->bottom=unhand(bounds)->right=0;
            data->shell=(Widget)handle; data->cleanup=free_icon;
            XtAddCallback(data->shell,XtNdestroyCallback,frame_shell_destroyed,data);
            data->container=XtVaCreateManagedWidget("main",xmFormWidgetClass,data->shell,
                XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,NULL);
            data->widget=XtVaCreateManagedWidget("frame",xmDrawingAreaWidgetClass,data->container,
                XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,
                XmNtopAttachment,XmATTACH_FORM,XmNbottomAttachment,XmATTACH_FORM,
                XmNleftAttachment,XmATTACH_FORM,XmNrightAttachment,XmATTACH_FORM,NULL);
            XtAddEventHandler(data->shell,StructureNotifyMask|FocusChangeMask,False,shell_event,self);
            XFlush(awt_display);
        }
    }
    AWT_UNLOCK();
}
/* Release 3ee6c; baseline sun.awt.EmbeddedFrame is outside the peer inventory. */
JAVA_PUBLIC_API(stack_item *) Java_sun_awt_EmbeddedFrame_setPeer_stub(stack_item *p,struct execenv *ee)
{
    unhand((Hjava_awt_Component *)p[0].p)->peer=p[1].p;
    return p;
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
        data->cleanup=free_icon;
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
        XtAddCallback(data->shell,XtNdestroyCallback,frame_shell_destroyed,data);
        data->container=XtVaCreateManagedWidget("main",xmFormWidgetClass,data->shell,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,NULL);
        data->widget=XtVaCreateManagedWidget("frame",xmDrawingAreaWidgetClass,data->container,
            XmNmarginWidth,0,XmNmarginHeight,0,XmNresizePolicy,XmRESIZE_NONE,
            XmNtopAttachment,XmATTACH_FORM,XmNbottomAttachment,XmATTACH_FORM,
            XmNleftAttachment,XmATTACH_FORM,XmNrightAttachment,XmATTACH_FORM,NULL);
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
/* Release 2d794: attach the menu above the client and adjust Java insets. */
void sun_awt_motif_MFramePeer_pSetMenuBar(Hsun_awt_motif_MFramePeer *self,struct Hsun_awt_motif_MMenuBarPeer *bar)
{
    AwtPeer *data;
    Widget menu;
    Dimension height=0;
    XtWidgetGeometry preferred;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    menu=awt118_menu_widget((HObject *)bar);
    if (!data || !data->widget || (bar && !menu)) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        if (data->menuBar && data->menuBar!=menu) XtUnmanageChild(data->menuBar);
        data->menuBar=menu;
        if (menu) {
            XtVaSetValues(menu,XmNtopAttachment,XmATTACH_FORM,XmNleftAttachment,XmATTACH_FORM,
                XmNrightAttachment,XmATTACH_FORM,NULL);
            XtSetMappedWhenManaged(menu,True); XtManageChild(menu);
            XtVaGetValues(menu,XmNheight,&height,NULL);
            XtQueryGeometry(menu,NULL,&preferred);
            if ((preferred.request_mode&CWHeight) && preferred.height>height) {
                height=preferred.height; XtVaSetValues(menu,XmNheight,height,NULL);
            }
        }
        XtVaSetValues(data->widget,XmNtopAttachment,menu ? XmATTACH_WIDGET : XmATTACH_FORM,
            XmNtopWidget,menu,NULL);
        if (unhand(self)->insets) unhand(unhand(self)->insets)->top+=height-data->menuHeight;
        data->menuHeight=height; XFlush(awt_display);
    }
    AWT_UNLOCK();
}
