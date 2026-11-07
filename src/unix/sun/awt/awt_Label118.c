/* Solaris libawt.so 2bff0/2c0ac/2c208: label creation and resources. */
#include "awt118.h"
#include "sun_awt_motif_MLabelPeer.h"
#include <Xm/Label.h>
#include <stdlib.h>
#include <string.h>

void sun_awt_motif_MLabelPeer_create(Hsun_awt_motif_MLabelPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        data->widget=XtVaCreateManagedWidget("label",xmLabelWidgetClass,owner->widget,
            XmNhighlightThickness,0,XmNalignment,XmALIGNMENT_BEGINNING,XmNrecomputeSize,False,
            XmNtraversalOn,True,XmNmappedWhenManaged,False,NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MLabelPeer_setText(Hsun_awt_motif_MLabelPeer *self,Hjava_lang_String *value)
{
    AwtPeer *data;
    char *text,*newline;
    XmString label;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        text=value ? allocCString(value) : NULL;
        newline=text ? strchr(text,'\n') : NULL;
        if (newline) *newline=0;
        label=XmStringCreateLocalized(text ? text : "");
        XtVaSetValues(data->widget,XmNlabelString,label,NULL);
        XmStringFree(label); free(text); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MLabelPeer_setAlignment(Hsun_awt_motif_MLabelPeer *self,long alignment)
{
    AwtPeer *data;
    static const unsigned char values[]={XmALIGNMENT_BEGINNING,XmALIGNMENT_CENTER,XmALIGNMENT_END};
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (alignment>=0 && alignment<3) {
        XtVaSetValues(data->widget,XmNalignment,values[alignment],NULL); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
