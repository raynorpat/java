/* Solaris libawt.so 2e41c/2e498/2e770/2eea0/2f4c4; lazy X11 GC ownership. */
#include "awt118.h"
#include "sun_awt_motif_X11Graphics.h"
#include "java_awt_Component.h"
#include "java_awt_Color.h"
#include <stdlib.h>

typedef struct {
    AwtPeer *peer;
    Drawable drawable;
    GC gc;
    XRectangle clip;
    int clipped;
    unsigned long foreground, xorPixel;
    int xorMode;
} AwtGraphics;

static AwtGraphics *graphics(Hsun_awt_motif_X11Graphics *self)
{
    AwtGraphics *data=(AwtGraphics *)unhand(self)->pData;
    if (!data || !data->peer || !data->peer->widget || !XtIsRealized(data->peer->widget)) return NULL;
    if (!data->gc) {
        data->drawable=XtWindow(data->peer->widget);
        data->gc=XCreateGC(awt_display,data->drawable,0,NULL);
        if (!data->gc) return NULL;
        XSetGraphicsExposures(awt_display,data->gc,True);
    }
    return data;
}
void sun_awt_motif_X11Graphics_createFromComponent(Hsun_awt_motif_X11Graphics *self,Hsun_awt_motif_MComponentPeer *peer)
{
    AwtGraphics *data;
    AWT_LOCK();
    data=(AwtGraphics *)calloc(1,sizeof(*data));
    if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    else {
        data->peer=awt118_peer(peer);
        if (data->peer) data->peer->references++;
        unhand(self)->pData=(long)data;
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_createFromGraphics(Hsun_awt_motif_X11Graphics *self,Hsun_awt_motif_X11Graphics *source)
{
    AwtGraphics *from,*data;
    AWT_LOCK(); from=source ? (AwtGraphics *)unhand(source)->pData : NULL;
    if (!from) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        data=(AwtGraphics *)malloc(sizeof(*data));
        if (!data) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else {
            *data=*from; data->gc=NULL;
            if (data->peer) data->peer->references++;
            unhand(self)->pData=(long)data;
            if (graphics(source) && graphics(self)) {
                XCopyGC(awt_display,from->gc,GCForeground|GCBackground|GCFunction|GCFont,data->gc);
                if (data->clipped) XSetClipRectangles(awt_display,data->gc,0,0,&data->clip,1,Unsorted);
            }
        }
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_disposeImpl(Hsun_awt_motif_X11Graphics *self)
{
    AwtGraphics *data;
    AWT_LOCK(); data=(AwtGraphics *)unhand(self)->pData;
    if (data) {
        if (data->gc) XFreeGC(awt_display,data->gc);
        if (data->peer) awt118_release_peer(data->peer);
        free(data); unhand(self)->pData=0;
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_pSetForeground(Hsun_awt_motif_X11Graphics *self,Hjava_awt_Color *color)
{
    AwtGraphics *data;
    if (!color) return;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        data->foreground=awt118_pixel(unhand(color)->value);
        XSetForeground(awt_display,data->gc,data->foreground ^ (data->xorMode ? data->xorPixel : 0));
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_pSetFont(Hsun_awt_motif_X11Graphics *self,struct Hjava_awt_Font *font)
{
    AwtGraphics *data;
    XFontStruct *xfont;
    AWT_LOCK(); data=graphics(self);
    if (data && (xfont=awt118_font(font))!=NULL) XSetFont(awt_display,data->gc,xfont->fid);
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_changeClip(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long replace)
{
    AwtGraphics *data;
    long right,bottom;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        x+=unhand(self)->originX; y+=unhand(self)->originY;
        right=x+(width>0 ? width : 0); bottom=y+(height>0 ? height : 0);
        if (!replace && data->clipped) {
            if (x<data->clip.x) x=data->clip.x;
            if (y<data->clip.y) y=data->clip.y;
            if (right>data->clip.x+data->clip.width) right=data->clip.x+data->clip.width;
            if (bottom>data->clip.y+data->clip.height) bottom=data->clip.y+data->clip.height;
        }
        data->clip.x=x; data->clip.y=y;
        data->clip.width=right>x ? right-x : 0; data->clip.height=bottom>y ? bottom-y : 0;
        data->clipped=1;
        XSetClipRectangles(awt_display,data->gc,0,0,&data->clip,1,Unsorted);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_removeClip(Hsun_awt_motif_X11Graphics *self)
{
    AwtGraphics *data;
    AWT_LOCK(); data=graphics(self);
    if (data) { data->clipped=0; XSetClipMask(awt_display,data->gc,None); }
    AWT_UNLOCK();
}
struct Hjava_awt_Rectangle *sun_awt_motif_X11Graphics_getClipBounds(Hsun_awt_motif_X11Graphics *self)
{
    AwtGraphics *data;
    struct Hjava_awt_Rectangle *result=NULL;
    AWT_LOCK(); data=(AwtGraphics *)unhand(self)->pData;
    if (data && data->clipped) result=(struct Hjava_awt_Rectangle *)execute_java_constructor(EE(),
        "java/awt/Rectangle",NULL,"(IIII)",data->clip.x-unhand(self)->originX,
        data->clip.y-unhand(self)->originY,data->clip.width,data->clip.height);
    AWT_UNLOCK();
    return result;
}
void sun_awt_motif_X11Graphics_fillRect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height)
{
    AwtGraphics *data;
    if (width<=0 || height<=0) return;
    AWT_LOCK(); data=graphics(self);
    if (data) XFillRectangle(awt_display,data->drawable,data->gc,x+unhand(self)->originX,
        y+unhand(self)->originY,width,height);
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_clearRect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height)
{
    AwtGraphics *data;
    Hjava_awt_Color *background;
    if (width<=0 || height<=0) return;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        background=unhand(unhand(unhand(self)->peer)->target)->background;
        XSetFunction(awt_display,data->gc,GXcopy);
        XSetForeground(awt_display,data->gc,background ? awt118_pixel(unhand(background)->value) : WhitePixel(awt_display,awt_screen));
        XFillRectangle(awt_display,data->drawable,data->gc,x+unhand(self)->originX,y+unhand(self)->originY,width,height);
        XSetForeground(awt_display,data->gc,data->foreground ^ (data->xorMode ? data->xorPixel : 0));
        XSetFunction(awt_display,data->gc,data->xorMode ? GXxor : GXcopy);
    }
    AWT_UNLOCK();
}
