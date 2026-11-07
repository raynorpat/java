/* Image ownership and completion recovered from winawt_g 10019EDA..1001A8C7.
 * Java color-model/consumer calls run on the calling Java thread. Only native
 * snapshots and bitmap mutations are dispatched to the toolkit thread. */
#include "awt_Image.h"
extern "C" {
#include "sun_awt_image_OffScreenImageSource.h"
#include "java_awt_Color.h"
#include "java_awt_Rectangle.h"
}
#include <stdlib.h>
#include <string.h>
#include <limits.h>

AwtSurface *awtImageSurface(Hsun_awt_image_ImageRepresentation *image,BOOL create)
{
    if (!image) return NULL;
    AwtSurface *s=(AwtSurface *)unhand(image)->pData;
    if (s || !create) return s;
    int w=unhand(image)->width,h=unhand(image)->height;
    if (w <= 0 || h <= 0 || (size_t)w > INT_MAX/4/(size_t)h) return NULL;
    s=new AwtSurface(NULL); s->width=w; s->height=h;
    BITMAPINFO info; ZeroMemory(&info,sizeof(info));
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=w;
    info.bmiHeader.biHeight=-h; info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    s->dc=CreateCompatibleDC(NULL); s->ownsDC=TRUE;
    s->bitmap=CreateDIBSection(s->dc,&info,DIB_RGB_COLORS,(void **)&s->bits,NULL,0);
    if (!s->dc || !s->bitmap) { delete s; return NULL; }
    s->oldBitmap=SelectObject(s->dc,s->bitmap);
    ZeroMemory(s->bits,(size_t)w*h*4);
    s->coverage=(BYTE *)calloc((size_t)w*h,1); s->remaining=w*h;
    if (!s->coverage) { delete s; return NULL; }
    if (unhand(image)->hints&4) { s->rows=(BYTE *)calloc(h,1); if (!s->rows) { delete s; return NULL; } }
    unhand(image)->pData=(long)s; return s;
}
enum ImageOperation { INIT, PIXELS, FINISH, DISPOSE_IMAGE, SNAPSHOT, DRAW, STRETCH };
struct ImageRequest {
    Hsun_awt_image_ImageRepresentation *peer;
    ImageOperation operation;
    Hsun_awt_windows_WGraphics *graphics;
    long v[8];
    DWORD *pixels;
    BOOL result;
    BOOL background;
    COLORREF color;
};
static void imageRequest(void *argument)
{
    ImageRequest *r=(ImageRequest *)argument;
    AwtSurface *s=awtImageSurface(r->peer,r->operation == INIT || r->operation == PIXELS || r->operation == FINISH);
    if (r->operation == DISPOSE_IMAGE) {
        unhand(r->peer)->pData=0; if (s) s->Release(); r->result=TRUE; return;
    }
    if (!s) return;
    switch (r->operation) {
    case INIT:
        free(s->coverage); s->coverage=NULL; s->remaining=0;
        free(s->rows); s->rows=NULL;
        s->background=r->color;
        for (int i=0;i<s->width*s->height;i++) s->bits[i]=RGB(GetBValue(r->color),GetGValue(r->color),GetRValue(r->color));
        unhand(r->peer)->availinfo=43; r->result=TRUE; break;
    case PIXELS: {
        int sw=unhand(r->peer)->srcW,sh=unhand(r->peer)->srcH;
        if (sw <= 0) sw=s->width; if (sh <= 0) sh=s->height;
        GdiFlush(); int firstY=s->height,lastY=0,firstX=s->width,lastX=0;
        for (int y=0;y<s->height;y++) {
            int sy=(int)(((__int64)2*y+1)*sh/(2*s->height));
            if (sy < r->v[1] || sy-r->v[1] >= r->v[3]) continue;
            firstY=min(firstY,y); lastY=y+1;
            for (int x=0;x<s->width;x++) {
                int sx=(int)(((__int64)2*x+1)*sw/(2*s->width));
                if (sx < r->v[0] || sx-r->v[0] >= r->v[2]) continue;
                firstX=min(firstX,x); lastX=max(lastX,x+1);
                DWORD pixel=r->pixels[(sy-r->v[1])*r->v[2]+sx-r->v[0]];
                s->bits[y*s->width+x]=pixel&0xffffff;
                if (s->coverage && !s->coverage[y*s->width+x]) {
                    s->coverage[y*s->width+x]=1; --s->remaining;
                }
                /* 100251F0 + 1002B3E0: transposed 8x8 ordered alpha dither. */
                static const BYTE dither[8][8] = {
                    {0,128,32,160,8,136,40,168},{192,64,224,96,200,72,232,104},
                    {48,176,16,144,56,184,24,152},{240,112,208,80,248,120,216,88},
                    {12,140,44,172,4,132,36,164},{204,76,236,108,196,68,228,100},
                    {60,188,28,156,52,180,20,148},{252,124,220,92,244,116,212,84}
                };
                BOOL opaque=(pixel>>24)+dither[x&7][y&7] >= 255;
                if (!opaque && !s->alpha) {
                    s->alpha=(BYTE *)malloc((size_t)s->width*s->height);
                    if (!s->alpha) return;
                    memset(s->alpha,255,(size_t)s->width*s->height);
                }
                if (s->alpha) s->alpha[y*s->width+x]=opaque?255:0;
            }
        }
        /* 10017071 replicates a top-down scanline into preceding missing
         * rows, giving interlaced decoders their coarse-to-fine display. */
        if (s->rows && firstY<lastY && firstX<lastX) {
            for (int y=firstY-1;y>=0 && !s->rows[y];y--) {
                memcpy(s->bits+y*s->width,s->bits+firstY*s->width,s->width*4);
                if (s->alpha) memcpy(s->alpha+y*s->width,s->alpha+firstY*s->width,s->width);
                if (s->coverage) for (int x=0;x<s->width;x++) {
                    if (!s->coverage[y*s->width+x]) { s->coverage[y*s->width+x]=1; --s->remaining; }
                }
            }
            int changed=firstY;
            while (changed>0 && !s->rows[changed-1]) --changed;
            for (int y=firstY;y<lastY;y++) s->rows[y]=1;
            firstY=changed;
        }
        if (s->coverage && !s->remaining) { free(s->coverage); s->coverage=NULL; }
        Hjava_awt_Rectangle *rect=unhand(r->peer)->newbits;
        if (rect) {
            unhand(rect)->x=firstX<s->width?firstX:0;
            unhand(rect)->y=firstY<s->height?firstY:0;
            unhand(rect)->width=max(0,lastX-firstX);
            unhand(rect)->height=max(0,lastY-firstY);
        }
        r->result=TRUE; break;
    }
    case FINISH: r->result=!r->v[0] && !s->finished; s->finished=TRUE; break;
    case SNAPSHOT:
        GdiFlush();
        r->pixels=(DWORD *)malloc((size_t)s->width*s->height*4);
        if (r->pixels) { memcpy(r->pixels,s->bits,(size_t)s->width*s->height*4); r->v[0]=s->width; r->v[1]=s->height; r->result=TRUE; }
        break;
    case DRAW: case STRETCH: {
        if ((unhand(r->peer)->availinfo&11) != 11) break;
        AwtDC lease; if (!awtGraphicsDC(r->graphics,&lease)) break;
        int ox=unhand(r->graphics)->originX,oy=unhand(r->graphics)->originY;
        int dx=r->v[0]+ox,dy=r->v[1]+oy,dw,dh,sx,sy,sw,sh;
        if (r->operation == DRAW) { dw=sw=s->width; dh=sh=s->height; sx=sy=0; }
        else { dw=r->v[2]-r->v[0]; dh=r->v[3]-r->v[1]; sx=r->v[4]; sy=r->v[5]; sw=r->v[6]-sx; sh=r->v[7]-sy; }
        if (!dw || !dh || !sw || !sh) { awtReleaseDC(&lease); break; }
        /* The DIB path adjusts reversed endpoints before calling GDI; the
         * offscreen compatible-DC path does not (10018082 / 10013EE4). */
        if (!unhand(r->peer)->offscreen) {
            if (dw < 0) --dx; if (dh < 0) --dy;
            if (sw < 0) --sx; if (sh < 0) --sy;
        }
        BOOL xormode=awtGraphicsXOR(r->graphics);
        COLORREF xorColor=awtGraphicsXORColor(r->graphics);
        HBRUSH xorBrush=NULL; HGDIOBJ oldBrush=NULL;
        if (xormode) { xorBrush=CreateSolidBrush(xorColor); oldBrush=SelectObject(lease.dc,xorBrush); }
        SetStretchBltMode(lease.dc,COLORONCOLOR);
        if (!s->alpha && !s->coverage) {
            StretchBlt(lease.dc,dx,dy,dw,dh,s->dc,sx,sy,sw,sh,xormode?0x960000:SRCCOPY);
        } else {
            BITMAPINFO info; ZeroMemory(&info,sizeof(info));
            info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=s->width;
            info.bmiHeader.biHeight=-s->height; info.bmiHeader.biPlanes=1;
            info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
            HDC temp=CreateCompatibleDC(lease.dc); DWORD *bits=NULL;
            HBITMAP bitmap=CreateDIBSection(temp,&info,DIB_RGB_COLORS,(void **)&bits,NULL,0);
            if (bitmap) {
                HGDIOBJ old=SelectObject(temp,bitmap); GdiFlush();
                DWORD bg=RGB(GetBValue(r->color),GetGValue(r->color),GetRValue(r->color));
                DWORD xorPixel=RGB(GetBValue(xorColor),GetGValue(xorColor),GetRValue(xorColor));
                if (xormode || (r->background && !s->coverage)) {
                    for (int i=0;i<s->width*s->height;i++) bits[i]=(s->coverage && !s->coverage[i])?xorPixel:(!s->alpha || s->alpha[i])?s->bits[i]:(xormode?xorPixel:bg);
                    StretchBlt(lease.dc,dx,dy,dw,dh,temp,sx,sy,sw,sh,xormode?0x960000:SRCCOPY);
                } else {
                    for (int i=0;i<s->width*s->height;i++) bits[i]=(!s->coverage || s->coverage[i]) && (r->background || !s->alpha || s->alpha[i])?0:0xffffff;
                    StretchBlt(lease.dc,dx,dy,dw,dh,temp,sx,sy,sw,sh,SRCAND); GdiFlush();
                    for (int i=0;i<s->width*s->height;i++) bits[i]=(s->coverage && !s->coverage[i])?0:(!s->alpha || s->alpha[i])?s->bits[i]:r->background?bg:0;
                    StretchBlt(lease.dc,dx,dy,dw,dh,temp,sx,sy,sw,sh,SRCPAINT);
                }
                GdiFlush(); SelectObject(temp,old); DeleteObject(bitmap);
            }
            DeleteDC(temp);
        }
        if (xorBrush) { SelectObject(lease.dc,oldBrush); DeleteObject(xorBrush); }
        awtReleaseDC(&lease); r->result=TRUE; break;
    }
    }
}
static void invokeImage(ImageRequest *r)
{ AwtToolkit::GetInstance().Invoke(imageRequest,r); }
#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_image_ImageRepresentation_##name##_stub(stack_item *sp,struct execenv *ee)
#define IMAGE ((Hsun_awt_image_ImageRepresentation *)sp[0].p)
STUB(offscreenInit)
{
    Hjava_awt_Color *color=(Hjava_awt_Color *)sp[1].p;
    if (!IMAGE || !color) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    if (unhand(IMAGE)->width <= 0 || unhand(IMAGE)->height <= 0) { SignalError(ee,"java/lang/IllegalArgumentException",NULL); return sp; }
    long rgb=unhand(color)->value;
    ImageRequest r={IMAGE,INIT,NULL,{0},NULL,FALSE,FALSE,RGB((rgb>>16)&255,(rgb>>8)&255,rgb&255)};
    invokeImage(&r); if (!r.result) SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp;
}
static stack_item *setPixels(stack_item *sp,execenv *ee,BOOL bytes)
{
    HObject *model=(HObject *)sp[5].p; HArrayOfInt *array=(HArrayOfInt *)sp[6].p;
    int w=sp[3].i,h=sp[4].i,offset=sp[7].i,scan=sp[8].i;
    ImageRequest r={IMAGE,PIXELS,NULL,{sp[1].i,sp[2].i,w,h},NULL,FALSE};
    if (!IMAGE || !model || !array) SignalError(ee,"java/lang/NullPointerException",NULL);
    else if (w <= 0 || h <= 0) r.result=TRUE; /* reference returns -1 (Java true) */
    else {
        __int64 last=(__int64)offset+(__int64)(h-1)*scan;
        __int64 low=min((__int64)offset,last),high=max((__int64)offset,last)+w;
        if (low < 0 || high > obj_length(array)) SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL);
        else if ((size_t)w > INT_MAX/4/(size_t)h) SignalError(ee,"java/lang/OutOfMemoryError",NULL);
        else {
            r.pixels=(DWORD *)malloc((size_t)w*h*4);
            if (!r.pixels) SignalError(ee,"java/lang/OutOfMemoryError",NULL);
            else {
                for (int y=0;y<h && !exceptionOccurred(ee);y++) for (int x=0;x<w;x++) {
                    long pixel=bytes ? (unsigned char)unhand((HArrayOfByte *)array)->body[offset+y*scan+x] : unhand(array)->body[offset+y*scan+x];
                    r.pixels[y*w+x]=(DWORD)execute_java_dynamic_method(ee,model,"getRGB","(I)I",pixel);
                    if (exceptionOccurred(ee)) break;
                }
                if (!exceptionOccurred(ee)) invokeImage(&r);
                free(r.pixels);
                if (!exceptionOccurred(ee) && !r.result) SignalError(ee,"java/lang/OutOfMemoryError",NULL);
            }
        }
    }
    sp[0].i=r.result; return sp+1;
}
STUB(setBytePixels) { return setPixels(sp,ee,TRUE); }
STUB(setIntPixels) { return setPixels(sp,ee,FALSE); }
STUB(finish) {
    ImageRequest r={IMAGE,FINISH,NULL,{sp[1].i},NULL,FALSE};
    if (!IMAGE) SignalError(ee,"java/lang/NullPointerException",NULL); else invokeImage(&r);
    sp[0].i=r.result; return sp+1;
}
STUB(disposeImage) {
    ImageRequest r={IMAGE,DISPOSE_IMAGE,NULL,{0},NULL,FALSE}; if (IMAGE) invokeImage(&r); return sp;
}
static stack_item *drawImage(stack_item *sp,execenv *ee,BOOL stretch)
{
    Hjava_awt_Color *background=(Hjava_awt_Color *)sp[stretch?10:4].p;
    ImageRequest r={IMAGE,stretch?STRETCH:DRAW,(Hsun_awt_windows_WGraphics *)sp[1].p,{0},NULL,FALSE};
    if (!IMAGE || !r.graphics) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    for (int i=0;i<(stretch?8:2);i++) r.v[i]=sp[i+2].i;
    if (background) { long rgb=unhand(background)->value; r.background=TRUE; r.color=RGB((rgb>>16)&255,(rgb>>8)&255,rgb&255); }
    invokeImage(&r); return sp;
}
STUB(imageDraw) { return drawImage(sp,ee,FALSE); }
STUB(imageStretch) { return drawImage(sp,ee,TRUE); }
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_image_OffScreenImageSource_sendPixels_stub(stack_item *sp,struct execenv *ee)
{
    Hsun_awt_image_OffScreenImageSource *source=(Hsun_awt_image_OffScreenImageSource *)sp[0].p;
    Hsun_awt_image_ImageRepresentation *image=unhand(source)->baseIR;
    if (!image) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    if ((unhand(image)->availinfo&43) != 43) { SignalError(ee,"java/lang/IllegalAccessError",NULL); return sp; }
    ImageRequest r={image,SNAPSHOT,NULL,{0},NULL,FALSE}; invokeImage(&r);
    if (!r.result) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp; }
    HObject *model=(HObject *)execute_java_constructor(ee,"java/awt/image/DirectColorModel",NULL,"(IIIII)",24,0xff,0xff00,0xff0000,0);
    HArrayOfInt *row=(HArrayOfInt *)ArrayAlloc(T_INT,r.v[0]);
    if (!model || !row) SignalError(ee,"java/lang/OutOfMemoryError",NULL);
    else {
        HObject *consumer=(HObject *)unhand(source)->theConsumer;
        if (consumer) execute_java_dynamic_method(ee,consumer,"setColorModel","(Ljava/awt/image/ColorModel;)V",model);
        consumer=(HObject *)unhand(source)->theConsumer;
        if (consumer && !exceptionOccurred(ee)) execute_java_dynamic_method(ee,consumer,"setHints","(I)V",30);
        for (int y=0;y<r.v[1] && !exceptionOccurred(ee);y++) {
            consumer=(HObject *)unhand(source)->theConsumer; if (!consumer) break;
            /* 10018F52 initializes channel shifts only for <=16 bpp. The
             * 32-bpp sendPixels branch at 10018C6B consequently repeats blue
             * into all three channels. Preserve this observable legacy bug. */
            HDC display=GetDC(NULL); int depth=GetDeviceCaps(display,BITSPIXEL); ReleaseDC(NULL,display);
            for (int x=0;x<r.v[0];x++) {
                DWORD rgb=r.pixels[y*r.v[0]+x];
                unhand(row)->body[x]=depth==32 ? (rgb&255)*0x10101 : ((rgb&255)<<16)|(rgb&0xff00)|((rgb>>16)&255);
            }
            execute_java_dynamic_method(ee,consumer,"setPixels","(IIIILjava/awt/image/ColorModel;[III)V",0,y,r.v[0],1,model,row,0,r.v[0]);
        }
    }
    free(r.pixels); return sp;
}



