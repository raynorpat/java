/* Solaris release 20e38..2144c: Motif file selection and modal callbacks. */
#include "awt118.h"
#include "sun_awt_motif_MFileDialogPeer.h"
#include "sun_awt_motif_MDialogPeer.h"
#include "java_awt_FileDialog.h"
#include "java_awt_Component.h"
#include <X11/Shell.h>
#include <Xm/FileSB.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/Protocols.h>
#include <stdlib.h>
#include <string.h>

static void shell_destroyed(Widget widget,XtPointer closure,XtPointer call)
{ AwtPeer *data=(AwtPeer *)closure; awt118_end_modal(data); data->shell=NULL; }
static void selected(Widget widget,XtPointer closure,XtPointer call)
{
    Hsun_awt_motif_MFileDialogPeer *self=(Hsun_awt_motif_MFileDialogPeer *)closure;
    AwtPeer *data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    XmFileSelectionBoxCallbackStruct *event=(XmFileSelectionBoxCallbackStruct *)call;
    char *path=NULL;
    Hjava_lang_String *value;
    if (!data) return;
    data->references++;
    if (event && event->reason==XmCR_OK && XmStringGetLtoR(event->value,XmFONTLIST_DEFAULT_TAG,&path)) {
        value=makeJavaString(path,strlen(path)); XtFree(path);
        if (value) execute_java_dynamic_method(EE(),(HObject *)self,"handleSelected","(Ljava/lang/String;)V",value);
    } else execute_java_dynamic_method(EE(),(HObject *)self,"handleCancel","()V");
    awt118_callback_done();
    if (data->widget) XtUnmanageChild(data->widget);
    awt118_end_modal(data); awt118_release_peer(data);
}
void sun_awt_motif_MFileDialogPeer_create(Hsun_awt_motif_MFileDialogPeer *self,Hsun_awt_motif_MComponentPeer *parent)
{
    AwtPeer *data,*owner;
    Arg args[6]; int n=0;
    Widget help,button;
    XmString text;
    AWT_LOCK(); owner=awt118_peer(parent);
    if (!owner || !owner->shell) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if ((data=awt118_new_peer((Hsun_awt_motif_MComponentPeer *)self))!=NULL) {
        data->cleanup=awt118_end_modal;
        XtSetArg(args[n],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL); n++;
        XtSetArg(args[n],XmNautoUnmanage,False); n++;
        XtSetArg(args[n],XmNresizePolicy,XmRESIZE_NONE); n++;
        XtSetArg(args[n],XtNvisual,awt_visInfo.visual); n++;
        XtSetArg(args[n],XtNdepth,awt_visInfo.depth); n++;
        XtSetArg(args[n],XtNcolormap,awt_cmap); n++;
        data->widget=XmCreateFileSelectionDialog(owner->shell,"fileDialog",args,n);
        data->container=data->widget; data->shell=XtParent(data->widget);
        XtVaSetValues(data->shell,XmNdeleteResponse,XmDO_NOTHING,NULL);
        XtAddCallback(data->shell,XtNdestroyCallback,shell_destroyed,data);
        help=XmFileSelectionBoxGetChild(data->widget,XmDIALOG_HELP_BUTTON); if (help) XtUnmanageChild(help);
        button=XmFileSelectionBoxGetChild(data->widget,XmDIALOG_OK_BUTTON);
        text=XmStringCreateLocalized(unhand((Hjava_awt_FileDialog *)unhand(self)->target)->mode ? "Save" : "Open");
        XtVaSetValues(button,XmNlabelString,text,NULL); XmStringFree(text);
        XtAddCallback(data->widget,XmNokCallback,selected,self);
        XtAddCallback(data->widget,XmNcancelCallback,selected,self);
        XmAddWMProtocolCallback(data->shell,XmInternAtom(awt_display,"WM_DELETE_WINDOW",False),selected,self);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_MFileDialogPeer_pShow(Hsun_awt_motif_MFileDialogPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->widget) XtManageChild(data->widget);
    AWT_UNLOCK(); sun_awt_motif_MDialogPeer_pShow((Hsun_awt_motif_MDialogPeer *)self);
}
void sun_awt_motif_MFileDialogPeer_pHide(Hsun_awt_motif_MFileDialogPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data) { if (data->widget) XtUnmanageChild(data->widget); awt118_end_modal(data); }
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MFileDialogPeer_pDispose(Hsun_awt_motif_MFileDialogPeer *self)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->widget) XtUnmanageChild(data->widget);
    awt118_dispose_peer((Hsun_awt_motif_MComponentPeer *)self); XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MFileDialogPeer_pReshape(Hsun_awt_motif_MFileDialogPeer *self,long x,long y,long width,long height)
{
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (data && data->shell) XtVaSetValues(data->shell,XtNx,x,XtNy,y,NULL);
    XFlush(awt_display); AWT_UNLOCK();
}
void sun_awt_motif_MFileDialogPeer_setFileEntry(Hsun_awt_motif_MFileDialogPeer *self,Hjava_lang_String *directory,Hjava_lang_String *file,HArrayOfString *files)
{
    AwtPeer *data;
    char *dir,*name,*mask;
    XmString pattern,entry,*items=NULL;
    Widget field,list;
    int count=files ? obj_length(files) : 0,i;
    if (!directory) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self);
    if (!data || !data->widget) { AWT_UNLOCK(); return; }
    dir=allocCString(directory); name=file ? allocCString(file) : NULL;
    if (!dir) { free(name); AWT_UNLOCK(); return; }
    mask=(char *)malloc(strlen(dir)+9);
    if (!mask) { free(dir); free(name); SignalError(EE(),"java/lang/OutOfMemoryError",NULL); AWT_UNLOCK(); return; }
    strcpy(mask,*dir ? dir : ".");
    if (mask[strlen(mask)-1]!='/') strcat(mask,"/");
    strcat(mask,"[^.]*"); pattern=XmStringCreateLocalized(mask);
    XmFileSelectionDoSearch(data->widget,pattern); XmStringFree(pattern); free(mask); free(dir);
    if (count==1 && !javaStringLength((Hjava_lang_String *)unhand(files)->body[0])) count=0;
    if (count) items=(XmString *)calloc(count,sizeof(XmString));
    if (count && !items) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    else if (files) {
        for (i=0;i<count;i++) {
            char *item=allocCString((Hjava_lang_String *)unhand(files)->body[i]);
            items[i]=XmStringCreateLocalized(item ? item : ""); free(item);
        }
        XtVaSetValues(data->widget,XmNfileListItems,items,XmNfileListItemCount,count,NULL);
        for (i=0;i<count;i++) XmStringFree(items[i]); free(items);
    }
    field=XmFileSelectionBoxGetChild(data->widget,XmDIALOG_TEXT);
    list=XmFileSelectionBoxGetChild(data->widget,XmDIALOG_LIST);
    XmTextFieldSetString(field,name ? name : "");
    XmTextFieldSetSelection(field,0,name ? strlen(name) : 0,CurrentTime);
    if (name && *name) { entry=XmStringCreateLocalized(name); XmListSelectItem(list,entry,False); XmStringFree(entry); }
    free(name); XFlush(awt_display); AWT_UNLOCK();
}
static void set_font(Widget widget,XmFontList list)
{
    WidgetList children; Cardinal count,i;
    if (XmIsPrimitive(widget) || XmIsGadget(widget)) XtVaSetValues(widget,XmNfontList,list,NULL);
    if (XtIsComposite(widget)) {
        XtVaGetValues(widget,XtNchildren,&children,XtNnumChildren,&count,NULL);
        for (i=0;i<count;i++) set_font(children[i],list);
    }
}
void sun_awt_motif_MFileDialogPeer_setFont(Hsun_awt_motif_MFileDialogPeer *self,struct Hjava_awt_Font *font)
{
    AwtPeer *data; XFontStruct *xfont; XmFontList list;
    AWT_LOCK(); data=awt118_peer((Hsun_awt_motif_MComponentPeer *)self); xfont=awt118_font(font);
    if (data && data->widget && xfont) {
        list=XmFontListCreate(xfont,XmFONTLIST_DEFAULT_TAG); set_font(data->widget,list); XmFontListFree(list);
    }
    AWT_UNLOCK();
}
