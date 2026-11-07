/* Solaris libawt.so 22fd8/23174/2345c and libawt_g.so 80fc8/82f2c.
 * Java listeners run before native replay on the Motif thread. */
#include "awt118.h"
#include "java_awt_AWTEvent.h"
#include <stdlib.h>

typedef struct NativeEvent {
    XEvent event;
    AwtPeer *owner;
    unsigned long token;
    struct NativeEvent *next;
} NativeEvent;
static AwtPeer *peers, *lastClick;
static NativeEvent *pendingEvents, *putback, **putbackTail=&putback;
static unsigned long nextToken;
static Time lastTime;
static int clickCount, lastX, lastY;

void awt118_register_input(AwtPeer *peer)
{ peer->next=peers; peers=peer; }

void awt118_forget_input(AwtPeer *peer)
{
    AwtPeer **link=&peers;
    NativeEvent **event, *old;
    while (*link && *link!=peer) link=&(*link)->next;
    if (*link) *link=peer->next;
    peer->next=NULL;
    if (lastClick==peer) { lastClick=NULL; clickCount=0; }
    event=&pendingEvents;
    while (*event) {
        if ((*event)->owner==peer) { old=*event; *event=old->next; free(old); }
        else event=&(*event)->next;
    }
    event=&putback;
    while (*event) {
        if ((*event)->owner==peer) { old=*event; *event=old->next; free(old); }
        else event=&(*event)->next;
    }
    putbackTail=event;
}

static int modifiers(unsigned int state)
{
    return ((state&ShiftMask) ? 1 : 0) | ((state&ControlMask) ? 2 : 0) |
        ((state&(Mod4Mask|Button3Mask)) ? 4 : 0) |
        ((state&(Mod1Mask|Button2Mask)) ? 8 : 0) | ((state&Button1Mask) ? 16 : 0);
}

static void post_mouse(AwtPeer *peer,XEvent *event,int id,int count,int popup,int replay)
{
    Hjava_awt_AWTEvent *javaEvent;
    NativeEvent *native=NULL;
    unsigned int state=event->xbutton.state;
    if (event->type==ButtonPress && event->xbutton.button<=3)
        state|=Button1Mask<<(event->xbutton.button-1);
    javaEvent=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/MouseEvent",NULL,
        "(Ljava/awt/Component;IJIIIIZ)",unhand(peer->javaPeer)->target,id,
        (int64_t)event->xbutton.time,modifiers(state),event->xbutton.x,event->xbutton.y,count,popup);
    if (!javaEvent) { awt118_callback_done(); return; }
    if (replay) {
        native=(NativeEvent *)malloc(sizeof(*native));
        if (!native) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); awt118_callback_done(); return; }
        native->event=*event; native->owner=peer;
        /* Opaque token instead of an XEvent pointer: disposal can retire pending
         * native storage before a queued Java event reaches its former peer. */
        if (++nextToken==0) ++nextToken;
        native->token=nextToken;
        native->next=pendingEvents; pendingEvents=native;
        unhand(javaEvent)->data=(long)native->token;
    }
    execute_java_dynamic_method(EE(),(HObject *)peer->javaPeer,"postEvent","(Ljava/awt/AWTEvent;)V",javaEvent);
    awt118_callback_done();
}

int awt118_filter_input(XEvent *event)
{
    AwtPeer *peer;
    Widget widget;
    int id;
    if (event->type<ButtonPress || event->type>LeaveNotify) return 0;
    widget=XtWindowToWidget(awt_display,event->xany.window);
    for (peer=peers;peer && peer->widget!=widget;peer=peer->next) {}
    if (!peer || !XtIsSensitive(widget)) return 0;
    switch (event->type) {
    case ButtonPress:
        if (lastClick==peer && event->xbutton.time-lastTime<=(Time)XtGetMultiClickTime(awt_display)) clickCount++;
        else clickCount=1;
        lastClick=peer; lastTime=event->xbutton.time;
        lastX=event->xbutton.x; lastY=event->xbutton.y;
        post_mouse(peer,event,501,clickCount,event->xbutton.button==Button3,1);
        break;
    case ButtonRelease:
        post_mouse(peer,event,502,clickCount,0,1);
        if (lastClick==peer) post_mouse(peer,event,500,clickCount,0,0);
        break;
    case MotionNotify:
        if (lastClick!=peer || event->xmotion.time-lastTime>(Time)XtGetMultiClickTime(awt_display) ||
            abs(event->xmotion.x-lastX)>=4 || abs(event->xmotion.y-lastY)>=4) {
            lastClick=NULL; clickCount=0;
        }
        id=(event->xmotion.state&(Button1Mask|Button2Mask|Button3Mask)) && !clickCount ? 506 : 503;
        post_mouse(peer,event,id,clickCount,0,1);
        break;
    default:
        if (event->xcrossing.mode!=NotifyNormal || event->xcrossing.detail==NotifyInferior) return 0;
        lastClick=NULL; clickCount=0;
        post_mouse(peer,event,event->type==EnterNotify ? 504 : 505,0,0,1);
    }
    return 1;
}

void sun_awt_motif_MComponentPeer_nativeHandleEvent(Hsun_awt_motif_MComponentPeer *self,Hjava_awt_AWTEvent *event)
{
    NativeEvent **link,*native;
    unsigned long token;
    if (!event) return;
    AWT_LOCK(); token=(unsigned long)unhand(event)->data; unhand(event)->data=0;
    link=&pendingEvents;
    while (*link && (*link)->token!=token) link=&(*link)->next;
    native=*link;
    if (native) {
        *link=native->next;
        if (!unhand(event)->consumed) {
            native->next=NULL; *putbackTail=native; putbackTail=&native->next;
        } else free(native);
    }
    AWT_UNLOCK();
}

int awt118_dispatch_putback(void)
{
    NativeEvent *native=putback;
    if (!native) return 0;
    putback=native->next;
    if (!putback) putbackTail=&putback;
    if (native->owner->widget && XtWindow(native->owner->widget)==native->event.xany.window)
        XtDispatchEvent(&native->event);
    free(native);
    return 1;
}
