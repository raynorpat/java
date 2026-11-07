#include "../../src/win32/sun/windows/awt_FontData.h"
#include "../../src/win32/sun/windows/awt_PixelFormat.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(value) do { if (!(value)) { printf("line %d failed: %s\n",__LINE__,#value); failures++; } } while (0)
static void be16(BYTE *p,unsigned value) { p[0]=(BYTE)(value>>8); p[1]=(BYTE)value; }
static void be32(BYTE *p,DWORD value) { p[0]=(BYTE)(value>>24);p[1]=(BYTE)(value>>16);p[2]=(BYTE)(value>>8);p[3]=(BYTE)value; }
static void fontDataTests()
{
    BYTE cmap[44]={0},sfnt[72]={0};
    be16(cmap+2,1);be16(cmap+4,3);be16(cmap+6,1);be32(cmap+8,12);
    be16(cmap+12,4);be16(cmap+14,32);be16(cmap+18,4);
    be16(cmap+26,0xe002);be16(cmap+28,0xffff);be16(cmap+32,0xe000);be16(cmap+34,0xffff);
    CHECK(awtCmapContains(cmap,sizeof(cmap),0xe000));CHECK(awtCmapContains(cmap,sizeof(cmap),0xe002));
    CHECK(!awtCmapContains(cmap,sizeof(cmap),0xdfff));CHECK(!awtCmapContains(cmap,sizeof(cmap),0xe003));CHECK(!awtCmapContains(cmap,sizeof(cmap),0xffff));
    /* Glyph arrays deliberately zero: the binary tests coverage, not glyph ids. */
    be32(sfnt,0x10000);be16(sfnt+4,1);be32(sfnt+12,0x636d6170);be32(sfnt+20,28);be32(sfnt+24,sizeof(cmap));memcpy(sfnt+28,cmap,sizeof(cmap));
    CHECK(awtSFNTContains(sfnt,sizeof(sfnt),0xe001));
    for(DWORD length=0;length<sizeof(sfnt);length++) CHECK(!awtSFNTContains(sfnt,length,0xe001));
    be32(sfnt+20,0xfffffff0);CHECK(!awtSFNTContains(sfnt,sizeof(sfnt),0xe001));be32(sfnt+20,28);
    be16(cmap+6,0);CHECK(!awtCmapContains(cmap,sizeof(cmap),0xe001));be16(cmap+6,1);
    CHECK(!wcscmp(awtEudcChoice(TRUE,L"face.ttf",TRUE,L"default.ttf"),L"face.ttf"));
    CHECK(!wcscmp(awtEudcChoice(FALSE,NULL,TRUE,L"default.ttf"),L"default.ttf"));
    CHECK(!awtEudcChoice(TRUE,L"",TRUE,L"default.ttf"));CHECK(!awtEudcChoice(TRUE,L"userfont.fon",TRUE,L"default.ttf"));CHECK(!awtEudcChoice(FALSE,NULL,FALSE,NULL));
    WCHAR directory[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,directory);GetTempFileNameW(directory,L"eud",0,path);
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);DWORD written=0;
    CHECK(file!=INVALID_HANDLE_VALUE);CHECK(WriteFile(file,sfnt,sizeof(sfnt),&written,NULL));CloseHandle(file);
    CHECK(awtEudcFileContains(path,0xe001));CHECK(!awtEudcFileContains(path,0xffff));CHECK(DeleteFileW(path));
}
static void pixelFormatTests()
{
    AwtPixelFormat f;f.depth=16;f.redMask=0xf800;f.greenMask=0x7e0;f.blueMask=0x1f;
    CHECK(f.Encode(0xffffff)==0xffff);CHECK(f.Encode(0xff0000)==0xf800);CHECK(f.Encode(0x00ff00)==0x7e0);CHECK(f.Encode(0x0000ff)==0x1f);
    CHECK(f.Encode(0x123456)==0x11aa);CHECK(f.Decode(0x11aa)==0x103452);CHECK(f.Decode(0xffff)==0xffffff);
    f.redMask=0x7c00;f.greenMask=0x3e0;CHECK(f.Encode(0xffffff)==0x7fff);CHECK(f.Decode(0x7fff)==0xffffff);
    BYTE row[16]={0};f.Write(row,1,f.Encode(0xff0000));CHECK(row[2]==0 && row[3]==0x7c);CHECK(f.Read(row,1)==0xff0000);
    f.depth=24;f.Write(row,2,0x123456);CHECK(row[6]==0x56 && row[7]==0x34 && row[8]==0x12);CHECK(f.Read(row,2)==0x123456);
    f.depth=32;f.Write(row,2,0xabcdef);CHECK(f.Read(row,2)==0xabcdef);
    f.depth=8;CHECK(awtPaletteRGB(0)==0);CHECK(awtPaletteRGB(255)==0xffffff);CHECK(awtPaletteIndex(0,0,0)==0);CHECK(awtPaletteIndex(255,255,255)==255);
    DWORD cubeHash=2166136261u;
    for(int r=0;r<256;r+=8)for(int g=0;g<256;g+=8)for(int b=0;b<256;b+=8)cubeHash=(cubeHash^awtPaletteIndex(r,g,b))*16777619u;
    CHECK(cubeHash==0xe51377db); /* Independently extracted from 1003BF68. */
    RGBQUAD colors[256];awtPaletteEntries(colors);DWORD hash[3]={2166136261u,2166136261u,2166136261u};
    for(int i=0;i<256;i++){hash[0]=(hash[0]^colors[i].rgbRed)*16777619u;hash[1]=(hash[1]^colors[i].rgbGreen)*16777619u;hash[2]=(hash[2]^colors[i].rgbBlue)*16777619u;}
    CHECK(hash[0]==0x58edb65f && hash[1]==0xe468fda0 && hash[2]==0x4d87cd27);
    for(int depth=8;depth<=32;depth+=8){f.depth=depth;f.redMask=0xf800;f.greenMask=0x7e0;f.blueMask=0x1f;void *bits=NULL;int stride=0;
        HDC dc=CreateCompatibleDC(NULL);HBITMAP bitmap=awtCreateImageDIB(dc,f,3,2,&bits,&stride);CHECK(bitmap && bits);CHECK(stride==((3*(depth/8)+3)&~3));
        if(bitmap){HGDIOBJ old=SelectObject(dc,bitmap);f.Write((BYTE *)bits+stride,2,f.Encode(0x123456));CHECK(f.Read((BYTE *)bits+stride,2)==f.Decode(f.Encode(0x123456)));SelectObject(dc,old);DeleteObject(bitmap);}DeleteDC(dc);
    }
    HPALETTE palette=awtCreateImagePalette();PALETTEENTRY entries[256];CHECK(palette && GetPaletteEntries(palette,0,256,entries)==256);CHECK(entries[0].peFlags==0 && entries[10].peFlags==PC_NOCOLLAPSE && entries[255].peFlags==0);DeleteObject(palette);
}
int main() { fontDataTests();pixelFormatTests();if(failures)return 1;puts("font cmap, EUDC, RGB555/565, palette cube and DIB fixtures passed");return 0; }
