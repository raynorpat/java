/* Solaris libawt.so 2625c..26b80; debug Choice_callback 4c1c0. */
#include "awt118.h"
#include "sun_awt_motif_MChoicePeer.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    AwtPeer peer;
    Widget menu;
    Widget *items;
    int count,capacity,columns;
} AwtChoice;

static AwtChoice *choice(Hsun_awt_motif_MChoicePeer *self)
{
    AwtChoice *data=(AwtChoice *)awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->peer.widget || !data->menu) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL;
    }
    return data;
}
static void menu_destroyed(Widget widget,XtPointer closure,XtPointer call)
{ ((AwtChoice *)closure)->menu=NULL; }
static void cleanup_choice(AwtPeer *peer)
{
    AwtChoice *data=(AwtChoice *)peer;
    if (data->menu) {
        XtRemoveCallback(data->menu,XmNdestroyCallback,menu_destroyed,data);
        XtDestroyWidget(XtParent(data->menu)); data->menu=NULL;
    }
    free(data->items); data->items=NULL; data->count=data->capacity=0;
}
static void chosen(Widget widget,XtPointer closure,XtPointer call)
{
    XtPointer index;
    XtVaGetValues(widget,XmNuserData,&index,NULL);
    execute_java_dynamic_method(EE(),(HObject *)closure,"action","(I)V",(long)index-1);
    awt118_callback_done();
}
static void fit_menu(AwtChoice *data,int shrink)
{
    Dimension height;
    if (shrink && data->columns>1) {
        data->columns--;
        XtVaSetValues(data->menu,XmNnumColumns,data->columns,NULL);
    }
    XtVaGetValues(data->menu,XmNheight,&height,NULL);
    if (height>DisplayHeight(awt_display,awt_screen)) {
        data->columns++;
        XtVaSetValues(data->menu,XmNnumColumns,data->columns,NULL);
    }
}
void sun_awt_motif_MChoicePeer_create(Hsun_awt_motif_MChoicePeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtChoice *data;
    AwtPeer *owner;
    Arg args[8];
    int n=0;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        data=(AwtChoice *)calloc(1,sizeof(*data));
        if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else {
            data->peer.references=1; data->peer.javaPeer=(Hsun_awt_motif_MComponentPeer *)self;
            data->peer.cleanup=cleanup_choice; data->columns=1;
            unhand(self)->pData=(long)data;
            XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
            XtSetArg(args[n],XmNnumColumns,1); n++;
            data->menu=XmCreatePulldownMenu(owner->widget,"pulldown",args,n);
            XtAddCallback(data->menu,XmNdestroyCallback,menu_destroyed,data);
            n=0;
            XtSetArg(args[n],XmNsubMenuId,data->menu); n++;
            XtSetArg(args[n],XmNmarginWidth,0); n++;
            XtSetArg(args[n],XmNmarginHeight,0); n++;
            XtSetArg(args[n],XmNspacing,0); n++;
            XtSetArg(args[n],XmNresizeWidth,False); n++;
            XtSetArg(args[n],XmNresizeHeight,False); n++;
            XtSetArg(args[n],XmNtraversalOn,True); n++;
            data->peer.widget=XmCreateOptionMenu(owner->widget,"choice",args,n);
            XtUnmanageChild(XmOptionLabelGadget(data->peer.widget));
            XtSetMappedWhenManaged(data->peer.widget,False);
            XtManageChild(data->peer.widget);
        }
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_pSelect(Hsun_awt_motif_MChoicePeer *self,long index)
{
    AwtChoice *data;
    AWT_LOCK(); data=choice(self);
    if (data) {
        if (index<0 || index>=data->count) SignalError(EE(),"java/lang/IllegalArgumentException",NULL);
        else XtVaSetValues(data->peer.widget,XmNmenuHistory,data->items[index],NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_addItem(Hsun_awt_motif_MChoicePeer *self,Hjava_lang_String *value,long index)
{
    AwtChoice *data;
    Widget *items,widget;
    Pixel foreground,background;
    XmString label;
    char *text;
    int i;
    if (!value) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=choice(self);
    if (data) {
        if (index<0 || index>data->count) SignalError(EE(),"java/lang/IllegalArgumentException",NULL);
        else {
            if (data->count==data->capacity) {
                items=(Widget *)realloc(data->items,(data->capacity+20)*sizeof(*items));
                if (!items) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); AWT_UNLOCK(); return; }
                data->items=items; data->capacity+=20;
            }
            text=allocCString(value); label=XmStringCreateLocalized(text);
            XtVaGetValues(data->peer.widget,XmNforeground,&foreground,XmNbackground,&background,NULL);
            widget=XtVaCreateManagedWidget("item",xmPushButtonWidgetClass,data->menu,
                XmNlabelString,label,XmNpositionIndex,index,XmNuserData,(XtPointer)(index+1),
                XmNforeground,foreground,XmNbackground,background,NULL);
            XmStringFree(label); free(text);
            memmove(data->items+index+1,data->items+index,(data->count-index)*sizeof(Widget));
            data->items[index]=widget; data->count++;
            for (i=index+1;i<data->count;i++) XtVaSetValues(data->items[i],XmNuserData,(XtPointer)(long)(i+1),NULL);
            XtAddCallback(widget,XmNactivateCallback,chosen,self);
            fit_menu(data,0);
        }
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_remove(Hsun_awt_motif_MChoicePeer *self,long index)
{
    AwtChoice *data;
    int i;
    AWT_LOCK(); data=choice(self);
    if (data) {
        if (index<0 || index>=data->count) SignalError(EE(),"java/lang/IllegalArgumentException",NULL);
        else {
            XtRemoveCallback(data->items[index],XmNactivateCallback,chosen,self);
            XtDestroyWidget(data->items[index]);
            data->count--;
            memmove(data->items+index,data->items+index+1,(data->count-index)*sizeof(Widget));
            for (i=index;i<data->count;i++) XtVaSetValues(data->items[i],XmNuserData,(XtPointer)(long)(i+1),NULL);
            fit_menu(data,1);
        }
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_pReshape(Hsun_awt_motif_MChoicePeer *self,long x,long y,long width,long height)
{
    AwtChoice *data;
    Hjava_awt_Component *target=unhand(self)->target;
    AwtPeer *parent=NULL;
    AWT_LOCK(); data=choice(self);
    if (unhand(target)->parent) parent=awt118_peer((Hsun_awt_motif_MComponentPeer *)unhand((Hjava_awt_Component *)unhand(target)->parent)->peer);
    if (parent) { x-=parent->decorLeft; y-=parent->decorTop+parent->menuHeight+parent->warningHeight; }
    if (data) {
        Widget button=XmOptionButtonGadget(data->peer.widget);
        /* Solaris awt_util_reshape temporarily unmanages each child. Avoid
         * re-entering RowColumn's geometry manager while resizing its gadget. */
        XtUnmanageChild(data->peer.widget);
        XtVaSetValues(data->peer.widget,XmNx,x,XmNy,y,XmNwidth,width>0 ? width : 1,XmNheight,height>0 ? height : 1,NULL);
        XtManageChild(data->peer.widget);
        XtUnmanageChild(button);
        XtVaSetValues(button,XmNx,0,XmNy,0,
            XmNwidth,width>0 ? width : 1,XmNheight,height>0 ? height : 1,NULL);
        XtManageChild(button);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_setFont(Hsun_awt_motif_MChoicePeer *self,struct Hjava_awt_Font *font)
{
    AwtChoice *data;
    XFontStruct *xfont;
    XmFontList list;
    int i;
    AWT_LOCK(); data=choice(self); xfont=awt118_font(font);
    if (data && xfont) {
        list=XmFontListCreate(xfont,XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(data->peer.widget,XmNfontList,list,NULL);
        XtVaSetValues(data->menu,XmNfontList,list,NULL);
        for (i=0;i<data->count;i++) XtVaSetValues(data->items[i],XmNfontList,list,NULL);
        XmFontListFree(list);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_setBackground(Hsun_awt_motif_MChoicePeer *self,Hjava_awt_Color *color)
{
    AwtChoice *data;
    Pixel pixel,foreground;
    int i;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException","null color"); return; }
    AWT_LOCK(); data=choice(self);
    if (data) {
        pixel=awt118_pixel(unhand(color)->value);
        XtVaGetValues(data->peer.widget,XmNforeground,&foreground,NULL);
        XmChangeColor(data->peer.widget,pixel); XtVaSetValues(data->peer.widget,XmNforeground,foreground,NULL);
        XmChangeColor(data->menu,pixel); XtVaSetValues(data->menu,XmNforeground,foreground,NULL);
        for (i=0;i<data->count;i++) { XmChangeColor(data->items[i],pixel); XtVaSetValues(data->items[i],XmNforeground,foreground,NULL); }
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MChoicePeer_setForeground(Hsun_awt_motif_MChoicePeer *self,Hjava_awt_Color *color)
{
    AwtChoice *data;
    Pixel pixel;
    int i;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException","null color"); return; }
    AWT_LOCK(); data=choice(self);
    if (data) {
        pixel=awt118_pixel(unhand(color)->value);
        XtVaSetValues(data->peer.widget,XmNforeground,pixel,NULL);
        XtVaSetValues(data->menu,XmNforeground,pixel,NULL);
        for (i=0;i<data->count;i++) XtVaSetValues(data->items[i],XmNforeground,pixel,NULL);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
