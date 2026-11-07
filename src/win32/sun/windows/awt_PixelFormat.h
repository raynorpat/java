#ifndef AWT_PIXEL_FORMAT_H
#define AWT_PIXEL_FORMAT_H
#include <windows.h>
struct AwtPixelFormat {
    int depth;
    DWORD redMask,greenMask,blueMask;
    AwtPixelFormat() : depth(32),redMask(0xff0000),greenMask(0xff00),blueMask(0xff) {}
    DWORD Encode(DWORD rgb) const;
    DWORD Decode(DWORD pixel) const;
    DWORD Read(const BYTE *row,int x) const;
    void Write(BYTE *row,int x,DWORD pixel) const;
};
DWORD awtPaletteRGB(unsigned index);
BYTE awtPaletteIndex(int red,int green,int blue);
BYTE awtOrderedPaletteIndex(int red,int green,int blue,int x,int y);
BYTE awtDiffusePaletteIndex(int red,int green,int blue,int *errors,int carry[3]);
void awtPaletteEntries(RGBQUAD *colors);
void awtDisplayPixelFormat(AwtPixelFormat *format);
HPALETTE awtCreateImagePalette();
HBITMAP awtCreateImageDIB(HDC,const AwtPixelFormat &,int width,int height,void **bits,int *stride);
#endif
