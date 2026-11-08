/* Solaris libawt_g.so 44040/440a0/44570: push button and activation callback. */
#include "awt118.h"
#include "java_awt_Button.h"
#include "sun_awt_motif_MButtonPeer.h"
#include <Xm/PushB.h>
#include <stdlib.h>

static void activated(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"action","()V");
    awt118_callback_done();
}
void sun_awt_motif_MButtonPeer_create(Hsun_awt_motif_MButtonPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Hjava_awt_Button *target=(Hjava_awt_Button *)unhand(self)->target;
    XmString label;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        label=awt118_string(unhand(target)->label,(HObject *)target);
        data->widget=XtVaCreateManagedWidget("button",xmPushButtonWidgetClass,owner->widget,
            XmNlabelString,label,XmNrecomputeSize,False,XmNmappedWhenManaged,False,
            XmNmarginWidth,0,XmNmarginHeight,0,NULL);
        XmStringFree(label);
        XtAddCallback(data->widget,XmNactivateCallback,activated,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MButtonPeer_setLabel(Hsun_awt_motif_MButtonPeer *self,Hjava_lang_String *value)
{
    AwtPeer *data;
    XmString label;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->widget) {
        label=awt118_string(value,(HObject *)unhand(self)->target);
        XtVaSetValues(data->widget,XmNlabelString,label,NULL);
        XmStringFree(label); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
