/* Solaris libawt.so 22fd8/23174/2345c and libawt_g.so 80fc8/82f2c.
 * Java listeners run before native replay on the Motif thread. */
#include "awt118.h"
#include "java_awt_AWTEvent.h"
#include <X11/keysym.h>
#include <Xm/ScrollBar.h>
#include <ctype.h>
#include <stdlib.h>
#include "awt_Keymap118.h"

typedef struct NativeEvent {
    XEvent event;
    AwtPeer *owner;
    unsigned long token;
    struct NativeEvent *next;
} NativeEvent;
static AwtPeer *peers, *lastClick;
static Widget scrollbarGrab;
static NativeEvent *pendingEvents, *putback, **putbackTail=&putback;
static unsigned long nextToken;
static Time lastTime;
static int clickCount, lastX, lastY;

static void scrollbar_destroyed(Widget widget,XtPointer closure,XtPointer call)
{
    if (scrollbarGrab==widget) {
        XtUngrabKeyboard(widget,CurrentTime);
        XtUngrabPointer(widget,CurrentTime);
        scrollbarGrab=NULL;
    }
}

void awt118_register_input(AwtPeer *peer)
{ peer->next=peers; peers=peer; }

void awt118_forget_input(AwtPeer *peer)
{
    AwtPeer **link=&peers;
    NativeEvent **event, *old;
    Widget ancestor=scrollbarGrab;
    /* OpenMotif grabs the keyboard while tracking a scrollbar. Xt retains
     * that widget after destruction unless the grab is explicitly released. */
    while (ancestor && ancestor!=peer->widget && ancestor!=peer->shell) ancestor=XtParent(ancestor);
    if (ancestor) {
        XtUngrabKeyboard(scrollbarGrab,CurrentTime);
        XtUngrabPointer(scrollbarGrab,CurrentTime);
        XtRemoveCallback(scrollbarGrab,XmNdestroyCallback,scrollbar_destroyed,NULL);
        scrollbarGrab=NULL;
    }
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

static void post_event(AwtPeer *peer,XEvent *event,Hjava_awt_AWTEvent *javaEvent,int replay)
{
    NativeEvent *native=NULL;
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

static void post_mouse(AwtPeer *peer,XEvent *event,int id,int count,int popup,int replay)
{
    Hjava_awt_AWTEvent *javaEvent;
    Window child;
    int x=event->xbutton.x,y=event->xbutton.y;
    unsigned int state=event->xbutton.state;
    if (event->xany.window!=XtWindow(peer->widget))
        XTranslateCoordinates(awt_display,event->xany.window,XtWindow(peer->widget),x,y,&x,&y,&child);
    x+=peer->decorLeft; y+=peer->decorTop+peer->menuHeight+peer->warningHeight;
    if (event->type==ButtonPress && event->xbutton.button<=3)
        state|=Button1Mask<<(event->xbutton.button-1);
    javaEvent=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/MouseEvent",NULL,
        "(Ljava/awt/Component;IJIIIIZ)",unhand(peer->javaPeer)->target,id,
        (int64_t)event->xbutton.time,modifiers(state),x,y,count,popup);
    post_event(peer,event,javaEvent,replay);
}

void awt118_focus_event(Hsun_awt_motif_MComponentPeer *peer,XEvent *event)
{
    Hjava_awt_AWTEvent *javaEvent;
    if (event->xfocus.mode!=NotifyNormal || event->xfocus.detail==NotifyPointer ||
        event->xfocus.detail==NotifyVirtual) return;
    javaEvent=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/FocusEvent",NULL,
        "(Ljava/awt/Component;IZ)",unhand(peer)->target,event->type==FocusIn ? 1004 : 1005,
        event->type==FocusOut);
    if (javaEvent) execute_java_dynamic_method(EE(),(HObject *)peer,"postEvent","(Ljava/awt/AWTEvent;)V",javaEvent);
    awt118_callback_done();
}

/* Release 3caf0: unshifted keysym selects VK; translated symbol selects char. */
static int post_key(AwtPeer *peer,XEvent *event)
{
    KeySym symbol,translated;
    Modifiers used;
    int i,code=0,typed=0,character=0,mods=modifiers(event->xkey.state);
    Hjava_awt_AWTEvent *javaEvent;
    symbol=XKeycodeToKeysym(awt_display,event->xkey.keycode,0);
    for (i=0;i<sizeof(keymap)/sizeof(keymap[0]);i++) if (keymap[i].symbol==symbol) {
        code=keymap[i].code; typed=keymap[i].typed; break;
    }
    if (!code) return 0;
    if (event->type==KeyPress) {
        if (symbol==XK_Shift_L || symbol==XK_Shift_R || symbol==XK_Shift_Lock) mods|=1;
        if (symbol==XK_Control_L || symbol==XK_Control_R) mods|=2;
        if (symbol==XK_Meta_L || symbol==XK_Meta_R) mods|=4;
        if (symbol==XK_Alt_L || symbol==XK_Alt_R) mods|=8;
    }
    if (symbol<256) {
        XtTranslateKeycode(awt_display,event->xkey.keycode,event->xkey.state,&used,&translated);
        character=(int)translated;
        if (event->xkey.state&ControlMask) {
            if (character=='[' || character=='\\' || character==']' || character=='_') character-=64;
            else if (character<128 && isalpha(character)) character=tolower(character)-96;
        }
    } else if (typed) {
        if (symbol>=XK_KP_0 && symbol<=XK_KP_9) character='0'+symbol-XK_KP_0;
        else switch (symbol) {
        case XK_Return: case XK_KP_Enter: case XK_Linefeed: character=10; break;
        case XK_BackSpace: character=8; break;
        case XK_Tab: character=9; break;
        case XK_Escape: character=27; break;
        case XK_Delete: case XK_KP_Delete: character=127; break;
        case XK_KP_Multiply: character='*'; break;
        case XK_KP_Add: character='+'; break;
        case XK_KP_Subtract: character='-'; break;
        case XK_KP_Decimal: character='.'; break;
        case XK_KP_Divide: character='/'; break;
        case XK_KP_Equal: character='='; break;
        }
    }
    javaEvent=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/KeyEvent",NULL,
        "(Ljava/awt/Component;IJIIC)",unhand(peer->javaPeer)->target,event->type==KeyPress ? 401 : 402,
        (int64_t)event->xkey.time,mods,code,character);
    post_event(peer,event,javaEvent,1);
    if (event->type==KeyPress && typed) {
        javaEvent=(Hjava_awt_AWTEvent *)execute_java_constructor(EE(),"java/awt/event/KeyEvent",NULL,
            "(Ljava/awt/Component;IJIIC)",unhand(peer->javaPeer)->target,400,
            (int64_t)event->xkey.time,mods,0,character);
        post_event(peer,event,javaEvent,0);
    }
    return 1;
}

int awt118_filter_input(XEvent *event)
{
    AwtPeer *peer;
    Widget widget,focus;
    int id;
    if (event->type<KeyPress || event->type>LeaveNotify) return 0;
    widget=XtWindowToWidget(awt_display,event->xany.window);
    if (!widget) return 0;
    if (!awt118_modal_allows(widget)) return 1;
    /* Original event loop dispatches scrollbar tracking directly to Xt. */
    if (XtIsSubclass(widget,xmScrollBarWidgetClass) &&
        (event->type==ButtonPress || event->type==ButtonRelease ||
        (event->type==MotionNotify && (event->xmotion.state==Button1Mask ||
            event->xmotion.state==Button2Mask || event->xmotion.state==Button3Mask)))) {
        if (event->type==ButtonPress) {
            if (scrollbarGrab) XtRemoveCallback(scrollbarGrab,XmNdestroyCallback,scrollbar_destroyed,NULL);
            scrollbarGrab=widget;
            XtAddCallback(widget,XmNdestroyCallback,scrollbar_destroyed,NULL);
        } else if (event->type==ButtonRelease && scrollbarGrab) {
            XtRemoveCallback(scrollbarGrab,XmNdestroyCallback,scrollbar_destroyed,NULL);
            scrollbarGrab=NULL;
        }
        return 0;
    }
    if (widget && (event->type==KeyPress || event->type==KeyRelease)) {
        focus=XmGetFocusWidget(widget);
        if (focus) widget=focus;
    }
    for (peer=peers;peer && peer->widget!=widget && peer->content!=widget;peer=peer->next) {}
    if (!peer || !XtIsSensitive(widget)) return 0;
    switch (event->type) {
    case KeyPress: case KeyRelease:
        lastClick=NULL; clickCount=0;
        return post_key(peer,event);
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
    if (native->owner->widget && XtWindowToWidget(awt_display,native->event.xany.window))
        XtDispatchEvent(&native->event);
    free(native);
    return 1;
}
