/* Solaris libawt.so 32ecc/33620/36280/36314: offscreen storage and delivery. */
#include "awt_Image118.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include "java_awt_Rectangle.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

AwtImage *awt118_image(Hsun_awt_image_ImageRepresentation *self,Hjava_awt_Color *background)
{
    AwtImage *data;
    GC gc;
    if (!self) return NULL;
    data=(AwtImage *)unhand(self)->pData;
    if (!data && (unhand(self)->availinfo&3)==3 && unhand(self)->width>0 && unhand(self)->height>0) {
        data=(AwtImage *)calloc(1,sizeof(*data));
        if (!data) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return NULL; }
        data->width=unhand(self)->width; data->height=unhand(self)->height; data->references=1;
        data->hints=unhand(self)->hints;
        data->background=background ? awt118_pixel(unhand(background)->value) : awt118_pixel(0xffffff);
        data->pixmap=XCreatePixmap(awt_display,RootWindow(awt_display,awt_screen),data->width,data->height,awt_visInfo.depth);
        gc=XCreateGC(awt_display,data->pixmap,0,NULL);
        XSetForeground(awt_display,gc,data->background);
        XFillRectangle(awt_display,data->pixmap,gc,0,0,data->width,data->height);
        XFreeGC(awt_display,gc);
        unhand(self)->pData=(long)data;
    }
    return data;
}
void awt118_release_image(AwtImage *data)
{
    if (--data->references==0) {
        if (data->pixmap) XFreePixmap(awt_display,data->pixmap);
        free(data->coverage);
        free(data->rows);
        free(data);
    }
}
void sun_awt_image_ImageRepresentation_offscreenInit(Hsun_awt_image_ImageRepresentation *self,Hjava_awt_Color *background)
{
    AWT_LOCK();
    if (!self) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else if (unhand(self)->width<=0 || unhand(self)->height<=0) SignalError(EE(),"java/lang/IllegalArgumentException",NULL);
    else {
        unhand(self)->availinfo=43;
        if (!awt118_image(self,background)) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    }
    AWT_UNLOCK();
}
void sun_awt_image_ImageRepresentation_disposeImage(Hsun_awt_image_ImageRepresentation *self)
{
    AwtImage *data;
    AWT_LOCK();
    if (!self) SignalError(EE(),"java/lang/NullPointerException",NULL);
    else {
        data=(AwtImage *)unhand(self)->pData; unhand(self)->pData=0;
        if (data) { data->retired=1; awt118_release_image(data); }
    }
    AWT_UNLOCK();
}
void sun_awt_image_OffScreenImageSource_sendPixels(Hsun_awt_image_OffScreenImageSource *self)
{
    AwtImage *data;
    HArrayOfInt *pixels;
    HObject *model,*consumer;
    XImage *row;
    int x,y;
    if (!self || !unhand(self)->baseIR) { SignalError(EE(),"java/lang/NullPointerException",NULL); return; }
    if ((unhand(unhand(self)->baseIR)->availinfo&43)!=43) { SignalError(EE(),"java/lang/IllegalAccessError",NULL); return; }
    AWT_LOCK(); data=awt118_image(unhand(self)->baseIR,NULL);
    if (data) data->references++;
    AWT_UNLOCK();
    if (!data) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return; }
    model=(HObject *)sun_awt_motif_MToolkit_makeColorModel(awt_lock);
    consumer=(HObject *)unhand(self)->theConsumer;
    if (model && consumer && !exceptionOccurred(EE()))
        execute_java_dynamic_method(EE(),consumer,"setColorModel","(Ljava/awt/image/ColorModel;)V",model);
    consumer=(HObject *)unhand(self)->theConsumer;
    if (consumer && !exceptionOccurred(EE())) execute_java_dynamic_method(EE(),consumer,"setHints","(I)V",14);
    pixels=(HArrayOfInt *)ArrayAlloc(T_INT,data->width);
    if (!pixels) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
    for (y=0;pixels && y<data->height && !exceptionOccurred(EE());y++) {
        consumer=(HObject *)unhand(self)->theConsumer;
        if (!consumer) break;
        AWT_LOCK();
        if (data->retired) { AWT_UNLOCK(); break; }
        row=XGetImage(awt_display,data->pixmap,0,y,data->width,1,AllPlanes,ZPixmap);
        if (row) {
            for (x=0;x<data->width;x++) unhand(pixels)->body[x]=XGetPixel(row,x,0);
            XDestroyImage(row);
        }
        AWT_UNLOCK();
        if (!row) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); break; }
        execute_java_dynamic_method(EE(),consumer,"setPixels","(IIIILjava/awt/image/ColorModel;[III)V",
            0,y,data->width,1,model,pixels,0,data->width);
    }
    AWT_LOCK(); awt118_release_image(data); AWT_UNLOCK();
}

/* Release 336a4/338a0: validate the source rectangle before conversion. */
static long set_pixels(Hsun_awt_image_ImageRepresentation *self,long x,long y,long width,long height,
    struct Hjava_awt_image_ColorModel *model,HArrayOfInt *pixels,long offset,long scan,int bytes)
{
    AwtImage *data;
    XImage *row;
    GC gc;
    long dx,dy,sx,sy,first,last,top,bottom,srcW,srcH,value;
    long result=0;
    if (!self || !model || !pixels) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return -1;
    }
    srcW=unhand(self)->srcW; srcH=unhand(self)->srcH;
    if (x<0 || y<0 || width<0 || height<0 || scan<0 || offset<0 ||
        x>srcW || width>srcW-x || y>srcH || height>srcH-y) goto bounds;
    if (!width || !height) return 0;
    if (offset>obj_length(pixels) || width>obj_length(pixels)-offset ||
        (scan && (obj_length(pixels)-offset-width)/scan<height-1)) goto bounds;
    AWT_LOCK(); data=awt118_image(self,NULL);
    if (!data) { AWT_UNLOCK(); SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return -1; }
    data->references++;
    if (!data->coverage && (size_t)data->width<=SIZE_MAX/(size_t)data->height)
        data->coverage=(unsigned char *)calloc(data->width,data->height);
    if ((data->hints&4) && !data->rows) data->rows=(unsigned char *)calloc(data->height,1);
    if (!data->coverage || ((data->hints&4) && !data->rows)) {
        awt118_release_image(data); AWT_UNLOCK();
        SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return -1;
    }
    row=XCreateImage(awt_display,awt_visInfo.visual,awt_visInfo.depth,ZPixmap,0,NULL,data->width,1,32,0);
    if (row) row->data=(char *)calloc(1,row->bytes_per_line);
    if (!row || !row->data) {
        if (row) XDestroyImage(row);
        awt118_release_image(data); AWT_UNLOCK();
        SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return -1;
    }
    gc=XCreateGC(awt_display,data->pixmap,0,NULL);
    first=data->width; last=0; top=data->height; bottom=0;
    AWT_UNLOCK();
    for (dy=0;dy<data->height && !data->retired && !exceptionOccurred(EE());dy++) {
        sy=((2LL*dy+1)*srcH)/(2LL*data->height);
        if (sy<y || sy>=y+height) continue;
        for (dx=0;dx<data->width;dx++) {
            sx=((2LL*dx+1)*srcW)/(2LL*data->width);
            if (sx<x || sx>=x+width) continue;
            value=bytes ? (unsigned char)unhand((HArrayOfByte *)pixels)->body[offset+(sy-y)*scan+sx-x] :
                unhand(pixels)->body[offset+(sy-y)*scan+sx-x];
            value=execute_java_dynamic_method(EE(),(HObject *)model,"getRGB","(I)I",value);
            if (data->retired || exceptionOccurred(EE())) break;
            /* Release 3b6d0 and 36630: transposed ordered alpha threshold. */
            {
                static const unsigned char dither[8][8]={
                    {0,128,32,160,8,136,40,168},{192,64,224,96,200,72,232,104},
                    {48,176,16,144,56,184,24,152},{240,112,208,80,248,120,216,88},
                    {12,140,44,172,4,132,36,164},{204,76,236,108,196,68,228,100},
                    {60,188,28,156,52,180,20,148},{252,124,220,92,244,116,212,84}
                };
                data->coverage[dy*data->width+dx]=((unsigned long)value>>24)+dither[dx&7][dy&7]>=255 ? 2 : 1;
            }
            XPutPixel(row,dx,0,awt118_pixel(value));
            if (dx<first) first=dx;
            if (dx+1>last) last=dx+1;
        }
        if (data->retired || exceptionOccurred(EE())) break;
        if (last>first) {
            AWT_LOCK(); XPutImage(awt_display,data->pixmap,gc,row,first,0,first,dy,last-first,1);
            if (dy<top) top=dy;
            bottom=dy+1;
            if (data->rows) {
                long previous;
                for (previous=dy-1;previous>=0 && !data->rows[previous];previous--) {
                    XCopyArea(awt_display,data->pixmap,data->pixmap,gc,first,dy,last-first,1,first,previous);
                    memcpy(data->coverage+previous*data->width+first,data->coverage+dy*data->width+first,last-first);
                    if (previous<top) top=previous;
                }
                data->rows[dy]=1;
            }
            AWT_UNLOCK();
        }
    }
    AWT_LOCK();
    if (!data->retired && !exceptionOccurred(EE()) && bottom>top) {
        Hjava_awt_Rectangle *bits=unhand(self)->newbits;
        if (bits) {
            unhand(bits)->x=first; unhand(bits)->y=top;
            unhand(bits)->width=last-first; unhand(bits)->height=bottom-top;
        }
        result=1;
    }
    XFreeGC(awt_display,gc); XDestroyImage(row); awt118_release_image(data); AWT_UNLOCK();
    return result;
bounds:
    SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return -1;
}
long sun_awt_image_ImageRepresentation_setIntPixels(Hsun_awt_image_ImageRepresentation *self,long x,long y,long width,long height,
    struct Hjava_awt_image_ColorModel *model,HArrayOfInt *pixels,long offset,long scan)
{ return set_pixels(self,x,y,width,height,model,pixels,offset,scan,0); }
long sun_awt_image_ImageRepresentation_setBytePixels(Hsun_awt_image_ImageRepresentation *self,long x,long y,long width,long height,
    struct Hjava_awt_image_ColorModel *model,HArrayOfByte *pixels,long offset,long scan)
{ return set_pixels(self,x,y,width,height,model,(HArrayOfInt *)pixels,offset,scan,1); }
long sun_awt_image_ImageRepresentation_finish(Hsun_awt_image_ImageRepresentation *self,long force)
{
    AwtImage *data;
    long retry=0;
    if (!self) { SignalError(EE(),"java/lang/NullPointerException",NULL); return 0; }
    AWT_LOCK(); data=awt118_image(self,NULL);
    if (data) {
        retry=!force && awt_visInfo.depth<=8 && !(data->hints&2);
        free(data->rows); data->rows=NULL; data->hints=0;
    }
    AWT_UNLOCK(); return retry;
}
