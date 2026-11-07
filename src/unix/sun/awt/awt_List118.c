/* Solaris libawt.so 2b780..2bed0; debug Slist_callback 5b860. */
#include "awt118.h"
#include "sun_awt_motif_MListPeer.h"
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <stdlib.h>

static void selected(Widget widget,XtPointer closure,XtPointer call)
{
    XmListCallbackStruct *event=(XmListCallbackStruct *)call;
    if (event->reason!=XmCR_DEFAULT_ACTION || (event->event && event->event->type==KeyPress)) {
        execute_java_dynamic_method(EE(),(HObject *)closure,"handleListChanged","(I)V",event->item_position-1);
        awt118_callback_done();
    }
    if (event->reason==XmCR_DEFAULT_ACTION) {
        execute_java_dynamic_method(EE(),(HObject *)closure,"action","(I)V",event->item_position-1);
        awt118_callback_done();
    }
}
static Widget list_widget(Hsun_awt_motif_MListPeer *self)
{
    AwtPeer *data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->content) { SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL; }
    return data->content;
}
void sun_awt_motif_MListPeer_create(Hsun_awt_motif_MListPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Arg args[8];
    int n=0;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        XtSetArg(args[n],XmNlistSizePolicy,XmCONSTANT); n++;
        XtSetArg(args[n],XmNlistMarginWidth,0); n++;
        XtSetArg(args[n],XmNlistMarginHeight,0); n++;
        XtSetArg(args[n],XmNmarginWidth,0); n++;
        XtSetArg(args[n],XmNmarginHeight,0); n++;
        data->content=XmCreateScrolledList(owner->widget,"slist",args,n);
        data->widget=XtParent(data->content);
        XtSetMappedWhenManaged(data->widget,False);
        XtAddCallback(data->content,XmNdefaultActionCallback,selected,self);
        XtManageChild(data->content); XtManageChild(data->widget);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MListPeer_pDispose(Hsun_awt_motif_MListPeer *self)
{ sun_awt_motif_MComponentPeer_pDispose((Hsun_awt_motif_MComponentPeer *)self); }
void sun_awt_motif_MListPeer_setMultipleSelections(Hsun_awt_motif_MListPeer *self,long multiple)
{
    Widget widget;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) {
        XtRemoveCallback(widget,XmNbrowseSelectionCallback,selected,self);
        XtRemoveCallback(widget,XmNmultipleSelectionCallback,selected,self);
        XtVaSetValues(widget,XmNselectionPolicy,multiple ? XmMULTIPLE_SELECT : XmBROWSE_SELECT,NULL);
        XtAddCallback(widget,multiple ? XmNmultipleSelectionCallback : XmNbrowseSelectionCallback,selected,self);
    }
    AWT_UNLOCK();
}
long sun_awt_motif_MListPeer_isSelected(Hsun_awt_motif_MListPeer *self,long index)
{
    Widget widget;
    long result=0;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) result=XmListPosSelected(widget,index+1)!=False;
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MListPeer_addItem(Hsun_awt_motif_MListPeer *self,Hjava_lang_String *value,long index)
{
    Widget widget;
    char *text;
    XmString item;
    if (!value) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); widget=list_widget(self);
    if (widget) {
        text=allocCString(value); item=XmStringCreateLocalized(text);
        XmListAddItemUnselected(widget,item,index+1);
        XmStringFree(item); free(text); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MListPeer_delItems(Hsun_awt_motif_MListPeer *self,long first,long last)
{
    Widget widget;
    int count;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) {
        XtVaGetValues(widget,XmNitemCount,&count,NULL);
        if (first==0 && last>=count-1) XmListDeleteAllItems(widget);
        else if (last>=first) XmListDeleteItemsPos(widget,last-first+1,first+1);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MListPeer_makeVisible(Hsun_awt_motif_MListPeer *self,long index)
{
    Widget widget;
    int top;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) {
        XtVaGetValues(widget,XmNtopItemPosition,&top,NULL);
        if (index+1>=top) XmListSetBottomPos(widget,index+1);
        else XmListSetPos(widget,index+1);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MListPeer_pSelect(Hsun_awt_motif_MListPeer *self,long index)
{
    Widget widget;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) XmListSelectPos(widget,index+1,False);
    AWT_UNLOCK();
}
void sun_awt_motif_MListPeer_pDeselect(Hsun_awt_motif_MListPeer *self,long index)
{
    Widget widget;
    AWT_LOCK(); widget=list_widget(self);
    if (widget) XmListDeselectPos(widget,index+1);
    AWT_UNLOCK();
}
