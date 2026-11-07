/* Solaris libawt.so 32ecc/33620/36280/36314: offscreen storage and delivery. */
#include "awt_Image118.h"
#include "sun_awt_image_OffScreenImageSource.h"
#include <stdlib.h>

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
