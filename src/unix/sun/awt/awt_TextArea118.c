/* Solaris libawt.so 23da4..24bf0: compound scrolled text peer. */
#include "awt118.h"
#include "sun_awt_motif_MTextAreaPeer.h"
#include "java_awt_TextArea.h"
#include "java_awt_Color.h"
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <stdlib.h>
#include <string.h>

static AwtPeer *area(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget || !data->content) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL;
    }
    return data;
}
static void changed(Widget widget,XtPointer closure,XtPointer call)
{
    execute_java_dynamic_method(EE(),(HObject *)closure,"valueChanged","()V");
    awt118_callback_done();
}
static void mapped(Widget widget,XtPointer closure,XEvent *event,Boolean *dispatch)
{
    if (event->type==MapNotify) {
        execute_java_dynamic_method(EE(),(HObject *)closure,"selectMapNotify","()V");
        awt118_callback_done();
    }
}
void sun_awt_motif_MTextAreaPeer_create(Hsun_awt_motif_MTextAreaPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *owner,*data;
    Hjava_awt_TextArea *target=(Hjava_awt_TextArea *)unhand(self)->target;
    int policy,n=0;
    Arg args[11];
    Pixel background;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->widget || !target) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        policy=unhand(target)->scrollbarVisibility;
        XtVaGetValues(owner->widget,XmNbackground,&background,NULL);
        XtSetArg(args[n],XmNresizeWidth,False); n++;
        XtSetArg(args[n],XmNx,0); n++;
        XtSetArg(args[n],XmNy,0); n++;
        XtSetArg(args[n],XmNbackground,background); n++;
        XtSetArg(args[n],XmNeditMode,XmMULTI_LINE_EDIT); n++;
        XtSetArg(args[n],XmNwordWrap,policy==1 || policy==3); n++;
        XtSetArg(args[n],XmNscrollHorizontal,policy==0 || policy==2); n++;
        XtSetArg(args[n],XmNscrollVertical,policy==0 || policy==1); n++;
        XtSetArg(args[n],XmNmarginHeight,2); n++;
        XtSetArg(args[n],XmNmarginWidth,2); n++;
        XtSetArg(args[n],XmNuserData,self); n++;
        data->content=XmCreateScrolledText(owner->widget,"texta",args,n);
        data->widget=XtParent(data->content);
        XtVaSetValues(data->widget,XmNwidth,1,XmNheight,1,NULL);
        XtSetMappedWhenManaged(data->widget,False);
        XtManageChild(data->content); XtManageChild(data->widget);
        XtAddCallback(data->content,XmNvalueChangedCallback,changed,self);
        XtAddEventHandler(data->content,StructureNotifyMask,True,mapped,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MTextAreaPeer_pDispose(Hsun_awt_motif_MTextAreaPeer *self)
{ sun_awt_motif_MComponentPeer_pDispose((Hsun_awt_motif_MComponentPeer *)self); }
void sun_awt_motif_MTextAreaPeer_pSetEditable(Hsun_awt_motif_MTextAreaPeer *self,long editable)
{
    AwtPeer *data;
    AWT_LOCK(); data=area(self);
    if (data) XtVaSetValues(data->content,XmNeditable,editable!=0,XmNcursorPositionVisible,editable!=0,NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MTextAreaPeer_setTextBackground(Hsun_awt_motif_MTextAreaPeer *self,Hjava_awt_Color *color)
{
    AwtPeer *data;
    AWT_LOCK(); data=area(self);
    if (!color) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (data) XtVaSetValues(data->content,XmNbackground,awt118_pixel(unhand(color)->value),NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MTextAreaPeer_pSelect(Hsun_awt_motif_MTextAreaPeer *self,long start,long end)
{
    AwtPeer *data;
    AWT_LOCK(); data=area(self);
    if (data) XmTextSetSelection(data->content,start,end,CurrentTime);
    XFlush(awt_display); AWT_UNLOCK();
}
static long selection(Hsun_awt_motif_MTextAreaPeer *self,int last)
{
    AwtPeer *data;
    XmTextPosition start,end;
    long result=0;
    AWT_LOCK(); data=area(self);
    if (data) {
        if (!XmTextGetSelectionPosition(data->content,&start,&end) || start==end)
            result=XmTextGetInsertionPosition(data->content);
        else result=last ? end : start;
    }
    AWT_UNLOCK(); return result;
}
long sun_awt_motif_MTextAreaPeer_getSelectionStart(Hsun_awt_motif_MTextAreaPeer *self) { return selection(self,0); }
long sun_awt_motif_MTextAreaPeer_getSelectionEnd(Hsun_awt_motif_MTextAreaPeer *self) { return selection(self,1); }
long sun_awt_motif_MTextAreaPeer_getCaretPosition(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data; long result=0;
    AWT_LOCK(); data=area(self);
    if (data) result=XmTextGetInsertionPosition(data->content);
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MTextAreaPeer_setCaretPosition(Hsun_awt_motif_MTextAreaPeer *self,long position)
{
    AwtPeer *data;
    AWT_LOCK(); data=area(self);
    if (data) XmTextSetInsertionPosition(data->content,position);
    XFlush(awt_display); AWT_UNLOCK();
}
Hjava_lang_String *sun_awt_motif_MTextAreaPeer_getText(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data;
    Hjava_lang_String *result=NULL;
    char *text;
    AWT_LOCK(); data=area(self);
    if (data) {
        text=XmTextGetString(data->content);
        result=makeJavaString(text,strlen(text)); XtFree(text);
    }
    AWT_UNLOCK(); return result;
}
static void edit(Hsun_awt_motif_MTextAreaPeer *self,Hjava_lang_String *string,int mode,long start,long end)
{
    AwtPeer *data;
    char *text;
    if (!string) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    text=allocCString(string);
    AWT_LOCK(); data=area(self);
    if (data && text) {
        if (mode==0) XmTextSetString(data->content,text);
        else if (mode==1) XmTextInsert(data->content,start,text);
        else XmTextReplace(data->content,start,end,text);
        XFlush(awt_display);
    }
    AWT_UNLOCK(); free(text);
}
void sun_awt_motif_MTextAreaPeer_setText(Hsun_awt_motif_MTextAreaPeer *self,Hjava_lang_String *text) { edit(self,text,0,0,0); }
void sun_awt_motif_MTextAreaPeer_insert(Hsun_awt_motif_MTextAreaPeer *self,Hjava_lang_String *text,long position) { edit(self,text,1,position,0); }
void sun_awt_motif_MTextAreaPeer_replaceRange(Hsun_awt_motif_MTextAreaPeer *self,Hjava_lang_String *text,long start,long end) { edit(self,text,2,start,end); }

long sun_awt_motif_MTextAreaPeer_getExtraWidth(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data;
    Widget bar;
    Dimension margin,spacing,width=0,shadow=0;
    long result=0;
    AWT_LOCK(); data=area(self);
    if (data) {
        XtVaGetValues(data->content,XmNmarginWidth,&margin,NULL);
        XtVaGetValues(data->widget,XmNspacing,&spacing,XmNverticalScrollBar,&bar,NULL);
        if (bar) XtVaGetValues(bar,XmNwidth,&width,XmNshadowThickness,&shadow,NULL);
        result=width+spacing+2*margin+4*shadow;
    }
    AWT_UNLOCK(); return result;
}
long sun_awt_motif_MTextAreaPeer_getExtraHeight(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data;
    Widget bar;
    Dimension margin,shadow,highlight,spacing,height;
    long result=0;
    AWT_LOCK(); data=area(self);
    if (data) {
        XtVaGetValues(data->content,XmNmarginHeight,&margin,XmNshadowThickness,&shadow,XmNhighlightThickness,&highlight,NULL);
        result=2*(margin+shadow+highlight);
        XtVaGetValues(data->widget,XmNspacing,&spacing,XmNhorizontalScrollBar,&bar,NULL);
        if (bar) {
            XtVaGetValues(bar,XmNheight,&height,XmNshadowThickness,&shadow,XmNhighlightThickness,&highlight,NULL);
            result+=height+spacing+2*(shadow+highlight);
        }
    }
    AWT_UNLOCK(); return result;
}
void sun_awt_motif_MTextAreaPeer_setFont(Hsun_awt_motif_MTextAreaPeer *self,struct Hjava_awt_Font *font)
{
    AwtPeer *data;
    Dimension width,height,outerWidth,outerHeight;
    AWT_LOCK(); data=area(self);
    if (data) {
        XtVaGetValues(data->content,XmNwidth,&width,XmNheight,&height,NULL);
        XtVaGetValues(data->widget,XmNwidth,&outerWidth,XmNheight,&outerHeight,NULL);
        sun_awt_motif_MComponentPeer_pSetFont((Hsun_awt_motif_MComponentPeer *)self,font);
        XtVaSetValues(data->content,XmNwidth,width,XmNheight,height,NULL);
        XtVaSetValues(data->widget,XmNwidth,outerWidth,XmNheight,outerHeight,NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MTextAreaPeer_pMakeCursorVisible(Hsun_awt_motif_MTextAreaPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=area(self);
    if (data && XtIsRealized(data->content)) {
        XDefineCursor(awt_display,XtWindow(data->content),data->cursor); unhand(self)->cursorSet=1;
    }
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MTextAreaPeer_setCursor(Hsun_awt_motif_MTextAreaPeer *self,struct Hjava_awt_Cursor *cursor)
{
    sun_awt_motif_MComponentPeer_setCursor((Hsun_awt_motif_MComponentPeer *)self,cursor);
    sun_awt_motif_MTextAreaPeer_pMakeCursorVisible(self);
}
void sun_awt_motif_MTextAreaPeer_pShow(Hsun_awt_motif_MTextAreaPeer *self)
{
    sun_awt_motif_MComponentPeer_pShow((Hsun_awt_motif_MComponentPeer *)self);
    if (!unhand(self)->cursorSet) sun_awt_motif_MTextAreaPeer_pMakeCursorVisible(self);
}
