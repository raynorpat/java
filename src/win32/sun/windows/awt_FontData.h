#ifndef AWT_FONT_DATA_H
#define AWT_FONT_DATA_H
#include <windows.h>
#include <wchar.h>
/* 1000ED7B/1000F163 test format-4 segment membership, without consulting
 * glyphIdArray/idDelta. Both GDI font data and EUDC SFNT files use this path. */
static unsigned awtBE16(const BYTE *p) { return ((unsigned)p[0]<<8)|p[1]; }
static DWORD awtBE32(const BYTE *p) { return ((DWORD)p[0]<<24)|((DWORD)p[1]<<16)|((DWORD)p[2]<<8)|p[3]; }
static BOOL awtCmapContains(const BYTE *data,DWORD size,WCHAR ch)
{
    if (!data || size<4 || ch==0xffff) return FALSE;
    unsigned count=awtBE16(data+2);
    if (count>(size-4)/8) return FALSE;
    for (unsigned i=0;i<count;i++) {
        const BYTE *record=data+4+8*i;
        if (awtBE16(record)!=3 || awtBE16(record+2)!=1) continue;
        DWORD offset=awtBE32(record+4);
        if (offset>size || size-offset<16) return FALSE;
        const BYTE *table=data+offset;
        unsigned length=awtBE16(table+2),segments=awtBE16(table+6)/2;
        if (awtBE16(table)!=4 || length<16 || length>size-offset || segments>(length-16)/4) return FALSE;
        for (unsigned j=0;j<segments;j++) {
            unsigned end=awtBE16(table+14+2*j),start=awtBE16(table+16+2*segments+2*j);
            if (ch>=start && ch<=end) return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}
static BOOL awtSFNTContains(const BYTE *data,DWORD size,WCHAR ch)
{
    if (!data || size<12) return FALSE;
    unsigned count=awtBE16(data+4);
    if (count>(size-12)/16) return FALSE;
    for (unsigned i=0;i<count;i++) {
        const BYTE *table=data+12+16*i;
        if (awtBE32(table)!=0x636d6170) continue;
        DWORD offset=awtBE32(table+8),length=awtBE32(table+12);
        if (offset>size || length>size-offset) return FALSE;
        return awtCmapContains(data+offset,length,ch);
    }
    return FALSE;
}
/* An explicitly configured face wins even if it is an empty/unsupported value;
 * the default is consulted only if the face value is absent (1000FDC8). */
static const WCHAR *awtEudcChoice(BOOL faceFound,const WCHAR *face,BOOL defaultFound,const WCHAR *fallback)
{
    const WCHAR *path=faceFound?face:defaultFound?fallback:NULL;
    return path && *path && wcscmp(path,L"userfont.fon") ? path : NULL;
}
static BOOL awtEudcFileContains(const WCHAR *path,WCHAR ch)
{
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file==INVALID_HANDLE_VALUE) return FALSE;
    LARGE_INTEGER size; BOOL result=FALSE;
    if (GetFileSizeEx(file,&size) && size.QuadPart>=12 && size.QuadPart<=0xffffffff) {
        HANDLE mapping=CreateFileMappingW(file,NULL,PAGE_READONLY,0,0,NULL);
        if (mapping) {
            const BYTE *data=(const BYTE *)MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);
            if (data) { result=awtSFNTContains(data,(DWORD)size.QuadPart,ch); UnmapViewOfFile(data); }
            CloseHandle(mapping);
        }
    }
    CloseHandle(file); return result;
}
#endif
