#include <windows.h>
#include "StubPreamble.h"
__declspec(dllexport) stack_item *Java_GraphicsResources115_gdiObjects_stub(stack_item *sp,struct execenv *ee)
{
    sp[0].i=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS); return sp+1;
}
static HDC graphics_dc;
static HBITMAP graphics_bitmap;
static HGDIOBJ graphics_old;
__declspec(dllexport) stack_item *Java_GraphicsHDC115_create_stub(stack_item *sp,struct execenv *ee)
{
    BITMAPINFO info;
    void *bits;
    HRGN clip;
    ZeroMemory(&info,sizeof(info));
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=16;
    info.bmiHeader.biHeight=-16; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    graphics_dc=CreateCompatibleDC(NULL);
    graphics_bitmap=CreateDIBSection(graphics_dc,&info,DIB_RGB_COLORS,&bits,NULL,0);
    graphics_old=SelectObject(graphics_dc,graphics_bitmap);
    PatBlt(graphics_dc,0,0,16,16,WHITENESS);
    clip=CreateRectRgn(0,0,16,16); SelectClipRgn(graphics_dc,clip); DeleteObject(clip);
    sp[0].i=(long)graphics_dc;
    return sp+1;
}
__declspec(dllexport) stack_item *Java_GraphicsHDC115_pixel_stub(stack_item *sp,struct execenv *ee)
{
    GdiFlush(); sp[0].i=GetPixel(graphics_dc,sp[0].i,sp[1].i); return sp+1;
}
__declspec(dllexport) stack_item *Java_GraphicsHDC115_close_stub(stack_item *sp,struct execenv *ee)
{
    BOOL valid=GetObjectType(graphics_dc)==OBJ_MEMDC;
    if(valid) { SelectObject(graphics_dc,graphics_old); valid=DeleteDC(graphics_dc); }
    DeleteObject(graphics_bitmap); graphics_dc=NULL; graphics_bitmap=NULL;
    sp[0].i=valid; return sp+1;
}

