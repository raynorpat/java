/* Solaris libawt_g.so 482e0..48f78; release 24d38..25044. */
#include "awt118.h"
#include "java_awt_Checkbox.h"
#include "sun_awt_motif_MCheckboxPeer.h"
#include <Xm/ToggleB.h>
#include <stdlib.h>

static void toggled(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"action","(Z)V",XmToggleButtonGetState(widget)!=False);
    awt118_callback_done();
}
void sun_awt_motif_MCheckboxPeer_create(Hsun_awt_motif_MCheckboxPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Hjava_awt_Checkbox *target=(Hjava_awt_Checkbox *)unhand(self)->target;
    XmString label;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget || !target) SignalError(EE(),"java/lang/NullPointerException","null parent");
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        label=awt118_string(unhand(target)->label,(HObject *)target);
        data->widget=XtVaCreateManagedWidget("checkbox",xmToggleButtonWidgetClass,owner->widget,
            XmNlabelString,label,XmNrecomputeSize,False,XmNvisibleWhenOff,True,
            XmNtraversalOn,True,XmNspacing,0,XmNmappedWhenManaged,False,NULL);
        XmStringFree(label);
        XtAddCallback(data->widget,XmNvalueChangedCallback,toggled,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MCheckboxPeer_setCheckboxGroup(Hsun_awt_motif_MCheckboxPeer *self,struct Hjava_awt_CheckboxGroup *group)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else { XtVaSetValues(data->widget,XmNindicatorType,group ? XmONE_OF_MANY : XmN_OF_MANY,NULL); XFlush(awt_display); }
    AWT_UNLOCK();
}
void sun_awt_motif_MCheckboxPeer_pSetState(Hsun_awt_motif_MCheckboxPeer *self,long state)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else { XtVaSetValues(data->widget,XmNset,state!=0,NULL); XFlush(awt_display); }
    AWT_UNLOCK();
}
long sun_awt_motif_MCheckboxPeer_pGetState(Hsun_awt_motif_MCheckboxPeer *self)
{
    AwtPeer *data;
    long result=0;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else result=XmToggleButtonGetState(data->widget)!=False;
    AWT_UNLOCK();
    return result;
}
void sun_awt_motif_MCheckboxPeer_setLabel(Hsun_awt_motif_MCheckboxPeer *self,Hjava_lang_String *value)
{
    AwtPeer *data;
    XmString label;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        label=awt118_string(value,(HObject *)unhand(self)->target);
        XtVaSetValues(data->widget,XmNlabelString,label,NULL);
        XmStringFree(label); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
