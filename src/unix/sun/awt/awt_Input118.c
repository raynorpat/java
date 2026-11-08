/* Solaris libawt.so 22fd8/23174/2345c and libawt_g.so 80fc8/82f2c.
 * Java listeners run before native replay on the Motif thread. */
#include "awt118.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_event_KeyEvent.h"
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
static unsigned int altMask,metaMask,numLockMask;
static int modifierMapReady;

/* Solaris setup_modifier_map 412e4 derives these from the X server. On
 * XKB hosts inspect base symbols, avoiding Alt's secondary Meta alias. */
static void modifier_map(void)
{
    XModifierKeymap *mapping=XGetModifierMapping(awt_display);
    unsigned int superMask=0,mask;
    int i;
    KeySym symbol;
    altMask=metaMask=numLockMask=0;
    if (mapping) {
        for(i=3*mapping->max_keypermod;i<8*mapping->max_keypermod;i++) {
            if(!mapping->modifiermap[i]) continue;
            mask=1U<<(i/mapping->max_keypermod);
            symbol=XKeycodeToKeysym(awt_display,mapping->modifiermap[i],0);
            if(symbol==XK_Alt_L || symbol==XK_Alt_R) altMask|=mask;
            if(symbol==XK_Meta_L || symbol==XK_Meta_R) metaMask|=mask;
            if(symbol==XK_Super_L || symbol==XK_Super_R) superMask|=mask;
            if(symbol==XK_Num_Lock) numLockMask|=mask;
        }
        XFreeModifiermap(mapping);
    }
    if(!metaMask) metaMask=superMask;
    modifierMapReady=1;
}

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

int awt118_modifiers(unsigned int state)
{
    if(!modifierMapReady) modifier_map();
    return ((state&ShiftMask) ? 1 : 0) | ((state&ControlMask) ? 2 : 0) |
        ((state&(metaMask|Button3Mask)) ? 4 : 0) |
        ((state&(altMask|Button2Mask)) ? 8 : 0) | ((state&Button1Mask) ? 16 : 0);
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
        (int64_t)event->xbutton.time,awt118_modifiers(state),x,y,count,popup);
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
    int i,code=0,typed=0,character=0,mods=awt118_modifiers(event->xkey.state);
    Hjava_awt_AWTEvent *javaEvent;
    symbol=XKeycodeToKeysym(awt_display,event->xkey.keycode,0);
    if (symbol>=XK_KP_Space && symbol<=XK_KP_Equal) {
        /* Solaris uses its keypad group's function-key aliases. XKB hosts
         * translate Num Lock/Shift first, then use the standard KP symbols. */
        if (event->xkey.state&(numLockMask|ShiftMask)) {
            char bytes[8];
            XLookupString(&event->xkey,bytes,sizeof(bytes),&translated,NULL);
            if(translated!=NoSymbol) symbol=translated;
        }
        switch(symbol) {
        case XK_KP_Home: symbol=XK_Home; break;
        case XK_KP_Left: symbol=XK_Left; break;
        case XK_KP_Up: symbol=XK_Up; break;
        case XK_KP_Right: symbol=XK_Right; break;
        case XK_KP_Down: symbol=XK_Down; break;
        case XK_KP_Page_Up: symbol=XK_Page_Up; break;
        case XK_KP_Page_Down: symbol=XK_Page_Down; break;
        case XK_KP_End: symbol=XK_End; break;
        case XK_KP_Begin: symbol=XK_Clear; break;
        }
    }
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
    if(event->type==MappingNotify) {
        XRefreshKeyboardMapping(&event->xmapping); modifierMapReady=0; return 0;
    }
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

/* Solaris debug modify_Event 810b8 / awt_modify_KeyEvent 81370 applies
 * listener edits before the native event returns to Xt. */
static void modify_key(XEvent *native,Hjava_awt_AWTEvent *event)
{
    Classjava_awt_event_KeyEvent *key=unhand((Hjava_awt_event_KeyEvent *)event);
    KeySym symbol=NoSymbol;
    int i,code=key->keyCode,character=key->keyChar,mods=key->modifiers;
    if (code!=127) {
        if (character<256 && code!=8 && code!=9 && code!=10 && code!=27 && code!=106 && code!=107)
            symbol=character;
        else for(i=0;i<sizeof(keymap)/sizeof(keymap[0]);i++) if(keymap[i].code==code) {
            symbol=keymap[i].symbol; break;
        }
    }
    if ((mods&2) && symbol) {
        if (symbol==27 || symbol==28 || symbol==29 || symbol==31) symbol+=64;
        else if (symbol>=1 && symbol<=26) symbol+=96;
    }
    if (symbol) {
        KeyCode keycode=XKeysymToKeycode(awt_display,symbol);
        if(keycode) native->xkey.keycode=keycode;
    }
    if ((symbol>='A' && symbol<='Z') || (mods&1)) native->xkey.state|=ShiftMask;
    if (mods&2) native->xkey.state|=ControlMask;
    if (mods&4) native->xkey.state|=metaMask|Button3Mask;
    if (mods&8) native->xkey.state|=altMask|Button2Mask;
    if (mods&16) native->xkey.state|=Button1Mask;
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
            if (native->event.type==KeyPress || native->event.type==KeyRelease) modify_key(&native->event,event);
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
