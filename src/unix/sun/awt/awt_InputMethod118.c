/* Solaris libawt.so 2a710..2ac4c: XIM lookup and XIC lifetime. */
#include "awt118.h"
#include "sun_awt_motif_X11InputMethod.h"
#include "javaString.h"
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>

typedef struct InputContext118 {
    XIC ic;
    Widget owner,focus;
    Hsun_awt_motif_X11InputMethod *javaMethod;
    struct InputContext118 *next;
} InputContext118;
static XIM inputMethod;
static InputContext118 *contexts,*current;
static int composing;

static InputContext118 *context(long handle)
{
    InputContext118 *value;
    for(value=contexts;value && (long)value!=handle;value=value->next) {}
    return value;
}
static void retire_ic(InputContext118 *value)
{
    if(current==value) { current=NULL; composing=0; }
    if(value->ic) XDestroyIC(value->ic);
    value->ic=NULL;
}
static void window_destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    InputContext118 *value=(InputContext118 *)closure;
    retire_ic(value);
    if(value->owner==widget) value->owner=NULL;
    if(value->focus==widget) value->focus=NULL;
}
long sun_awt_motif_X11InputMethod_openXIM(Hsun_awt_motif_X11InputMethod *self)
{
    long result;
    AWT_LOCK();
    if(!inputMethod) inputMethod=XOpenIM(awt_display,NULL,NULL,NULL);
    result=inputMethod!=NULL;
    AWT_UNLOCK(); return result;
}
long sun_awt_motif_X11InputMethod_initializeXICNative(Hsun_awt_motif_X11InputMethod *self,Hsun_awt_motif_MComponentPeer *peer)
{
    InputContext118 *value=NULL;
    AwtPeer *data;
    AWT_LOCK(); data=awt118_peer(peer);
    if(!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException","createXIC");
    else {
        value=(InputContext118 *)calloc(1,sizeof(*value));
        if(!value) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else {
            value->owner=data->widget; value->javaMethod=self;
            value->next=contexts; contexts=value;
            XtAddCallback(value->owner,XtNdestroyCallback,window_destroyed,value);
        }
    }
    AWT_UNLOCK(); return (long)value;
}
void sun_awt_motif_X11InputMethod_setXICFocus(Hsun_awt_motif_X11InputMethod *self,Hsun_awt_motif_MComponentPeer *peer,long focus,long handle)
{
    InputContext118 *value;
    AwtPeer *data;
    XIMStyles *styles=NULL;
    XIMStyle style=0;
    int i;
    AWT_LOCK(); value=context(handle);
    if(!value) { AWT_UNLOCK(); return; }
    if(!focus) {
        if(value->ic) XUnsetICFocus(value->ic);
        if(current==value) { current=NULL; composing=0; }
    } else if(peer) {
        data=awt118_peer(peer);
        if(!data || !data->widget) SignalError(EE(),"java/lang/NullPointerException","setXICFocus pData");
        else if(value->owner && inputMethod && XtIsRealized(data->widget)) {
            if(!value->ic) {
                if(!XGetIMValues(inputMethod,XNQueryInputStyle,&styles,NULL) && styles) {
                    for(i=0;i<styles->count_styles;i++) {
                        if(styles->supported_styles[i]==(XIMPreeditNothing|XIMStatusNothing)) {
                            style=styles->supported_styles[i]; break;
                        }
                        if(styles->supported_styles[i]==(XIMPreeditNone|XIMStatusNone)) style=styles->supported_styles[i];
                    }
                    XFree(styles);
                }
                if(style) value->ic=XCreateIC(inputMethod,XNClientWindow,XtWindow(value->owner),
                    XNFocusWindow,XtWindow(data->widget),XNInputStyle,style,NULL);
            }
            if(value->ic) {
                if(value->focus && value->focus!=value->owner)
                    XtRemoveCallback(value->focus,XtNdestroyCallback,window_destroyed,value);
                value->focus=data->widget;
                if(value->focus!=value->owner) XtAddCallback(value->focus,XtNdestroyCallback,window_destroyed,value);
                XSetICValues(value->ic,XNFocusWindow,XtWindow(value->focus),NULL);
                XSetICFocus(value->ic); current=value; composing=0;
            }
        }
    }
    XSync(awt_display,False); AWT_UNLOCK();
}
Hjava_lang_String *sun_awt_motif_X11InputMethod_resetXIC(Hsun_awt_motif_X11InputMethod *self,long handle)
{
    InputContext118 *value;
    Hjava_lang_String *result=NULL;
    char *text;
    AWT_LOCK(); value=context(handle);
    if(value && value->ic) {
        text=XmbResetIC(value->ic);
        if(text) { result=makeJavaStringFromPlatformCString(text,strlen(text)); XFree(text); }
    }
    composing=0; AWT_UNLOCK(); return result;
}
void sun_awt_motif_X11InputMethod_disposeXIC(Hsun_awt_motif_X11InputMethod *self,long handle)
{
    InputContext118 *value,**link;
    AWT_LOCK(); value=context(handle);
    if(value) {
        retire_ic(value);
        if(value->owner) XtRemoveCallback(value->owner,XtNdestroyCallback,window_destroyed,value);
        if(value->focus && value->focus!=value->owner) XtRemoveCallback(value->focus,XtNdestroyCallback,window_destroyed,value);
        for(link=&contexts;*link!=value;link=&(*link)->next) {}
        *link=value->next; free(value);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11InputMethod_closeXIM(Hsun_awt_motif_X11InputMethod *self)
{
    InputContext118 *value;
    AWT_LOCK();
    for(value=contexts;value;value=value->next) retire_ic(value);
    if(inputMethod) XCloseIM(inputMethod);
    inputMethod=NULL; AWT_UNLOCK();
}

int awt118_xim_event(XEvent *event)
{
    char local[64],*bytes=local;
    KeySym symbol=NoSymbol;
    Status status;
    int length,consumed=1;
    Hjava_lang_String *text;
    InputContext118 *value=current;
    Widget widget,focus;
    if(!value || !value->ic || !value->focus || (event->type!=KeyPress && event->type!=KeyRelease)) return 0;
    if(!awt118_modal_allows(value->focus)) return 0;
    widget=XtWindowToWidget(awt_display,event->xany.window);
    focus=widget ? XmGetFocusWidget(widget) : NULL;
    if(widget!=value->focus && focus!=value->focus) return 0;
    if(XFilterEvent(event,XtWindow(value->focus))) return 1;
    if(event->type!=KeyPress) return 0;
    length=XmbLookupString(value->ic,&event->xkey,local,sizeof(local),&symbol,&status);
    if(status==XBufferOverflow) {
        bytes=(char *)malloc(length+1);
        if(!bytes) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return 1; }
        length=XmbLookupString(value->ic,&event->xkey,bytes,length+1,&symbol,&status);
    }
    if(status==XLookupKeySym && symbol==XK_Multi_key) composing=1;
    else if(status==XLookupChars || (status==XLookupBoth && (composing || (symbol>=128 && (symbol&0xff00)!=0xff00)))) {
        composing=0;
        text=makeJavaStringFromPlatformCString(bytes,length);
        if(text) execute_java_dynamic_method(EE(),(HObject *)value->javaMethod,"dispatchCommittedText",
            "(Ljava/lang/String;J)V",text,(int64_t)event->xkey.time);
        awt118_callback_done();
    } else if(status==XLookupBoth || (status==XLookupKeySym && !composing)) consumed=0;
    if(bytes!=local) free(bytes);
    return consumed;
}
