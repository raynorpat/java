/* Solaris release 28158..29704: menu records, callbacks and retirement. */
#include "awt118.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "sun_awt_motif_MMenuPeer.h"
#include "sun_awt_motif_MMenuItemPeer.h"
#include "sun_awt_motif_MCheckboxMenuItemPeer.h"
#include "java_awt_MenuItem.h"
#include "java_awt_Menu.h"
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SeparatoG.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Widget widget,pulldown;
    AwtPeer *frame;
} AwtMenu;

/* Both audited peer layouts put the native pData word first. */
static AwtMenu *record(HObject *peer)
{ return peer ? (AwtMenu *)*(long *)unhand(peer) : NULL; }
Widget awt118_menu_widget(HObject *peer)
{ AwtMenu *data=record(peer); return data ? data->widget : NULL; }
static AwtMenu *allocate(HObject *peer)
{
    AwtMenu *data=(AwtMenu *)calloc(1,sizeof(*data));
    if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    else *(long *)unhand(peer)=(long)data;
    return data;
}
static void destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    AwtMenu *data=(AwtMenu *)closure;
    if (data->frame && data->frame->menuBar==widget) data->frame->menuBar=NULL;
    if (data->widget==widget) data->widget=NULL;
    if (data->pulldown==widget) data->pulldown=NULL;
}
static void dispose(HObject *peer)
{
    AwtMenu *data=record(peer);
    if (!data) return;
    *(long *)unhand(peer)=0;
    if (data->pulldown) XtDestroyWidget(data->pulldown);
    if (data->widget) XtDestroyWidget(data->widget);
    if (data->frame) awt118_release_peer(data->frame);
    free(data);
}
static XmString label(Hjava_lang_String *value)
{
    char *text=value ? allocCString(value) : NULL;
    XmString result=XmStringCreateLocalized(text ? text : "");
    free(text); return result;
}
static void font(Widget widget,Hjava_awt_MenuItem *target)
{
    struct Hjava_awt_Font *value=(struct Hjava_awt_Font *)execute_java_dynamic_method(EE(),(HObject *)target,"getFont","()Ljava/awt/Font;");
    XFontStruct *xfont=value ? awt118_font(value) : NULL;
    if (xfont) {
        XmFontList list=XmFontListCreate(xfont,XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(widget,XmNfontList,list,NULL); XmFontListFree(list);
    }
}
void sun_awt_motif_MMenuBarPeer_create(Hsun_awt_motif_MMenuBarPeer *self,struct Hsun_awt_motif_MFramePeer *parent)
{
    AwtPeer *frame;
    AwtMenu *data;
    AWT_LOCK(); frame=awt118_peer((Hsun_awt_motif_MComponentPeer *)parent);
    if (!frame || !frame->container) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=allocate((HObject *)self))!=NULL) {
        data->frame=frame; frame->references++;
        data->widget=XmCreateMenuBar(frame->container,"menuBar",NULL,0);
        XtAddCallback(data->widget,XmNdestroyCallback,destroyed,data);
        XtSetMappedWhenManaged(data->widget,False); XtManageChild(data->widget);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MMenuBarPeer_pDispose(Hsun_awt_motif_MMenuBarPeer *self)
{ AWT_LOCK(); dispose((HObject *)self); AWT_UNLOCK(); }
static void create_menu(Hsun_awt_motif_MMenuPeer *self,Widget parent)
{
    AwtMenu *data;
    XmString text;
    Hjava_awt_Menu *target=(Hjava_awt_Menu *)unhand(self)->target;
    if (!parent) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    data=allocate((HObject *)self); if (!data) return;
    data->pulldown=XmCreatePulldownMenu(parent,"menu",NULL,0);
    XtAddCallback(data->pulldown,XmNdestroyCallback,destroyed,data);
    XtVaSetValues(data->pulldown,XmNtearOffModel,unhand(target)->tearOff ? XmTEAR_OFF_ENABLED : XmTEAR_OFF_DISABLED,NULL);
    text=label(unhand(target)->label);
    data->widget=XtVaCreateManagedWidget("menu",xmCascadeButtonWidgetClass,parent,
        XmNsubMenuId,data->pulldown,XmNlabelString,text,XmNsensitive,unhand(target)->enabled,NULL);
    XmStringFree(text); font(data->widget,(Hjava_awt_MenuItem *)target);
    XtAddCallback(data->widget,XmNdestroyCallback,destroyed,data);
    if (unhand(target)->isHelpMenu) XtVaSetValues(parent,XmNmenuHelpWidget,data->widget,NULL);
}
void sun_awt_motif_MMenuPeer_createMenu(Hsun_awt_motif_MMenuPeer *self,Hsun_awt_motif_MMenuBarPeer *parent)
{ AWT_LOCK(); create_menu(self,awt118_menu_widget((HObject *)parent)); AWT_UNLOCK(); }
void sun_awt_motif_MMenuPeer_createSubMenu(Hsun_awt_motif_MMenuPeer *self,Hsun_awt_motif_MMenuPeer *parent)
{
    AwtMenu *data;
    AWT_LOCK(); data=record((HObject *)parent); create_menu(self,data ? data->pulldown : NULL); AWT_UNLOCK();
}
void sun_awt_motif_MMenuPeer_pDispose(Hsun_awt_motif_MMenuPeer *self)
{ AWT_LOCK(); dispose((HObject *)self); AWT_UNLOCK(); }
static void activate(Widget widget,XtPointer closure,XtPointer call)
{
    Hsun_awt_motif_MMenuItemPeer *peer=(Hsun_awt_motif_MMenuItemPeer *)closure;
    XmAnyCallbackStruct *callback=(XmAnyCallbackStruct *)call;
    XEvent *event=callback ? callback->event : NULL;
    unsigned int state=event && (event->type==ButtonPress || event->type==ButtonRelease) ? event->xbutton.state :
        event && (event->type==KeyPress || event->type==KeyRelease) ? event->xkey.state : 0;
    int modifiers=((state&ShiftMask) ? 1 : 0)|((state&ControlMask) ? 2 : 0)|
        ((state&Mod4Mask) ? 4 : 0)|((state&Mod1Mask) ? 8 : 0);
    int64_t when=event ? (int64_t)event->xbutton.time : 0;
    if (unhand(peer)->isCheckbox)
        execute_java_dynamic_method(EE(),(HObject *)peer,"action","(JIZ)V",when,modifiers,XmToggleButtonGetState(widget));
    else execute_java_dynamic_method(EE(),(HObject *)peer,"action","(JI)V",when,modifiers);
    awt118_callback_done();
}
void sun_awt_motif_MMenuItemPeer_createMenuItem(Hsun_awt_motif_MMenuItemPeer *self,Hsun_awt_motif_MMenuPeer *parent)
{
    AwtMenu *data,*owner;
    Hjava_awt_MenuItem *target=unhand(self)->target;
    char *text;
    XmString string;
    AWT_LOCK(); owner=record((HObject *)parent);
    if (!owner || !owner->pulldown) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=allocate((HObject *)self))!=NULL) {
        text=unhand(target)->label ? allocCString(unhand(target)->label) : NULL;
        if (text && !strcmp(text,"-")) data->widget=XmCreateSeparatorGadget(owner->pulldown,"separator",NULL,0);
        else {
            string=label(unhand(target)->label);
            data->widget=XtVaCreateWidget("item",unhand(self)->isCheckbox ? xmToggleButtonWidgetClass : xmPushButtonWidgetClass,
                owner->pulldown,XmNlabelString,string,XmNsensitive,unhand(target)->enabled,NULL);
            XmStringFree(string); font(data->widget,target);
            XtAddCallback(data->widget,unhand(self)->isCheckbox ? XmNvalueChangedCallback : XmNactivateCallback,activate,self);
        }
        free(text); XtAddCallback(data->widget,XmNdestroyCallback,destroyed,data); XtManageChild(data->widget);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MMenuItemPeer_pEnable(Hsun_awt_motif_MMenuItemPeer *self)
{ Widget widget; AWT_LOCK(); widget=awt118_menu_widget((HObject *)self); if (widget) XtSetSensitive(widget,True); AWT_UNLOCK(); }
void sun_awt_motif_MMenuItemPeer_pDisable(Hsun_awt_motif_MMenuItemPeer *self)
{ Widget widget; AWT_LOCK(); widget=awt118_menu_widget((HObject *)self); if (widget) XtSetSensitive(widget,False); AWT_UNLOCK(); }
void sun_awt_motif_MMenuItemPeer_pDispose(Hsun_awt_motif_MMenuItemPeer *self)
{ AWT_LOCK(); dispose((HObject *)self); AWT_UNLOCK(); }
void sun_awt_motif_MMenuItemPeer_pSetLabel(Hsun_awt_motif_MMenuItemPeer *self,Hjava_lang_String *value)
{
    Widget widget; XmString string;
    AWT_LOCK(); widget=awt118_menu_widget((HObject *)self);
    if (widget && !XmIsSeparatorGadget(widget)) {
        string=label(value); XtUnmanageChild(widget); XtVaSetValues(widget,XmNlabelString,string,NULL);
        XtManageChild(widget); XmStringFree(string);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MCheckboxMenuItemPeer_pSetState(Hsun_awt_motif_MCheckboxMenuItemPeer *self,long state)
{ Widget widget; AWT_LOCK(); widget=awt118_menu_widget((HObject *)self); if (widget) XmToggleButtonSetState(widget,state!=0,False); AWT_UNLOCK(); }
long sun_awt_motif_MCheckboxMenuItemPeer_getState(Hsun_awt_motif_MCheckboxMenuItemPeer *self)
{ Widget widget; long state=0; AWT_LOCK(); widget=awt118_menu_widget((HObject *)self); if (widget) state=XmToggleButtonGetState(widget); AWT_UNLOCK(); return state; }
