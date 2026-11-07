/* Solaris libawt.so 27464..280f4: scroll policy, insets and adjustments. */
#include "awt118.h"
#include "sun_awt_motif_MScrollPanePeer.h"
#include "java_awt_ScrollPane.h"
#include "java_awt_Insets.h"
#include <X11/IntrinsicP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/Manager.h>

static AwtPeer *pane(Hsun_awt_motif_MScrollPanePeer *self)
{
    AwtPeer *data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL;
    }
    return data;
}
static Widget scrollbar(AwtPeer *data,int vertical)
{
    Widget bar=NULL;
    if (data && XmIsScrolledWindow(data->widget))
        XtVaGetValues(data->widget,vertical ? XmNverticalScrollBar : XmNhorizontalScrollBar,&bar,NULL);
    return bar;
}
static void adjusted(Widget widget,XtPointer closure,XtPointer call)
{
    Hsun_awt_motif_MScrollPanePeer *self=(Hsun_awt_motif_MScrollPanePeer *)closure;
    XmScrollBarCallbackStruct *event=(XmScrollBarCallbackStruct *)call;
    unsigned char orientation;
    int type;
    switch (event->reason) {
    case XmCR_INCREMENT: type=1; break;
    case XmCR_DECREMENT: type=2; break;
    case XmCR_PAGE_INCREMENT: type=4; break;
    case XmCR_PAGE_DECREMENT: type=3; break;
    case XmCR_VALUE_CHANGED: case XmCR_TO_TOP: case XmCR_TO_BOTTOM: case XmCR_DRAG: type=5; break;
    default: type=0;
    }
    if (event->reason==XmCR_DRAG) unhand(self)->dragInProgress=1;
    XtVaGetValues(widget,XmNorientation,&orientation,NULL);
    execute_java_dynamic_method(EE(),(HObject *)self,
        orientation==XmVERTICAL ? "scrolledVertical" : "scrolledHorizontal","(II)V",event->value,type);
    awt118_callback_done();
    if (event->reason==XmCR_VALUE_CHANGED) unhand(self)->dragInProgress=0;
}
void sun_awt_motif_MScrollPanePeer_create(Hsun_awt_motif_MScrollPanePeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *owner,*data;
    Hjava_awt_ScrollPane *target=(Hjava_awt_ScrollPane *)unhand(self)->target;
    Widget bar;
    Arg args[5];
    int policy,n=0,i,j;
    static const char *callbacks[]={XmNincrementCallback,XmNdecrementCallback,
        XmNpageIncrementCallback,XmNpageDecrementCallback,XmNtoBottomCallback,XmNtoTopCallback,XmNdragCallback};
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget || !target) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        policy=unhand(target)->scrollbarDisplayPolicy;
        XtSetArg(args[n],XmNuserData,self); n++;
        if (policy==2) data->widget=XtCreateWidget("scrolledWindow",xmManagerWidgetClass,owner->widget,args,n);
        else {
            XtSetArg(args[n],XmNscrollBarDisplayPolicy,policy==1 ? XmSTATIC : XmAS_NEEDED); n++;
            XtSetArg(args[n],XmNscrollingPolicy,XmAUTOMATIC); n++;
            XtSetArg(args[n],XmNvisualPolicy,XmCONSTANT); n++;
            XtSetArg(args[n],XmNspacing,0); n++;
            data->widget=XmCreateScrolledWindow(owner->widget,"scroller",args,n);
            for (i=0;i<2;i++) {
                bar=scrollbar(data,i);
                if (bar) {
                    for (j=0;j<sizeof(callbacks)/sizeof(callbacks[0]);j++) XtAddCallback(bar,callbacks[j],adjusted,self);
                    XtVaSetValues(bar,XmNhighlightThickness,0,NULL);
                }
            }
        }
        XtSetMappedWhenManaged(data->widget,False); XtManageChild(data->widget);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MScrollPanePeer_pSetScrollChild(Hsun_awt_motif_MScrollPanePeer *self,Hsun_awt_motif_MComponentPeer *child)
{
    AwtPeer *data,*content;
    AWT_LOCK(); data=pane(self); content=awt118_peer(child);
    if (!content || !content->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (data && XmIsScrolledWindow(data->widget)) XmScrolledWindowSetAreas(data->widget,NULL,NULL,content->widget);
    AWT_UNLOCK();
}
void sun_awt_motif_MScrollPanePeer_pSetIncrement(Hsun_awt_motif_MScrollPanePeer *self,long orientation,long block,long increment)
{
    Widget bar;
    AWT_LOCK(); bar=scrollbar(pane(self),orientation==1);
    if (bar) XtVaSetValues(bar,block ? XmNpageIncrement : XmNincrement,increment,NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
long sun_awt_motif_MScrollPanePeer_pGetScrollbarSpace(Hsun_awt_motif_MScrollPanePeer *self,long orientation)
{
    AwtPeer *data;
    Widget bar;
    Dimension size=0,spacing=0,highlight=0;
    AWT_LOCK(); data=pane(self); bar=scrollbar(data,orientation==1);
    if (bar) {
        XtVaGetValues(data->widget,XmNspacing,&spacing,NULL);
        XtVaGetValues(bar,orientation==1 ? XmNwidth : XmNheight,&size,XmNhighlightThickness,&highlight,NULL);
    }
    AWT_UNLOCK(); return size+spacing+2*highlight;
}
long sun_awt_motif_MScrollPanePeer_pGetBlockIncrement(Hsun_awt_motif_MScrollPanePeer *self,long orientation)
{
    Widget bar;
    int increment=0;
    AWT_LOCK(); bar=scrollbar(pane(self),orientation==1);
    if (bar) XtVaGetValues(bar,XmNpageIncrement,&increment,NULL);
    AWT_UNLOCK(); return increment;
}
Hjava_awt_Insets *sun_awt_motif_MScrollPanePeer_pInsets(Hsun_awt_motif_MScrollPanePeer *self,long width,long height,long childWidth,long childHeight)
{
    AwtPeer *data;
    Hjava_awt_Insets *result=NULL;
    Widget hbar,vbar;
    Dimension shadow=0,marginWidth=0,marginHeight=0,spacing=0,hsize=0,vsize=0,hlight=0,vlight=0;
    unsigned char placement=XmBOTTOM_RIGHT;
    int policy,horizontal,vertical,top,left,bottom,right,swap;
    AWT_LOCK(); data=pane(self);
    if (data) {
        policy=unhand((Hjava_awt_ScrollPane *)unhand(self)->target)->scrollbarDisplayPolicy;
        XtVaGetValues(data->widget,XmNshadowThickness,&shadow,NULL);
        if (policy!=2) {
            XtVaGetValues(data->widget,XmNscrollBarPlacement,&placement,XmNspacing,&spacing,
                XmNscrolledWindowMarginWidth,&marginWidth,XmNscrolledWindowMarginHeight,&marginHeight,NULL);
            hbar=scrollbar(data,0); vbar=scrollbar(data,1);
            XtVaGetValues(hbar,XmNheight,&hsize,XmNhighlightThickness,&hlight,NULL);
            XtVaGetValues(vbar,XmNwidth,&vsize,XmNhighlightThickness,&vlight,NULL);
        }
        horizontal=policy==1 || (policy==0 && childWidth>width-2*shadow);
        vertical=policy==1 || (policy==0 && childHeight>height-2*shadow);
        if (policy==0) {
            if (!horizontal && vertical && childWidth>width-2*shadow-vsize-vlight-spacing) horizontal=1;
            else if (!vertical && horizontal && childHeight>height-2*shadow-hsize-hlight-spacing) vertical=1;
        }
        top=shadow+marginHeight+(vertical ? vlight : 0);
        bottom=shadow+marginHeight+(horizontal ? hsize+hlight+spacing : vertical ? vlight : 0);
        left=shadow+marginWidth+(horizontal ? hlight : 0);
        right=shadow+marginWidth+(vertical ? vsize+vlight+spacing : horizontal ? hlight : 0);
        if (placement==XmTOP_LEFT || placement==XmTOP_RIGHT) { swap=top; top=bottom; bottom=swap; }
        if (placement==XmTOP_LEFT || placement==XmBOTTOM_LEFT) { swap=left; left=right; right=swap; }
        result=(Hjava_awt_Insets *)execute_java_constructor(EE(),"java/awt/Insets",NULL,"(IIII)",top,left,bottom,right);
    }
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MScrollPanePeer_setScrollPosition(Hsun_awt_motif_MScrollPanePeer *self,long x,long y)
{
    AwtPeer *data;
    Widget bar;
    WidgetList children;
    Cardinal count;
    int i,value,minimum,maximum,slider,increment,page;
    AWT_LOCK(); data=pane(self);
    if (data) {
        if (!XmIsScrolledWindow(data->widget)) {
            XtVaGetValues(data->widget,XmNchildren,&children,XmNnumChildren,&count,NULL);
            if (count) XtMoveWidget(children[0],-x,-y);
            else SignalError(EE(),"java/lang/NullPointerException",NULL);
        } else for (i=1;i>=0;i--) {
            bar=scrollbar(data,i);
            if (!bar) continue;
            XtVaGetValues(bar,XmNminimum,&minimum,XmNmaximum,&maximum,XmNsliderSize,&slider,
                XmNincrement,&increment,XmNpageIncrement,&page,NULL);
            value=i ? y : x;
            if (i && value>maximum-slider) value=maximum-slider;
            if (!i && value<minimum) value=minimum;
            XmScrollBarSetValues(bar,value,slider,increment,page,True);
        }
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
