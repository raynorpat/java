/* Solaris libawt.so 26d80..27318: scrollbar resources and callbacks. */
#include "awt118.h"
#include "sun_awt_motif_MScrollbarPeer.h"
#include "java_awt_Scrollbar.h"
#include <Xm/ScrollBar.h>

static void adjusted(Widget widget,XtPointer closure,XtPointer call)
{
    XmScrollBarCallbackStruct *event=(XmScrollBarCallbackStruct *)call;
    char *method;
    switch (event->reason) {
    case XmCR_DECREMENT: method="lineUp"; break;
    case XmCR_INCREMENT: method="lineDown"; break;
    case XmCR_PAGE_DECREMENT: method="pageUp"; break;
    case XmCR_PAGE_INCREMENT: method="pageDown"; break;
    default: method="dragAbsolute"; break;
    }
    execute_java_dynamic_method(EE(),(HObject *)closure,method,"(I)V",event->value);
    awt118_callback_done();
}
void sun_awt_motif_MScrollbarPeer_create(Hsun_awt_motif_MScrollbarPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Hjava_awt_Scrollbar *target=(Hjava_awt_Scrollbar *)unhand(self)->target;
    static const char *callbacks[]={XmNdecrementCallback,XmNincrementCallback,
        XmNpageDecrementCallback,XmNpageIncrementCallback,XmNdragCallback,XmNtoTopCallback,XmNtoBottomCallback};
    int i;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget || !target) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (unhand(target)->orientation!=0 && unhand(target)->orientation!=1)
        SignalError(EE(),"java/lang/IllegalArgumentException","bad scrollbar orientation");
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        data->widget=XtVaCreateManagedWidget("scrollbar",xmScrollBarWidgetClass,owner->widget,
            XmNorientation,unhand(target)->orientation==0 ? XmHORIZONTAL : XmVERTICAL,
            XmNminimum,unhand(target)->minimum,XmNmaximum,unhand(target)->maximum,
            XmNvalue,unhand(target)->value,XmNsliderSize,unhand(target)->visibleAmount,
            XmNincrement,unhand(target)->lineIncrement,XmNpageIncrement,unhand(target)->pageIncrement,
            XmNmappedWhenManaged,False,NULL);
        for (i=0;i<sizeof(callbacks)/sizeof(callbacks[0]);i++)
            XtAddCallback(data->widget,callbacks[i],adjusted,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MScrollbarPeer_pSetValues(Hsun_awt_motif_MScrollbarPeer *self,long value,long visible,long minimum,long maximum)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        XtVaSetValues(data->widget,XmNminimum,minimum,XmNmaximum,maximum,XmNvalue,value,XmNsliderSize,visible,NULL);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MScrollbarPeer_setLineIncrement(Hsun_awt_motif_MScrollbarPeer *self,long increment)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else { XtVaSetValues(data->widget,XmNincrement,increment,NULL); XFlush(awt_display); }
    AWT_UNLOCK();
}
void sun_awt_motif_MScrollbarPeer_setPageIncrement(Hsun_awt_motif_MScrollbarPeer *self,long increment)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else { XtVaSetValues(data->widget,XmNpageIncrement,increment,NULL); XFlush(awt_display); }
    AWT_UNLOCK();
}
