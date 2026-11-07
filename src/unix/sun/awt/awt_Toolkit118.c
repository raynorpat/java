/* Reconstructed from Solaris libawt_g.so 416ac/42b94 and libawt.so
 * 22894/23174/23530..23694. See tests/awt118/bootstrap-evidence.md. */
#include "awt118.h"
#include "sun_awt_motif_InputThread.h"
#include <X11/Shell.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <locale.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

Display *awt_display;
XtAppContext awt_appContext;
XVisualInfo awt_visInfo;
Colormap awt_cmap;
int awt_screen;
Hsun_awt_motif_MToolkit *awt_lock;
static int inputReady;

/* Release 22d78: optional input waiter notifies the dispatch loop, then waits
 * until that loop drains the pending Xt work. Dispatch remains on MToolkit. */
void sun_awt_motif_InputThread_run(Hsun_awt_motif_InputThread *self)
{
    struct pollfd fd;
    AWT_LOCK();
    for(;;) {
        while(!XtAppPending(awt_appContext)) {
            fd.fd=ConnectionNumber(awt_display); fd.events=POLLIN;
            AWT_UNLOCK(); poll(&fd,1,250); AWT_LOCK();
        }
        inputReady=1; monitorNotifyAll((unsigned int)awt_lock);
        while(inputReady) monitorWait((unsigned int)awt_lock,-1);
    }
}

void sun_awt_motif_MToolkit_init(Hsun_awt_motif_MToolkit *self)
{
    int argc = 0, count;
    char *argv[] = { NULL };
    XVisualInfo query, *info;
    awt_lock = self;
    AWT_LOCK();
    if (!XSupportsLocale()) setlocale(LC_ALL, "C");
    XSetLocaleModifiers("");
    XtToolkitInitialize();
    awt_appContext = XtCreateApplicationContext();
    awt_display = XtOpenDisplay(awt_appContext, getenv("DISPLAY") ? NULL : ":0.0",
        "MToolkit", "XApplication", NULL, 0, &argc, argv);
    if (!awt_display) {
        XtDestroyApplicationContext(awt_appContext);
        awt_appContext = NULL;
        SignalError(EE(), "java/lang/InternalError", "Can't connect to X11 window server");
        AWT_UNLOCK();
        return;
    }
    awt_screen = DefaultScreen(awt_display);
    if (getenv("FORCEDEFVIS") || !XMatchVisualInfo(awt_display, awt_screen, 24, TrueColor, &awt_visInfo)) {
        query.visualid = XVisualIDFromVisual(DefaultVisual(awt_display, awt_screen));
        query.depth = DefaultDepth(awt_display, awt_screen);
        info = XGetVisualInfo(awt_display, VisualIDMask | VisualDepthMask, &query, &count);
        if (!info) {
            SignalError(EE(), "java/lang/InternalError", "Can't find default X11 visual");
            AWT_UNLOCK();
            return;
        }
        awt_visInfo = *info;
        XFree(info);
    }
    awt_cmap = awt_visInfo.visual == DefaultVisual(awt_display, awt_screen)
        ? DefaultColormap(awt_display, awt_screen)
        : XCreateColormap(awt_display, RootWindow(awt_display, awt_screen), awt_visInfo.visual, AllocNone);
    awt118_init_colors();
    AWT_UNLOCK();
}

void sun_awt_motif_MToolkit_run(Hsun_awt_motif_MToolkit *self)
{
    struct pollfd fd;
    XEvent event;
    XtInputMask pending;
    AWT_LOCK();
    for (;;) {
        while (awt118_dispatch_putback() || (pending=XtAppPending(awt_appContext))!=0) {
            pending=XtAppPending(awt_appContext);
            if (pending&XtIMXEvent) {
                XtAppNextEvent(awt_appContext,&event);
                if (!awt118_xim_event(&event) && !awt118_filter_input(&event)) XtDispatchEvent(&event);
            } else if (pending) XtAppProcessEvent(awt_appContext,pending);
        }
        XFlush(awt_display);
        inputReady=0;
        monitorNotifyAll((unsigned int)awt_lock);
        fd.fd = ConnectionNumber(awt_display);
        fd.events = POLLIN;
        AWT_UNLOCK();
        poll(&fd, 1, 250);
        AWT_LOCK();
    }
}

long sun_awt_motif_MToolkit_getScreenWidth(Hsun_awt_motif_MToolkit *self)
{ return DisplayWidth(awt_display, awt_screen); }
long sun_awt_motif_MToolkit_getScreenHeight(Hsun_awt_motif_MToolkit *self)
{ return DisplayHeight(awt_display, awt_screen); }
long sun_awt_motif_MToolkit_getScreenResolution(Hsun_awt_motif_MToolkit *self)
{ return (long)(DisplayWidth(awt_display, awt_screen) * 25.4 / DisplayWidthMM(awt_display, awt_screen)); }
void sun_awt_motif_MToolkit_sync(Hsun_awt_motif_MToolkit *self)
{ AWT_LOCK(); XSync(awt_display, False); AWT_UNLOCK(); }
void sun_awt_motif_MToolkit_beep(Hsun_awt_motif_MToolkit *self)
{ AWT_LOCK(); XBell(awt_display, 0); AWT_UNLOCK(); }

struct Hjava_awt_image_ColorModel *sun_awt_motif_MToolkit_makeColorModel(Hsun_awt_motif_MToolkit *self)
{
    if (awt_visInfo.class != TrueColor) {
        return awt118_index_model();
    }
    return (struct Hjava_awt_image_ColorModel *)execute_java_constructor(EE(),
        "java/awt/image/DirectColorModel", NULL, "(IIIII)", awt_visInfo.depth,
        awt_visInfo.red_mask, awt_visInfo.green_mask, awt_visInfo.blue_mask, 0);
}

void sun_awt_motif_MToolkit_loadSystemColors(Hsun_awt_motif_MToolkit *self, HArrayOfInt *array)
{
    Widget shell, panel, button, menu, text, scroll;
    Pixel bg, fg, top, bottom, pixels[26];
    Colormap cmap;
    XColor color;
    int i;
    AWT_LOCK();
    for (i=0; i<26; i++) pixels[i] = (Pixel)-1;
    shell = XtAppCreateShell("awtcolors", "XApplication", vendorShellWidgetClass, awt_display, NULL, 0);
    panel = XmCreateDrawingArea(shell, "awtpanelcolor", NULL, 0);
    button = XmCreatePushButton(panel, "awtcontrolcolor", NULL, 0);
    menu = XmCreatePulldownMenu(button, "awtcolormenu", NULL, 0);
    text = XmCreateText(panel, "awttextcolor", NULL, 0);
    scroll = XmCreateScrollBar(panel, "awtscrollbarcolor", NULL, 0);
    XtVaGetValues(panel, XmNbackground, &bg, XmNforeground, &fg, XmNcolormap, &cmap, NULL);
    pixels[7]=pixels[24]=bg; pixels[9]=pixels[25]=fg;
    XtVaGetValues(menu, XmNbackground, &bg, XmNforeground, &fg, NULL);
    pixels[10]=bg; pixels[11]=fg;
    XtVaGetValues(text, XmNbackground, &bg, XmNforeground, &fg, NULL);
    pixels[12]=pixels[15]=bg; pixels[13]=pixels[14]=fg;
    XtVaGetValues(button, XmNbackground, &bg, XmNforeground, &fg,
        XmNtopShadowColor, &top, XmNbottomShadowColor, &bottom, NULL);
    pixels[17]=bg; pixels[18]=fg; pixels[19]=pixels[20]=top; pixels[21]=pixels[22]=bottom;
    XtVaGetValues(scroll, XmNbackground, &bg, NULL); pixels[23]=bg;
    for (i=0; i<26; i++) if (pixels[i] != (Pixel)-1) {
        color.pixel = pixels[i]; XQueryColor(awt_display, cmap, &color);
        unhand(array)->body[i] = 0xff000000 | ((color.red >> 8) << 16) | ((color.green >> 8) << 8) | (color.blue >> 8);
    }
    XtDestroyWidget(shell);
    AWT_UNLOCK();
}
