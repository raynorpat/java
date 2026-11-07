/* Test-only X11 event/pixel observation through the live peer's widget. */
#include "awt118.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <Xm/ToggleB.h>
#include <string.h>

static Widget widget(Hjava_awt_Component *target)
{
    Hsun_awt_motif_MComponentPeer *peer=(Hsun_awt_motif_MComponentPeer *)unhand(target)->peer;
    return peer && unhand(peer)->pData ? *(Widget *)unhand(peer)->pData : NULL;
}

JAVA_PUBLIC_API(stack_item *) Java_Input115_key_stub(stack_item *p,struct execenv *ee)
{
    KeyCode code;
    AWT_LOCK();
    code=XKeysymToKeycode(awt_display,(KeySym)p[0].i);
    if (p[1].i&1) XTestFakeKeyEvent(awt_display,XKeysymToKeycode(awt_display,XK_Shift_L),True,CurrentTime);
    if (p[1].i&2) XTestFakeKeyEvent(awt_display,XKeysymToKeycode(awt_display,XK_Control_L),True,CurrentTime);
    XTestFakeKeyEvent(awt_display,code,True,CurrentTime);
    XTestFakeKeyEvent(awt_display,code,False,CurrentTime);
    if (p[1].i&2) XTestFakeKeyEvent(awt_display,XKeysymToKeycode(awt_display,XK_Control_L),False,CurrentTime);
    if (p[1].i&1) XTestFakeKeyEvent(awt_display,XKeysymToKeycode(awt_display,XK_Shift_L),False,CurrentTime);
    XFlush(awt_display);
    AWT_UNLOCK();
    return p;
}

JAVA_PUBLIC_API(stack_item *) Java_Widgets115_state_stub(stack_item *p,struct execenv *ee)
{
    AWT_LOCK();
    p[0].i=XmToggleButtonGetState(widget(p[0].p))!=False;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Widgets115_alignment_stub(stack_item *p,struct execenv *ee)
{
    unsigned char value;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNalignment,&value,NULL);
    p[0].i=value;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Widgets115_label_stub(stack_item *p,struct execenv *ee)
{
    XmString label;
    char *text=NULL;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNlabelString,&label,NULL);
    XmStringGetLtoR(label,XmFONTLIST_DEFAULT_TAG,&text);
    p[0].p=makeJavaString(text ? text : "",text ? strlen(text) : 0);
    if (text) XtFree(text);
    XmStringFree(label);
    AWT_UNLOCK();
    return p+1;
}

JAVA_PUBLIC_API(stack_item *) Java_Window115_click_stub(stack_item *p,struct execenv *ee)
{
    Widget w;
    AWT_LOCK();
    w=widget(p[0].p);
    if (!w || !XtIsRealized(w)) SignalError(ee,"java/lang/RuntimeException","button not realized");
    else {
        XWarpPointer(awt_display,None,XtWindow(w),0,0,0,0,10,10);
        XTestFakeButtonEvent(awt_display,Button1,True,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,False,CurrentTime);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
    return p;
}

JAVA_PUBLIC_API(stack_item *) Java_Window115_pixel_stub(stack_item *p,struct execenv *ee)
{
    Widget w;
    XImage *image;
    long pixel=0;
    AWT_LOCK();
    w=widget(p[0].p);
    if (!w || !XtIsRealized(w)) SignalError(ee,"java/lang/RuntimeException","canvas not realized");
    else {
        image=XGetImage(awt_display,XtWindow(w),p[1].i,p[2].i,1,1,AllPlanes,ZPixmap);
        if (image) { pixel=XGetPixel(image,0,0); XDestroyImage(image); }
        else SignalError(ee,"java/lang/RuntimeException","pixel read failed");
    }
    AWT_UNLOCK();
    p[0].i=pixel;
    return p+1;
}
