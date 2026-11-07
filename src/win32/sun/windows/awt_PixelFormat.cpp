/* Display formats and palette conversion recovered from winawt_g10018F52,
 * 100251F0,100279D0,100285F0,10029590,1002A500 and1002ABD0. */
#include "awt_PixelFormat.h"
#include "awt_PaletteData.h"
#include <limits.h>
static int bounded(int value) { return value<0?0:value>255?255:value; }
static unsigned offsetOf(DWORD mask) { unsigned n=0; if(mask)while(!(mask&1)){mask>>=1;n++;}return n; }
static DWORD encodeChannel(unsigned value,DWORD mask)
{
    if(!mask)return 0;unsigned offset=offsetOf(mask);DWORD max=mask>>offset;
    unsigned discard=0;while(max<128){max<<=1;discard++;}
    return ((value>>discard)<<offset)&mask;
}
static DWORD decodeChannel(DWORD pixel,DWORD mask)
{
    if(!mask)return 0;unsigned offset=offsetOf(mask);DWORD max=mask>>offset;
    return ((pixel&mask)>>offset)*255/max;
}
DWORD AwtPixelFormat::Encode(DWORD rgb) const
{
    if(depth==8)return awtPaletteIndex((rgb>>16)&255,(rgb>>8)&255,rgb&255);
    if(depth==16)return encodeChannel((rgb>>16)&255,redMask)|encodeChannel((rgb>>8)&255,greenMask)|encodeChannel(rgb&255,blueMask);
    return rgb&0xffffff;
}
DWORD AwtPixelFormat::Decode(DWORD pixel) const
{
    if(depth==8)return awtPaletteRGB(pixel&255);
    if(depth==16)return (decodeChannel(pixel,redMask)<<16)|(decodeChannel(pixel,greenMask)<<8)|decodeChannel(pixel,blueMask);
    return pixel&0xffffff;
}
DWORD AwtPixelFormat::Read(const BYTE *row,int x) const
{
    DWORD pixel;
    if(depth==8)pixel=row[x];
    else if(depth==16)pixel=((const WORD *)row)[x];
    else if(depth==24){const BYTE *p=row+3*x;pixel=p[0]|(p[1]<<8)|(p[2]<<16);}
    else pixel=((const DWORD *)row)[x];
    return Decode(pixel);
}
void AwtPixelFormat::Write(BYTE *row,int x,DWORD pixel) const
{
    if(depth==8)row[x]=(BYTE)pixel;
    else if(depth==16)((WORD *)row)[x]=(WORD)pixel;
    else if(depth==24){BYTE *p=row+3*x;p[0]=(BYTE)pixel;p[1]=(BYTE)(pixel>>8);p[2]=(BYTE)(pixel>>16);}
    else ((DWORD *)row)[x]=pixel;
}
DWORD awtPaletteRGB(unsigned index)
{ return ((DWORD)awtPaletteRed[index]<<16)|((DWORD)awtPaletteGreen[index]<<8)|awtPaletteBlue[index]; }
BYTE awtPaletteIndex(int red,int green,int blue)
{ return awtPaletteCube[((bounded(red)&0xf8)<<7)|((bounded(green)&0xf8)<<2)|(bounded(blue)>>3)]; }
static int bayer(int x,int y)
{
    int result=0;
    for(int bit=0;bit<3;bit++){
        BOOL a=(x&(1<<bit))!=0,b=(y&(1<<bit))!=0;
        result|=(a?(b?1:3):(b?2:0))<<(4-2*bit);
    }
    return result;
}
BYTE awtOrderedPaletteIndex(int red,int green,int blue,int x,int y)
{
    x&=7;y&=7;
    return awtPaletteIndex(red+40*bayer(x,y)/64-20,
        green+40*bayer(x,7-y)/64-20,blue+40*bayer(7-x,y)/64-20);
}
BYTE awtDiffusePaletteIndex(int red,int green,int blue,int *errors,int carry[3])
{
    int value[3]={bounded(red+errors[3]),bounded(green+errors[4]),bounded(blue+errors[5])};
    BYTE index=awtPaletteIndex(value[0],value[1],value[2]);
    int color[3]={awtPaletteRed[index],awtPaletteGreen[index],awtPaletteBlue[index]};
    for(int c=0;c<3;c++){
        int error=value[c]-color[c],e3=(3*error)>>4,e5=(5*error)>>4,e7=(7*error)>>4;
        errors[c+3]=carry[c];errors[c]+=e3;errors[c+3]+=e5;errors[c+6]+=e7;
        carry[c]=error-e3-e5-e7;
    }
    return index;
}
void awtPaletteEntries(RGBQUAD *colors)
{
    for(int i=0;i<256;i++){
        colors[i].rgbRed=awtPaletteRed[i];colors[i].rgbGreen=awtPaletteGreen[i];
        colors[i].rgbBlue=awtPaletteBlue[i];colors[i].rgbReserved=0;
    }
}
void awtDisplayPixelFormat(AwtPixelFormat *format)
{
    HDC dc=CreateDCA("DISPLAY",NULL,NULL,NULL);if(!dc)return;
    int depth=GetDeviceCaps(dc,BITSPIXEL);
    if(depth>=24)format->depth=24;
    else if(depth>8){
        format->depth=16;format->redMask=0x7c00;format->greenMask=0x3e0;format->blueMask=0x1f;
        struct {BITMAPINFOHEADER header;DWORD colors[256];} info;ZeroMemory(&info,sizeof(info));info.header.biSize=sizeof(BITMAPINFOHEADER);
        HBITMAP bitmap=CreateCompatibleBitmap(dc,1,1);BYTE pixel[4];
        if(bitmap){
            if(GetDIBits(dc,bitmap,0,1,NULL,(BITMAPINFO *)&info,DIB_RGB_COLORS) &&
                GetDIBits(dc,bitmap,0,1,pixel,(BITMAPINFO *)&info,DIB_RGB_COLORS) && info.header.biCompression==BI_BITFIELDS){
                format->redMask=info.colors[0];format->greenMask=info.colors[1];format->blueMask=info.colors[2];
            }
            DeleteObject(bitmap);
        }
    }else if(depth>=4)format->depth=8;
    DeleteDC(dc);
}
HPALETTE awtCreateImagePalette()
{
    struct {WORD version,count;PALETTEENTRY colors[256];} palette;palette.version=0x300;palette.count=256;
    for(int i=0;i<256;i++){
        palette.colors[i].peRed=awtPaletteRed[i];palette.colors[i].peGreen=awtPaletteGreen[i];palette.colors[i].peBlue=awtPaletteBlue[i];
        palette.colors[i].peFlags=(i<10 || i>=246)?0:PC_NOCOLLAPSE;
    }
    return CreatePalette((LOGPALETTE *)&palette);
}
HBITMAP awtCreateImageDIB(HDC dc,const AwtPixelFormat &format,int width,int height,void **bits,int *stride)
{
    *bits=NULL;*stride=0;
    unsigned __int64 row=((unsigned __int64)width*(format.depth/8)+3)&~(unsigned __int64)3;
    if(width<=0 || height<=0 || row>INT_MAX || row*height>INT_MAX)return NULL;
    struct {BITMAPINFOHEADER header;RGBQUAD colors[256];} info;ZeroMemory(&info,sizeof(info));
    info.header.biSize=sizeof(BITMAPINFOHEADER);info.header.biWidth=width;info.header.biHeight=-height;
    info.header.biPlanes=1;info.header.biBitCount=(WORD)format.depth;
    if(format.depth==8){info.header.biClrUsed=info.header.biClrImportant=256;awtPaletteEntries(info.colors);}
    else if(format.depth==16){info.header.biCompression=BI_BITFIELDS;DWORD *masks=(DWORD *)info.colors;masks[0]=format.redMask;masks[1]=format.greenMask;masks[2]=format.blueMask;}
    HBITMAP bitmap=CreateDIBSection(dc,(BITMAPINFO *)&info,DIB_RGB_COLORS,bits,NULL,0);
    if(bitmap)*stride=(int)row;return bitmap;
}
