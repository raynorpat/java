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
        x=(long)((unsigned long)x+(unsigned long)unhand(self)->originX);
        y=(long)((unsigned long)y+(unsigned long)unhand(self)->originY);
        right=width>0 && height>0 ? (long)((unsigned long)x+(unsigned long)width) : x;
        bottom=width>0 && height>0 ? (long)((unsigned long)y+(unsigned long)height) : y;
        if (!replace && data->clipped) {
            if (x<data->clip.x) x=data->clip.x;
            if (y<data->clip.y) y=data->clip.y;
            if (right>data->clip.x+data->clip.width) right=data->clip.x+data->clip.width;
            if (bottom>data->clip.y+data->clip.height) bottom=data->clip.y+data->clip.height;
            if (right<x) right=x;
            if (bottom<y) bottom=y;
        }
        data->clip.x=x; data->clip.y=y;
        data->clip.width=(unsigned long)right-(unsigned long)x;
        data->clip.height=(unsigned long)bottom-(unsigned long)y;
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

/* Release 2ec44..2fbe0: the X primitives retain Java's origin and GC mode. */
void sun_awt_motif_X11Graphics_setPaintMode(Hsun_awt_motif_X11Graphics *self)
{
    AwtGraphics *data;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        data->xorMode=0; XSetFunction(awt_display,data->gc,GXcopy);
        XSetForeground(awt_display,data->gc,data->foreground);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_setXORMode(Hsun_awt_motif_X11Graphics *self,Hjava_awt_Color *color)
{
    AwtGraphics *data;
    if (!color) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    AWT_LOCK(); data=graphics(self);
    if (data) {
        data->xorMode=1; data->xorPixel=awt118_pixel(unhand(color)->value);
        XSetFunction(awt_display,data->gc,GXxor);
        XSetForeground(awt_display,data->gc,data->foreground^data->xorPixel);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_drawLine(Hsun_awt_motif_X11Graphics *self,long x1,long y1,long x2,long y2)
{
    AwtGraphics *data;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        XDrawLine(awt_display,data->drawable,data->gc,x1+unhand(self)->originX,y1+unhand(self)->originY,
            x2+unhand(self)->originX,y2+unhand(self)->originY); XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_drawRect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height)
{
    AwtGraphics *data;
    if (width<0 || height<0) return;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        XDrawRectangle(awt_display,data->drawable,data->gc,x+unhand(self)->originX,y+unhand(self)->originY,width,height);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_copyArea(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long dx,long dy)
{
    AwtGraphics *data;
    if (width<=0 || height<=0) return;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        x+=unhand(self)->originX; y+=unhand(self)->originY;
        XCopyArea(awt_display,data->drawable,data->drawable,data->gc,x,y,width,height,x+dx,y+dy);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
static void arc(AwtGraphics *data,long x,long y,long width,long height,long start,long extent,int fill)
{
    if (width<0 || height<0) return;
    if (extent>=360 || extent<=-360) { start=0; extent=360; }
    else start%=360;
    if (fill) XFillArc(awt_display,data->drawable,data->gc,x,y,width,height,start*64,extent*64);
    else XDrawArc(awt_display,data->drawable,data->gc,x,y,width,height,start*64,extent*64);
}
static void draw_arc(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long start,long extent,int fill)
{
    AwtGraphics *data;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        arc(data,x+unhand(self)->originX,y+unhand(self)->originY,width,height,start,extent,fill);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_drawOval(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height)
{ draw_arc(self,x,y,width,height,0,360,0); }
void sun_awt_motif_X11Graphics_fillOval(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height)
{ draw_arc(self,x,y,width,height,0,360,1); }
void sun_awt_motif_X11Graphics_drawArc(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long start,long extent)
{ draw_arc(self,x,y,width,height,start,extent,0); }
void sun_awt_motif_X11Graphics_fillArc(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long start,long extent)
{ draw_arc(self,x,y,width,height,start,extent,1); }

static void round_rect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long arcWidth,long arcHeight,int fill)
{
    AwtGraphics *data;
    long left,right,top,bottom;
    int64_t aw=arcWidth,ah=arcHeight;
    if (width<=0 || height<=0) return;
    if (aw<0) aw=-aw;
    if (ah<0) ah=-ah;
    arcWidth=aw>width ? width : (long)aw; arcHeight=ah>height ? height : (long)ah;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        x+=unhand(self)->originX; y+=unhand(self)->originY;
        left=x+arcWidth/2; right=x+width-arcWidth/2;
        top=y+arcHeight/2; bottom=y+height-arcHeight/2;
        arc(data,x,y,arcWidth,arcHeight,90,90,fill);
        arc(data,x+width-arcWidth,y,arcWidth,arcHeight,0,90,fill);
        arc(data,x,y+height-arcHeight,arcWidth,arcHeight,180,90,fill);
        arc(data,x+width-arcWidth,y+height-arcHeight,arcWidth,arcHeight,270,90,fill);
        if (fill) {
            XFillRectangle(awt_display,data->drawable,data->gc,left,y,right-left,height);
            XFillRectangle(awt_display,data->drawable,data->gc,x,top,left-x,bottom-top);
            XFillRectangle(awt_display,data->drawable,data->gc,right,top,x+width-right,bottom-top);
        } else {
            XDrawLine(awt_display,data->drawable,data->gc,left+1,y,right-1,y);
            XDrawLine(awt_display,data->drawable,data->gc,x+width,top+1,x+width,bottom-1);
            XDrawLine(awt_display,data->drawable,data->gc,left+1,y+height,right-1,y+height);
            XDrawLine(awt_display,data->drawable,data->gc,x,top+1,x,bottom-1);
        }
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_drawRoundRect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long arcWidth,long arcHeight)
{ round_rect(self,x,y,width,height,arcWidth,arcHeight,0); }
void sun_awt_motif_X11Graphics_fillRoundRect(Hsun_awt_motif_X11Graphics *self,long x,long y,long width,long height,long arcWidth,long arcHeight)
{ round_rect(self,x,y,width,height,arcWidth,arcHeight,1); }

static void polygon(Hsun_awt_motif_X11Graphics *self,HArrayOfInt *xs,HArrayOfInt *ys,long count,int close,int fill)
{
    AwtGraphics *data;
    XPoint local[64],*points;
    long i,total=count;
    if (!xs || !ys) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    if (count<0 || count>obj_length(xs) || count>obj_length(ys)) {
        SignalError(EE(),"java/lang/IllegalArgumentException",NULL); return;
    }
    close=close && count>2 && (unhand(xs)->body[count-1]!=unhand(xs)->body[0] || unhand(ys)->body[count-1]!=unhand(ys)->body[0]);
    if (close) total++;
    points=total<=64 ? local : (XPoint *)malloc(total*sizeof(XPoint));
    if (!points) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return; }
    for (i=0;i<count;i++) {
        points[i].x=unhand(xs)->body[i]+unhand(self)->originX;
        points[i].y=unhand(ys)->body[i]+unhand(self)->originY;
    }
    if (close) points[count]=points[0];
    AWT_LOCK(); data=graphics(self);
    if (data && total) {
        if (fill) XFillPolygon(awt_display,data->drawable,data->gc,points,total,Complex,CoordModeOrigin);
        else XDrawLines(awt_display,data->drawable,data->gc,points,total,CoordModeOrigin);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
    if (points!=local) free(points);
}
void sun_awt_motif_X11Graphics_drawPoly(Hsun_awt_motif_X11Graphics *self,HArrayOfInt *xs,HArrayOfInt *ys,long count,long close)
{ polygon(self,xs,ys,count,close!=0,0); }
void sun_awt_motif_X11Graphics_fillPolygon(Hsun_awt_motif_X11Graphics *self,HArrayOfInt *xs,HArrayOfInt *ys,long count)
{ polygon(self,xs,ys,count,0,1); }

/* Release 2e7cc..2eaa8 and 3240c: single-font arrays are capped at 1024;
 * multifont segments return their advance to the unchanged Java renderer. */
void sun_awt_motif_X11Graphics_drawSFChars(Hsun_awt_motif_X11Graphics *self,HArrayOfChar *chars,long offset,long length,long x,long y)
{
    AwtGraphics *data;
    XChar2b converted[1024];
    long i;
    unicode value;
    if (!chars) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    if (offset<0 || length<0 || offset>obj_length(chars) || length>obj_length(chars)-offset) {
        SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return;
    }
    if (length>1024) length=1024;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        for (i=0;i<length;i++) {
            value=unhand(chars)->body[offset+i];
            converted[i].byte1=value>>8; converted[i].byte2=value&255;
        }
        XDrawString16(awt_display,data->drawable,data->gc,x+unhand(self)->originX,y+unhand(self)->originY,converted,length);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
void sun_awt_motif_X11Graphics_drawBytes(Hsun_awt_motif_X11Graphics *self,HArrayOfByte *bytes,long offset,long length,long x,long y)
{
    AwtGraphics *data;
    if (!bytes) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    if (offset<0 || length<0 || offset>obj_length(bytes) || length>obj_length(bytes)-offset) {
        SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return;
    }
    if (length>1024) length=1024;
    AWT_LOCK(); data=graphics(self);
    if (data) {
        XDrawString(awt_display,data->drawable,data->gc,x+unhand(self)->originX,y+unhand(self)->originY,unhand(bytes)->body+offset,length);
        XFlush(awt_display);
    }
    AWT_UNLOCK();
}
long sun_awt_motif_X11Graphics_drawMFCharsSegment(Hsun_awt_motif_X11Graphics *self,long bounding,
    struct Hjava_awt_Font *font,struct Hsun_awt_FontDescriptor *descriptor,HArrayOfByte *bytes,long length,long x,long y)
{
    AwtGraphics *data;
    XFontStruct *xfont;
    long advance=0;
    if (!font || !descriptor || !bytes) { SignalError(EE(),"java/lang/NullPointerException",NULL); return 0; }
    if (length<0 || length>obj_length(bytes)) { SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return 0; }
    AWT_LOCK(); data=graphics(self);
    if (data && (xfont=awt118_segment_font(font,descriptor))!=NULL) {
        XSetFont(awt_display,data->gc,xfont->fid);
        x+=unhand(self)->originX; y+=unhand(self)->originY;
        if (xfont->min_byte1 || xfont->max_byte1) {
            XDrawString16(awt_display,data->drawable,data->gc,x,y,(XChar2b *)unhand(bytes)->body,length/2);
            advance=XTextWidth16(xfont,(XChar2b *)unhand(bytes)->body,length/2);
        } else {
            XDrawString(awt_display,data->drawable,data->gc,x,y,unhand(bytes)->body,length);
            advance=XTextWidth(xfont,unhand(bytes)->body,length);
        }
    }
    AWT_UNLOCK(); return advance;
}
