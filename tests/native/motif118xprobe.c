/* Test-only X11 event/pixel observation through the live peer's widget. */
#include "awt118.h"
#include "java_awt_Component.h"
#include "java_awt_MenuComponent.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_X11Graphics.h"
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Shell.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/ScrolledW.h>
#include <Xm/FileSB.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <stdlib.h>
#include <string.h>

static Widget widget(Hjava_awt_Component *target)
{
    Hsun_awt_motif_MComponentPeer *peer=(Hsun_awt_motif_MComponentPeer *)unhand(target)->peer;
    AwtPeer *data=peer ? awt118_peer(peer) : NULL;
    return data ? (data->content ? data->content : data->widget) : NULL;
}

static int warning_banner(Widget w,const char *expected)
{
    if (XmIsLabel(w)) {
        XmString value; char *text=NULL; XColor background,foreground;
        XWindowAttributes attributes; int match;
        XtVaGetValues(w,XmNlabelString,&value,XmNbackground,&background.pixel,
            XmNforeground,&foreground.pixel,NULL);
        XmStringGetLtoR(value,XmFONTLIST_DEFAULT_TAG,&text);
        match=text && !strcmp(text,expected);
        if (text) XtFree(text);
        XmStringFree(value);
        if (match && XtIsRealized(w)) {
            XImage *image; int painted;
            XQueryColor(awt_display,awt_cmap,&background); XQueryColor(awt_display,awt_cmap,&foreground);
            XGetWindowAttributes(awt_display,XtWindow(w),&attributes);
            if (attributes.map_state!=IsViewable) return 0;
            image=XGetImage(awt_display,XtWindow(w),attributes.width-1,attributes.height-1,1,1,AllPlanes,ZPixmap);
            painted=image && XGetPixel(image,0,0)==background.pixel;
            if(image) XDestroyImage(image);
            return painted && (background.red>>8)==255 &&
                (background.green>>8)==225 && !background.blue &&
                !foreground.red && !foreground.green && !foreground.blue;
        }
    }
    if (XtIsComposite(w)) {
        WidgetList children; Cardinal count,i;
        XtVaGetValues(w,XtNchildren,&children,XtNnumChildren,&count,NULL);
        for(i=0;i<count;i++) if(warning_banner(children[i],expected)) return 1;
    }
    return 0;
}
JAVA_PUBLIC_API(stack_item *) Java_Warning115_banner_stub(stack_item *p,struct execenv *ee)
{
    Widget w; char *text; int result;
    AWT_LOCK(); w=widget(p[0].p); text=allocCString(p[1].p);
    while(w && !XtIsShell(w)) w=XtParent(w);
    result=w && warning_banner(w,text); free(text);
    AWT_UNLOCK(); p[0].i=result; return p+1;
}

static int embedded_retired;
static KeyCode test_alt,test_meta;
JAVA_PUBLIC_API(stack_item *) Java_Modifiers115_remap_stub(stack_item *p,struct execenv *ee)
{
    int minimum,maximum,i,result;
    KeySym symbols[2]={XK_Alt_L,XK_Meta_L};
    XModifierKeymap *mapping;
    AWT_LOCK(); XDisplayKeycodes(awt_display,&minimum,&maximum);
    test_alt=maximum-1; test_meta=maximum;
    XChangeKeyboardMapping(awt_display,test_alt,1,symbols,2);
    mapping=XGetModifierMapping(awt_display);
    for(i=0;i<8*mapping->max_keypermod;i++)
        if(mapping->modifiermap[i]==test_alt || mapping->modifiermap[i]==test_meta) mapping->modifiermap[i]=0;
    mapping=XInsertModifiermapEntry(mapping,test_alt,p[0].i ? Mod3MapIndex : Mod5MapIndex);
    mapping=XInsertModifiermapEntry(mapping,test_meta,p[0].i ? Mod5MapIndex : Mod3MapIndex);
    result=XSetModifierMapping(awt_display,mapping); XFreeModifiermap(mapping);
    XSync(awt_display,False); AWT_UNLOCK(); p[0].i=result==MappingSuccess; return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Modifiers115_key_stub(stack_item *p,struct execenv *ee)
{
    KeyCode modifier=p[0].i ? test_meta : test_alt,letter;
    AWT_LOCK(); letter=XKeysymToKeycode(awt_display,XK_a);
    XTestFakeKeyEvent(awt_display,modifier,True,CurrentTime);
    XTestFakeKeyEvent(awt_display,letter,True,CurrentTime);
    XTestFakeKeyEvent(awt_display,letter,False,CurrentTime);
    XTestFakeKeyEvent(awt_display,modifier,False,CurrentTime);
    XFlush(awt_display); AWT_UNLOCK(); return p;
}
JAVA_PUBLIC_API(stack_item *) Java_Modifiers115_keypad_stub(stack_item *p,struct execenv *ee)
{
    XModifierKeymap *mapping;
    KeyCode lock,key; unsigned int mask=0; int i;
    AWT_LOCK(); lock=XKeysymToKeycode(awt_display,XK_Num_Lock);
    mapping=XGetModifierMapping(awt_display);
    for(i=0;i<8*mapping->max_keypermod;i++)
        if(mapping->modifiermap[i]==lock) mask|=1U<<(i/mapping->max_keypermod);
    XFreeModifiermap(mapping);
    XkbLockModifiers(awt_display,XkbUseCoreKbd,mask,p[0].i ? mask : 0);
    key=XKeysymToKeycode(awt_display,XK_KP_Home);
    XTestFakeKeyEvent(awt_display,key,True,CurrentTime);
    XTestFakeKeyEvent(awt_display,key,False,CurrentTime);
    XFlush(awt_display); AWT_UNLOCK(); return p;
}
static void embedded_destroyed(Widget widget,XtPointer closure,XtPointer call)
{ embedded_retired++; }
JAVA_PUBLIC_API(stack_item *) Java_Embedded115_host_stub(stack_item *p,struct execenv *ee)
{
    Widget shell;
    AWT_LOCK();
    shell=XtVaAppCreateShell("EmbeddedHost","XApplication",vendorShellWidgetClass,awt_display,
        XtNvisual,awt_visInfo.visual,XtNdepth,awt_visInfo.depth,XtNcolormap,awt_cmap,
        XtNwidth,160,XtNheight,100,XtNallowShellResize,True,XtNmappedWhenManaged,False,NULL);
    XtAddCallback(shell,XtNdestroyCallback,embedded_destroyed,NULL);
    AWT_UNLOCK(); p[0].i=(long)shell; return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Embedded115_retired_stub(stack_item *p,struct execenv *ee)
{ p[0].i=embedded_retired; return p+1; }
JAVA_PUBLIC_API(stack_item *) Java_Embedded115_destroyHost_stub(stack_item *p,struct execenv *ee)
{ AWT_LOCK(); XtDestroyWidget((Widget)p[0].i); AWT_UNLOCK(); return p; }
JAVA_PUBLIC_API(stack_item *) Java_Menus115_click_stub(stack_item *p,struct execenv *ee)
{
    Widget item;
    Position x,y; Dimension width,height;
    AWT_LOCK(); item=awt118_menu_widget((HObject *)unhand((Hjava_awt_MenuComponent *)p[0].p)->peer);
    if (!item || !XtIsRealized(item)) SignalError(ee,"java/lang/RuntimeException","menu not realized");
    else {
        XtVaGetValues(item,XmNwidth,&width,XmNheight,&height,NULL);
        XtTranslateCoords(item,width/2,height/2,&x,&y);
        XTestFakeMotionEvent(awt_display,awt_screen,x,y,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,True,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,False,CurrentTime); XFlush(awt_display);
    }
    AWT_UNLOCK(); return p;
}
JAVA_PUBLIC_API(stack_item *) Java_FileDialog115_choose_stub(stack_item *p,struct execenv *ee)
{
    Widget dialog,button,field;
    char *path;
    Position x,y; Dimension width,height;
    AWT_LOCK(); dialog=widget(p[0].p);
    if (!dialog || !XtIsRealized(dialog)) SignalError(ee,"java/lang/RuntimeException","file dialog not realized");
    else {
        if (!p[2].i) {
            path=allocCString(p[1].p); field=XmFileSelectionBoxGetChild(dialog,XmDIALOG_TEXT);
            XmTextFieldSetString(field,path); free(path);
        }
        button=XmFileSelectionBoxGetChild(dialog,p[2].i ? XmDIALOG_CANCEL_BUTTON : XmDIALOG_OK_BUTTON);
        XtVaGetValues(button,XmNwidth,&width,XmNheight,&height,NULL); XtTranslateCoords(button,width/2,height/2,&x,&y);
        XTestFakeMotionEvent(awt_display,awt_screen,x,y,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,True,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,False,CurrentTime); XFlush(awt_display);
    }
    AWT_UNLOCK(); return p;
}
JAVA_PUBLIC_API(stack_item *) Java_FileDialog115_listed_stub(stack_item *p,struct execenv *ee)
{
    int count=0;
    AWT_LOCK(); XtVaGetValues(widget(p[0].p),XmNfileListItemCount,&count,NULL);
    p[0].i=count; AWT_UNLOCK(); return p+1;
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

JAVA_PUBLIC_API(stack_item *) Java_InputMethod115_compose_stub(stack_item *p,struct execenv *ee)
{
    int minimum,maximum,i;
    KeySym symbol=XK_Multi_key, sequence[2]={XK_apostrophe,XK_e};
    XMappingEvent mapping;
    AWT_LOCK(); XDisplayKeycodes(awt_display,&minimum,&maximum);
    XChangeKeyboardMapping(awt_display,maximum,1,&symbol,1); XSync(awt_display,False);
    memset(&mapping,0,sizeof(mapping)); mapping.display=awt_display; mapping.request=MappingKeyboard;
    mapping.first_keycode=maximum; mapping.count=1; XRefreshKeyboardMapping(&mapping);
    XTestFakeKeyEvent(awt_display,maximum,True,CurrentTime);
    XTestFakeKeyEvent(awt_display,maximum,False,CurrentTime);
    for(i=0;i<2;i++) {
        KeyCode key=XKeysymToKeycode(awt_display,sequence[i]);
        XTestFakeKeyEvent(awt_display,key,True,CurrentTime);
        XTestFakeKeyEvent(awt_display,key,False,CurrentTime);
    }
    XFlush(awt_display); AWT_UNLOCK(); return p;
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
JAVA_PUBLIC_API(stack_item *) Java_Choice115_mouse_stub(stack_item *p,struct execenv *ee)
{
    AWT_LOCK();
    if (p[1].i) XWarpPointer(awt_display,None,XtWindow(widget(p[0].p)),0,0,0,0,10,10);
    XTestFakeButtonEvent(awt_display,Button1,p[1].i,CurrentTime);
    XFlush(awt_display);
    AWT_UNLOCK();
    return p;
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

JAVA_PUBLIC_API(stack_item *) Java_LatePanel115_sizeMatches_stub(stack_item *p,struct execenv *ee)
{
    Hjava_awt_Component *target=p[0].p;
    Widget w;
    Dimension width=0,height=0;
    AWT_LOCK(); w=widget(target);
    if(w) XtVaGetValues(w,XtNwidth,&width,XtNheight,&height,NULL);
    p[0].i=w && width==unhand(target)->width && height==unhand(target)->height;
    AWT_UNLOCK(); return p+1;
}

JAVA_PUBLIC_API(stack_item *) Java_Raster115_pixels_stub(stack_item *p,struct execenv *ee)
{
    XImage *image;
    HArrayOfInt *pixels;
    int x,y,width=p[3].i,height=p[4].i;
    AWT_LOCK();
    image=XGetImage(awt_display,XtWindow(widget(p[0].p)),p[1].i,p[2].i,width,height,AllPlanes,ZPixmap);
    pixels=(HArrayOfInt *)ArrayAlloc(T_INT,width*height);
    if (image && pixels) {
        for (y=0;y<height;y++) for (x=0;x<width;x++) unhand(pixels)->body[y*width+x]=XGetPixel(image,x,y);
    }
    if (image) XDestroyImage(image);
    p[0].p=pixels;
    AWT_UNLOCK(); return p+1;
}

JAVA_PUBLIC_API(stack_item *) Java_Raster115_singleFont_stub(stack_item *p,struct execenv *ee)
{
    sun_awt_motif_X11Graphics_drawSFChars(p[0].p,p[1].p,p[2].i,p[3].i,p[4].i,p[5].i);
    return p;
}

JAVA_PUBLIC_API(stack_item *) Java_Icon115_sizes_stub(stack_item *p,struct execenv *ee)
{
    XIconSize size={8,8,32,32,8,8};
    AWT_LOCK(); XSetIconSizes(awt_display,RootWindow(awt_display,awt_screen),&size,1); AWT_UNLOCK();
    return p;
}

static Widget print_button(Widget widget,const char *wanted)
{
    if (XmIsPushButton(widget) && XtIsManaged(widget) && XtIsRealized(widget)) {
        XmString label; char *text=NULL; int match;
        XWindowAttributes attributes;
        if (!XGetWindowAttributes(awt_display,XtWindow(widget),&attributes) || attributes.map_state!=IsViewable) return NULL;
        XtVaGetValues(widget,XmNlabelString,&label,NULL);
        XmStringGetLtoR(label,XmFONTLIST_DEFAULT_TAG,&text);
        match=text && !strcmp(text,wanted);
        if(text) XtFree(text); XmStringFree(label);
        if(match) return widget;
    }
    if(XtIsComposite(widget)) {
        WidgetList children; Cardinal count,i;
        XtVaGetValues(widget,XtNchildren,&children,XtNnumChildren,&count,NULL);
        for(i=0;i<count;i++) { Widget found=print_button(children[i],wanted); if(found) return found; }
    }
    return NULL;
}
JAVA_PUBLIC_API(stack_item *) Java_Print115_choose_stub(stack_item *p,struct execenv *ee)
{
    Window root,parent,*children=NULL; unsigned count=0,i;
    Widget button=NULL;
    const char *label=p[0].i ? "Cancel" : "Print";
    AWT_LOCK();
    XQueryTree(awt_display,RootWindow(awt_display,awt_screen),&root,&parent,&children,&count);
    for(i=0;i<count && !button;i++) {
        Widget shell=XtWindowToWidget(awt_display,children[i]);
        if(shell) button=print_button(shell,label);
    }
    if(children) XFree(children);
    if(button) {
        Position x,y; Dimension width,height;
        XtVaGetValues(button,XmNwidth,&width,XmNheight,&height,NULL);
        XtTranslateCoords(button,width/2,height/2,&x,&y);
        XTestFakeMotionEvent(awt_display,awt_screen,x,y,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,True,CurrentTime);
        XTestFakeButtonEvent(awt_display,Button1,False,CurrentTime); XFlush(awt_display);
    }
    AWT_UNLOCK(); p[0].i=button!=NULL; return p+1;
}
JAVA_PUBLIC_API(stack_item *) Java_Icon115_pixel_stub(stack_item *p,struct execenv *ee)
{
    Widget w;
    XWMHints *hints;
    Window root;
    int x,y;
    unsigned width,height,border,depth;
    XImage *image=NULL;
    long pixel=0;
    AWT_LOCK(); w=widget(p[0].p);
    while(XtParent(w)) w=XtParent(w);
    hints=XGetWMHints(awt_display,XtWindow(w));
    if(!hints || !(hints->flags&IconPixmapHint) || !(hints->flags&IconWindowHint))
        SignalError(ee,"java/lang/RuntimeException","missing icon hints");
    else if(!XGetGeometry(awt_display,hints->icon_pixmap,&root,&x,&y,&width,&height,&border,&depth) || width!=16 || height!=16)
        SignalError(ee,"java/lang/RuntimeException","icon size");
    else image=XGetImage(awt_display,hints->icon_pixmap,0,0,1,1,AllPlanes,ZPixmap);
    if(image) { pixel=XGetPixel(image,0,0); XDestroyImage(image); }
    if(hints) XFree(hints);
    AWT_UNLOCK(); p[0].i=pixel; return p+1;
}
