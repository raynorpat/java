/* Solaris libawt.so 298dc..2a5a0: text callbacks, selection and echo storage. */
#include "awt118.h"
#include "sun_awt_motif_MTextFieldPeer.h"
#include <Xm/TextF.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    AwtPeer peer;
    char *password;
    unsigned char echo;
} AwtTextField;

static AwtTextField *field(Hsun_awt_motif_MTextFieldPeer *self)
{
    AwtTextField *data=(AwtTextField *)awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->peer.widget) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL;
    }
    return data;
}
static void cleanup_field(AwtPeer *peer)
{
    AwtTextField *data=(AwtTextField *)peer;
    free(data->password); data->password=NULL;
}
static void changed(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"valueChanged","()V");
    awt118_callback_done();
}
static void activated(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"action","()V");
    awt118_callback_done();
}
static void mapped(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch)
{
    if (event->type==MapNotify) {
        execute_java_dynamic_method(EE(),(HObject *)closure,"selectMapNotify","()V");
        awt118_callback_done();
    }
}
static void verify_echo(Widget widget,XtPointer closure,XtPointer call)
{
    AwtTextField *data=(AwtTextField *)closure;
    XmTextVerifyCallbackStruct *edit=(XmTextVerifyCallbackStruct *)call;
    size_t length=strlen(data->password),added=edit->text->length;
    char *next;
    /* The Solaris password widget allows append and trailing deletion only. */
    if (!added && edit->startPos==0 && edit->endPos==(XmTextPosition)length) {
        data->password[0]=0; return;
    }
    if (!added && edit->startPos==(XmTextPosition)length-1) {
        edit->endPos=length; data->password[edit->startPos]=0; return;
    }
    if (edit->startPos!=(XmTextPosition)length) { edit->doit=False; return; }
    next=(char *)realloc(data->password,length+added+1);
    if (!next) { edit->doit=False; SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return; }
    data->password=next;
    if (added) memcpy(next+length,edit->text->ptr,added);
    next[length+added]=0;
    if (added) memset(edit->text->ptr,data->echo,added);
}
void sun_awt_motif_MTextFieldPeer_create(Hsun_awt_motif_MTextFieldPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *owner;
    AwtTextField *data;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        data=(AwtTextField *)calloc(1,sizeof(*data));
        if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else {
            data->peer.references=1; data->peer.javaPeer=(Hsun_awt_motif_MComponentPeer *)self;
            data->peer.cleanup=cleanup_field; unhand(self)->pData=(long)data;
            data->peer.widget=XtVaCreateManagedWidget("textfield",xmTextFieldWidgetClass,owner->widget,
                XmNrecomputeSize,False,XmNhighlightThickness,1,XmNshadowThickness,2,
                XmNuserData,self,XmNmappedWhenManaged,False,NULL);
            XtAddCallback(data->peer.widget,XmNactivateCallback,activated,self);
            XtAddCallback(data->peer.widget,XmNvalueChangedCallback,changed,self);
            XtAddEventHandler(data->peer.widget,StructureNotifyMask,True,mapped,self);
        }
    }
    AWT_UNLOCK();
}
Hjava_lang_String *sun_awt_motif_MTextFieldPeer_getText(Hsun_awt_motif_MTextFieldPeer *self)
{
    AwtTextField *data;
    Hjava_lang_String *result=NULL;
    char *text;
    AWT_LOCK(); data=field(self);
    if (data) {
        text=data->password ? data->password : XmTextFieldGetString(data->peer.widget);
        result=makeJavaString(text,strlen(text));
        if (!data->password) XtFree(text);
    }
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MTextFieldPeer_setText(Hsun_awt_motif_MTextFieldPeer *self,Hjava_lang_String *string)
{
    AwtTextField *data;
    char *text=string ? allocCString(string) : NULL;
    AWT_LOCK(); data=field(self);
    if (data) {
        if (data->password) {
            XmTextFieldSetString(data->peer.widget,"");
            XmTextFieldInsert(data->peer.widget,0,text ? text : "");
            XmTextFieldSetInsertionPosition(data->peer.widget,text ? strlen(text) : 0);
        } else XmTextFieldSetString(data->peer.widget,text ? text : "");
        XFlush(awt_display);
    }
    AWT_UNLOCK(); free(text);
}
void sun_awt_motif_MTextFieldPeer_insertReplaceText(Hsun_awt_motif_MTextFieldPeer *self,Hjava_lang_String *string)
{
    AwtTextField *data;
    XmTextPosition start,end;
    char *text=string ? allocCString(string) : NULL;
    AWT_LOCK(); data=field(self);
    if (data) {
        if (!XmTextFieldGetSelectionPosition(data->peer.widget,&start,&end))
            start=end=XmTextFieldGetInsertionPosition(data->peer.widget);
        XmTextFieldReplace(data->peer.widget,start,end,text ? text : ""); XFlush(awt_display);
    }
    AWT_UNLOCK(); free(text);
}
void sun_awt_motif_MTextFieldPeer_setEchoChar(Hsun_awt_motif_MTextFieldPeer *self,unicode echo)
{
    AwtTextField *data;
    char *display;
    AWT_LOCK(); data=field(self);
    if (data) {
        display=XmTextFieldGetString(data->peer.widget);
        if (!data->password) {
            data->password=(char *)malloc(strlen(display)+1);
            if (data->password) strcpy(data->password,display);
        } else XtRemoveCallback(data->peer.widget,XmNmodifyVerifyCallback,verify_echo,data);
        if (!data->password) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else {
            data->echo=echo;
            memset(display,echo,strlen(display));
            XmTextFieldSetString(data->peer.widget,display);
            XtAddCallback(data->peer.widget,XmNmodifyVerifyCallback,verify_echo,data);
        }
        XtFree(display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MTextFieldPeer_preDispose(Hsun_awt_motif_MTextFieldPeer *self)
{
    AwtTextField *data;
    AWT_LOCK(); data=field(self);
    if (data) {
        XtRemoveCallback(data->peer.widget,XmNmodifyVerifyCallback,verify_echo,data);
        cleanup_field(&data->peer);
        /* Unmanaging lets the host Motif retire its own text selection timer. */
        XtUnmanageChild(data->peer.widget);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MTextFieldPeer_pSetEditable(Hsun_awt_motif_MTextFieldPeer *self,long editable)
{
    AwtTextField *data;
    AWT_LOCK(); data=field(self);
    if (data) XtVaSetValues(data->peer.widget,XmNeditable,editable!=0,XmNcursorPositionVisible,editable!=0,NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MTextFieldPeer_pSelect(Hsun_awt_motif_MTextFieldPeer *self,long start,long end)
{
    AwtTextField *data;
    AWT_LOCK(); data=field(self);
    if (data) XmTextFieldSetSelection(data->peer.widget,start,end,CurrentTime);
    XFlush(awt_display); AWT_UNLOCK();
}
static long selection(Hsun_awt_motif_MTextFieldPeer *self,int last)
{
    AwtTextField *data;
    XmTextPosition start,end;
    long result=0;
    AWT_LOCK(); data=field(self);
    if (data) {
        if (!XmTextFieldGetSelectionPosition(data->peer.widget,&start,&end) || start==end)
            result=XmTextFieldGetInsertionPosition(data->peer.widget);
        else result=last ? end : start;
    }
    AWT_UNLOCK(); return result;
}
long sun_awt_motif_MTextFieldPeer_getSelectionStart(Hsun_awt_motif_MTextFieldPeer *self) { return selection(self,0); }
long sun_awt_motif_MTextFieldPeer_getSelectionEnd(Hsun_awt_motif_MTextFieldPeer *self) { return selection(self,1); }
long sun_awt_motif_MTextFieldPeer_getCaretPosition(Hsun_awt_motif_MTextFieldPeer *self)
{
    AwtTextField *data; long result=0;
    AWT_LOCK(); data=field(self);
    if (data) result=XmTextFieldGetInsertionPosition(data->peer.widget);
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MTextFieldPeer_setCaretPosition(Hsun_awt_motif_MTextFieldPeer *self,long position)
{
    AwtTextField *data;
    AWT_LOCK(); data=field(self);
    if (data) XmTextFieldSetInsertionPosition(data->peer.widget,position);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MTextFieldPeer_setFont(Hsun_awt_motif_MTextFieldPeer *self,struct Hjava_awt_Font *font)
{ sun_awt_motif_MComponentPeer_pSetFont((Hsun_awt_motif_MComponentPeer *)self,font); }
