/* Test-only X11 event/pixel observation through the live peer's widget. */
#include "awt118.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <string.h>

static Widget widget(Hjava_awt_Component *target)
{
    Hsun_awt_motif_MComponentPeer *peer=(Hsun_awt_motif_MComponentPeer *)unhand(target)->peer;
    AwtPeer *data=peer ? awt118_peer(peer) : NULL;
    return data ? (data->content ? data->content : data->widget) : NULL;
}

JAVA_PUBLIC_API(stack_item *) Java_TextField115_displayed_stub(stack_item *p,struct execenv *ee)
{
    char *text;
    AWT_LOCK();
    text=XmTextFieldGetString(widget(p[0].p));
    p[0].p=makeJavaString(text,strlen(text));
    XtFree(text); AWT_UNLOCK();
    return p+1;
}

JAVA_PUBLIC_API(stack_item *) Java_TextArea115_scrollbars_stub(stack_item *p,struct execenv *ee)
{
    Boolean horizontal,vertical;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNscrollHorizontal,&horizontal,XmNscrollVertical,&vertical,NULL);
    p[0].i=(horizontal ? 1 : 0)|(vertical ? 2 : 0);
    AWT_UNLOCK(); return p+1;
}

JAVA_PUBLIC_API(stack_item *) Java_ScrollPane115_nativePosition_stub(stack_item *p,struct execenv *ee)
{
    Widget w,child;
    WidgetList children;
    Cardinal count;
    Position x=0,y=0;
    AWT_LOCK(); w=widget(p[0].p);
    if (XmIsScrolledWindow(w)) XtVaGetValues(w,XmNworkWindow,&child,NULL);
    else {
        XtVaGetValues(w,XmNchildren,&children,XmNnumChildren,&count,NULL);
        child=count ? children[0] : NULL;
    }
    if (child) XtVaGetValues(child,XmNx,&x,XmNy,&y,NULL);
    else SignalError(ee,"java/lang/RuntimeException","missing scroll child");
    p[0].p=execute_java_constructor(ee,"java/awt/Point",NULL,"(II)",-(int)x,-(int)y);
    AWT_UNLOCK(); return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_ScrollPane115_press_stub(stack_item *p,struct execenv *ee)
{
    Widget bar;
    AWT_LOCK();
    if (p[2].i) {
        XtVaGetValues(widget(p[0].p),p[1].i ? XmNverticalScrollBar : XmNhorizontalScrollBar,&bar,NULL);
        XWarpPointer(awt_display,None,XtWindow(bar),0,0,0,0,7,7);
    }
    XTestFakeButtonEvent(awt_display,Button1,p[2].i!=0,CurrentTime);
    XFlush(awt_display); AWT_UNLOCK(); return p;
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
JAVA_PUBLIC_API(stack_item *) Java_List115_itemCount_stub(stack_item *p,struct execenv *ee)
{
    int value;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNitemCount,&value,NULL);
    p[0].i=value;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Choice115_selected_stub(stack_item *p,struct execenv *ee)
{
    Widget selected;
    XtPointer index=NULL;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNmenuHistory,&selected,NULL);
    if (selected) XtVaGetValues(selected,XmNuserData,&index,NULL);
    p[0].i=(long)index-1;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Choice115_itemCount_stub(stack_item *p,struct execenv *ee)
{
    Widget menu;
    Cardinal count;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNsubMenuId,&menu,NULL);
    XtVaGetValues(menu,XtNnumChildren,&count,NULL);
    p[0].i=count;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Choice115_colorsMatch_stub(stack_item *p,struct execenv *ee)
{
    Widget menu,w;
    WidgetList children;
    Cardinal count,i;
    Pixel foreground,background,fg,bg;
    int matches=1;
    AWT_LOCK(); w=widget(p[0].p);
    XtVaGetValues(w,XmNsubMenuId,&menu,XmNforeground,&foreground,XmNbackground,&background,NULL);
    XtVaGetValues(menu,XtNnumChildren,&count,XtNchildren,&children,NULL);
    for (i=0;i<count;i++) {
        XtVaGetValues(children[i],XmNforeground,&fg,XmNbackground,&bg,NULL);
        if (fg!=foreground || bg!=background) matches=0;
    }
    p[0].i=matches;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Scrollbar115_value_stub(stack_item *p,struct execenv *ee)
{
    static const char *properties[]={XmNvalue,XmNsliderSize,XmNincrement,XmNpageIncrement,XmNminimum,XmNmaximum};
    int value;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),properties[p[1].i],&value,NULL);
    p[0].i=value;
    AWT_UNLOCK();
    return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Scrollbar115_press_stub(stack_item *p,struct execenv *ee)
{
    AWT_LOCK();
    if (p[1].i) XWarpPointer(awt_display,None,XtWindow(widget(p[0].p)),0,0,0,0,10,10);
    XTestFakeButtonEvent(awt_display,Button1,p[1].i!=0,CurrentTime);
    XFlush(awt_display);
    AWT_UNLOCK();
    return p;
}
JAVA_PUBLIC_API(stack_item *) Java_List115_topIndex_stub(stack_item *p,struct execenv *ee)
{
    int value;
    AWT_LOCK();
    XtVaGetValues(widget(p[0].p),XmNtopItemPosition,&value,NULL);
    p[0].i=value-1;
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
