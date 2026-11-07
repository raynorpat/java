/* Solaris release 2ffe4..30870: Xt selection ownership and text transfer. */
#include "awt118.h"
#include "sun_awt_motif_X11Selection.h"
#include "java_awt_datatransfer_DataFlavor.h"
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct Selection {
    Hsun_awt_motif_X11Selection *peer;
    struct Selection *next;
} Selection;
typedef struct {
    Hsun_awt_motif_X11Selection *peer;
    int targets,done;
} AwtSelectionRequest;
static Selection *selections;
static Widget selection_shell;
static Atom targets_atom,compound_atom;

static Hsun_awt_motif_X11Selection *find_selection(Atom atom)
{
    Selection *entry;
    for (entry=selections;entry;entry=entry->next) if ((Atom)unhand(entry->peer)->atom==atom) return entry->peer;
    return NULL;
}
void sun_awt_motif_X11Selection_init(Hsun_awt_motif_X11Selection *self)
{
    AWT_LOCK();
    if (!selection_shell) {
        selection_shell=XtVaAppCreateShell("awtSelection","XApplication",topLevelShellWidgetClass,awt_display,
            XtNwidth,1,XtNheight,1,XtNmappedWhenManaged,False,NULL);
        XtRealizeWidget(selection_shell);
        targets_atom=XInternAtom(awt_display,"TARGETS",False);
        compound_atom=XInternAtom(awt_display,"COMPOUND_TEXT",False);
    }
    AWT_UNLOCK();
}
long sun_awt_motif_X11Selection_getAtomForTarget(Hsun_awt_motif_X11Selection *self,Hjava_lang_String *name)
{
    char *text; Atom atom=None;
    if (!name) { SignalError(EE(),"java/lang/NullPointerException",NULL); return 0; }
    text=allocCString(name);
    AWT_LOCK(); if (text) atom=XInternAtom(awt_display,text,False); AWT_UNLOCK(); free(text); return atom;
}
void sun_awt_motif_X11Selection_registerTargetForFlavor(Hsun_awt_motif_X11Selection *self,
    Hjava_awt_datatransfer_DataFlavor *flavor,Hjava_lang_String *name)
{
    long atom;
    if (!flavor) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    atom=sun_awt_motif_X11Selection_getAtomForTarget(self,name);
    if (!exceptionOccurred(EE())) unhand(flavor)->atom=atom;
}
void sun_awt_motif_X11Selection_create(Hsun_awt_motif_X11Selection *self,Hjava_lang_String *name)
{
    Selection *entry;
    long atom=sun_awt_motif_X11Selection_getAtomForTarget(self,name);
    if (exceptionOccurred(EE())) return;
    entry=(Selection *)malloc(sizeof(*entry));
    if (!entry) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return; }
    AWT_LOCK(); unhand(self)->atom=atom; entry->peer=self; entry->next=selections; selections=entry; AWT_UNLOCK();
}
void sun_awt_motif_X11Selection_pDispose(Hsun_awt_motif_X11Selection *self)
{
    Selection **link,*entry;
    AWT_LOCK();
    for (link=&selections;*link && (*link)->peer!=self;link=&(*link)->next);
    if (*link) {
        entry=*link; *link=entry->next; free(entry);
        if (XGetSelectionOwner(awt_display,unhand(self)->atom)==XtWindow(selection_shell))
            XtDisownSelection(selection_shell,unhand(self)->atom,XtLastTimestampProcessed(awt_display));
    }
    AWT_UNLOCK();
}
static void lost(Widget widget,Atom *atom)
{
    Hsun_awt_motif_X11Selection *self=find_selection(*atom);
    if (self && unhand(self)->holder) {
        AWT_UNLOCK(); execute_java_dynamic_method(EE(),(HObject *)self,"lostSelectionOwnership","()V"); AWT_LOCK();
        awt118_callback_done();
    }
}
static Boolean convert(Widget widget,Atom *selection,Atom *target,Atom *type,XtPointer *value,unsigned long *length,int *format)
{
    Hsun_awt_motif_X11Selection *self=find_selection(*selection);
    HArrayOfObject *flavors;
    Hjava_awt_datatransfer_DataFlavor *flavor=NULL;
    Hjava_lang_String *string;
    Atom *atoms,atom;
    int i,j,count=0;
    char *text;
    XTextProperty property;
    if (!self || !unhand(self)->contents) return False;
    AWT_UNLOCK();
    flavors=(HArrayOfObject *)execute_java_dynamic_method(EE(),(HObject *)unhand(self)->contents,
        "getTransferDataFlavors","()[Ljava/awt/datatransfer/DataFlavor;");
    AWT_LOCK();
    if (exceptionOccurred(EE())) { awt118_callback_done(); return False; }
    if (!flavors || find_selection(*selection)!=self || !unhand(self)->contents) return False;
    atoms=(Atom *)XtMalloc((obj_length(flavors)*2+1)*sizeof(Atom));
    for (i=0;i<obj_length(flavors);i++) {
        Hjava_awt_datatransfer_DataFlavor *candidate=(Hjava_awt_datatransfer_DataFlavor *)unhand(flavors)->body[i];
        if (!candidate) continue;
        atom=unhand(candidate)->atom;
        if (!flavor && (atom==*target || (atom==XA_STRING && *target==compound_atom))) flavor=candidate;
        for (j=0;j<count && atoms[j]!=atom;j++);
        if (atom && j==count) { atoms[count++]=atom; if (atom==XA_STRING) atoms[count++]=compound_atom; }
    }
    if (*target==targets_atom && count) {
        *type=XA_ATOM; *value=atoms; *length=count; *format=32; return True;
    }
    XtFree((char *)atoms);
    if (!flavor || (*target!=XA_STRING && *target!=compound_atom)) return False;
    AWT_UNLOCK();
    string=(Hjava_lang_String *)execute_java_dynamic_method(EE(),(HObject *)self,"getStringDataFromOwner",
        "(Ljava/awt/datatransfer/DataFlavor;)Ljava/lang/String;",flavor);
    AWT_LOCK();
    if (exceptionOccurred(EE())) { awt118_callback_done(); return False; }
    if (!string || find_selection(*selection)!=self) return False;
    text=allocCString(string); if (!text) return False;
    if (*target==XA_STRING) {
        *length=strlen(text); *value=XtMalloc(*length+1); memcpy(*value,text,*length+1);
        *type=XA_STRING; *format=8; free(text); return True;
    }
    if (XmbTextListToTextProperty(awt_display,&text,1,XCompoundTextStyle,&property)==Success) {
        *length=property.nitems; *value=property.value; *type=property.encoding; *format=property.format;
        free(text); return True;
    }
    free(text); return False;
}
long sun_awt_motif_X11Selection_pGetSelectionOwnership(Hsun_awt_motif_X11Selection *self)
{
    long owned;
    AWT_LOCK(); owned=XtOwnSelection(selection_shell,unhand(self)->atom,XtLastTimestampProcessed(awt_display),convert,lost,NULL);
    XFlush(awt_display); AWT_UNLOCK(); return owned;
}
static void received(Widget widget,XtPointer closure,Atom *selection,Atom *type,XtPointer value,unsigned long *length,int *format)
{
    AwtSelectionRequest *request=(AwtSelectionRequest *)closure;
    HArrayOfInt *array;
    Hjava_lang_String *string=NULL;
    XTextProperty property;
    char **list=NULL;
    int count=0,i;
    if (value && *length<=INT_MAX) {
        if (request->targets && (*type==XA_ATOM || *type==targets_atom) && *format==32) {
            array=(HArrayOfInt *)ArrayAlloc(T_INT,*length);
            if (array) {
                for (i=0;i<*length;i++) unhand(array)->body[i]=((Atom *)value)[i];
                unhand(request->peer)->targetArray=array;
            }
        } else if (!request->targets && *format==8) {
            if (*type==XA_STRING) string=makeJavaString(value,*length);
            else if (*type==compound_atom) {
                property.value=value; property.encoding=*type; property.format=*format; property.nitems=*length;
                if (XmbTextPropertyToTextList(awt_display,&property,&list,&count)>=Success && list && count>0)
                    string=makeJavaString(list[0],strlen(list[0]));
                if (list) XFreeStringList(list);
            }
            if (string) unhand(request->peer)->data=(Hjava_lang_Object *)string;
        }
    }
    if (value) XtFree(value);
    request->done=1; monitorNotifyAll((unsigned int)awt_lock);
}
static void request_selection(Hsun_awt_motif_X11Selection *self,Atom target,int targets)
{
    AwtSelectionRequest request;
    request.peer=self; request.targets=targets; request.done=0;
    XtGetSelectionValue(selection_shell,unhand(self)->atom,target,received,&request,XtLastTimestampProcessed(awt_display));
    XFlush(awt_display);
    while (!request.done) monitorWait((unsigned int)awt_lock,-1);
}
void sun_awt_motif_X11Selection_pGetTransferTargets(Hsun_awt_motif_X11Selection *self)
{ AWT_LOCK(); request_selection(self,targets_atom,1); AWT_UNLOCK(); }
void sun_awt_motif_X11Selection_pGetTransferData(Hsun_awt_motif_X11Selection *self,HArrayOfInt *targets)
{
    int i;
    if (!targets) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK();
    for (i=0;i<obj_length(targets) && !unhand(self)->data;i++) request_selection(self,unhand(targets)->body[i],0);
    AWT_UNLOCK();
}
