/* Font records/metrics recovered from winawt_g1000C63B..1000E864. */
#include "awt_Font.h"
#include "awt_FontData.h"
extern "C" {
#include "sun_awt_PlatformFont.h"
#include "sun_awt_windows_WFontMetrics.h"
#include "sun_awt_windows_WDefaultFontCharset.h"
#include "sun_awt_CharsetString.h"
}
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>

static HArrayOfObject *fontDescriptors(Hjava_awt_Font *font)
{
    Hsun_awt_PlatformFont *peer = (Hsun_awt_PlatformFont *)unhand(font)->peer;
    return peer && unhand(peer)->props ? unhand(peer)->componentFonts : NULL;
}
static Hsun_awt_FontDescriptor *descriptorAt(HArrayOfObject *array,int index)
{ return (Hsun_awt_FontDescriptor *)unhand(array)->body[index]; }
static Hsun_awt_FontDescriptor *selectDescriptor(Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor)
{
    HArrayOfObject *array = fontDescriptors(font);
    if (!array || !obj_length(array)) return NULL;
    for (int i = 0; i < obj_length(array); i++) if (descriptorAt(array,i) == descriptor) return descriptor;
    return descriptorAt(array,0);
}
static void namedFontInfo(const WCHAR *name,long style,long size,LOGFONTW *info)
{
    static const struct { const WCHAR *name; BYTE value; } charsets[] = {
        {L"ANSI_CHARSET",0},{L"DEFAULT_CHARSET",1},{L"SYMBOL_CHARSET",2},{L"WingDings",2},
        {L"SHIFTJIS_CHARSET",128},{L"GB2312_CHARSET",134},{L"HANGEUL_CHARSET",129},
        {L"CHINESEBIG5_CHARSET",136},{L"OEM_CHARSET",255},{L"JOHAB_CHARSET",130},
        {L"HEBREW_CHARSET",177},{L"ARABIC_CHARSET",178},{L"GREEK_CHARSET",161},
        {L"TURKISH_CHARSET",162},{L"VIETNAMESE_CHARSET",163},{L"THAI_CHARSET",222},
        {L"EASTEUROPE_CHARSET",238},{L"RUSSIAN_CHARSET",204},{L"MAC_CHARSET",77},{L"BALTIC_CHARSET",186}
    };
    ZeroMemory(info,sizeof(*info)); info->lfHeight = -size;
    info->lfWeight = style & 1 ? FW_BOLD : FW_NORMAL; info->lfItalic = (BYTE)((style & 2) != 0);
    for (int i = 0; i < sizeof(charsets)/sizeof(charsets[0]); i++)
        if (wcsstr(name,charsets[i].name)) { info->lfCharSet = charsets[i].value; break; }
    int length = (int)wcscspn(name,L","); if (length >= LF_FACESIZE) length = LF_FACESIZE-1;
    wcsncpy(info->lfFaceName,name,length); info->lfFaceName[length] = 0;
}
BOOL awtFontInfo(Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor,LOGFONTW *info)
{
    if (!font) return FALSE;
    descriptor = selectDescriptor(font,descriptor);
    WCHAR *name = awtUnicode(descriptor ? unhand(descriptor)->nativeName : unhand(font)->name);
    if (!name) return FALSE;
    const WCHAR *face = name;
    if (!descriptor) {
        if (!wcscmp(name,L"TimesRoman") || !wcscmp(name,L"Serif")) face = L"Times New Roman";
        else if (!wcscmp(name,L"Courier") || !wcscmp(name,L"Monospaced")) face = L"Courier New";
        else if (!wcscmp(name,L"Dialog") || !wcscmp(name,L"DialogInput")) face = L"MS Sans Serif";
        else if (!wcscmp(name,L"ZapfDingbats")) face = L"WingDings";
        else face = L"Arial";
    }
    namedFontInfo(face,unhand(font)->style,unhand(font)->size,info); free(name); return TRUE;
}
static HFONT createFont(Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor)
{ LOGFONTW info; return awtFontInfo(font,descriptor,&info) ? CreateFontIndirectW(&info) : NULL; }

#define STUB(name) extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WFontMetrics_##name##_stub(stack_item *sp,struct execenv *ee)
STUB(init)
{
    Hsun_awt_windows_WFontMetrics *peer = (Hsun_awt_windows_WFontMetrics *)sp[0].p;
    Hjava_awt_Font *font = unhand(peer)->font;
    if (!font) { SignalError(ee,"java/lang/NullPointerException",NULL); return sp; }
    HArrayOfInt *widths = (HArrayOfInt *)ArrayAlloc(T_INT,256);
    if (!widths) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); return sp; }
    unhand(peer)->widths = widths;
    HDC dc = GetDC(NULL); HFONT nativeFont = createFont(font,NULL);
    if (!dc || !nativeFont) {
        if (dc) ReleaseDC(NULL,dc); if (nativeFont) DeleteObject(nativeFont);
        SignalError(ee,"java/lang/InternalError","cannot create font metrics"); return sp;
    }
    HGDIOBJ old = SelectObject(dc,nativeFont); TEXTMETRICA tm;
    GetTextMetricsA(dc,&tm);
    Classsun_awt_windows_WFontMetrics *m = unhand(peer);
    m->ascent = tm.tmAscent; m->descent = tm.tmDescent; m->leading = tm.tmExternalLeading;
    m->height = m->ascent+m->descent+m->leading;
    m->maxAscent = m->ascent; m->maxDescent = m->descent; m->maxHeight = m->height; m->maxAdvance = tm.tmMaxCharWidth;
    ZeroMemory(unhand(widths)->body,256*sizeof(long));
    GetCharWidthA(dc,tm.tmFirstChar,tm.tmLastChar,(int *)&unhand(widths)->body[tm.tmFirstChar]);
    SelectObject(dc,old); DeleteObject(nativeFont);
    HArrayOfObject *descriptors = fontDescriptors(font);
    for (int i = 1; descriptors && i < obj_length(descriptors); i++) {
        nativeFont = createFont(font,descriptorAt(descriptors,i));
        if (!nativeFont) continue;
        SelectObject(dc,nativeFont); GetTextMetricsA(dc,&tm);
        /* Preserve the binary's primary-ascent comparison for each fallback. */
        m->maxAscent = max(m->ascent,tm.tmAscent); m->maxDescent = max(m->maxDescent,tm.tmDescent);
        m->maxHeight = max(m->maxHeight,tm.tmAscent+tm.tmDescent+tm.tmExternalLeading);
        m->maxAdvance = max(m->maxAdvance,tm.tmMaxCharWidth);
        SelectObject(dc,old); DeleteObject(nativeFont);
    }
    ReleaseDC(NULL,dc); return sp;
}
STUB(bytesWidth)
{
    Hsun_awt_windows_WFontMetrics *peer = (Hsun_awt_windows_WFontMetrics *)sp[0].p;
    HArrayOfByte *bytes = (HArrayOfByte *)sp[1].p; long offset = sp[2].i, count = sp[3].i, width = 0;
    if (!bytes) SignalError(ee,"java/lang/NullPointerException","null bytes");
    else if (offset < 0 || count < 0 || offset > obj_length(bytes) || count > obj_length(bytes)-offset)
        SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL);
    else for (long i = 0; i < count; i++)
        width += unhand(unhand(peer)->widths)->body[(unsigned char)unhand(bytes)->body[offset+i]];
    sp[0].i = width; return sp+1;
}
STUB(needsConversion)
{
    Hjava_awt_Font *font = (Hjava_awt_Font *)sp[0].p;
    Hsun_awt_FontDescriptor *descriptor = (Hsun_awt_FontDescriptor *)sp[1].p;
    BOOL convert = FALSE;
    if (!font || !descriptor) SignalError(ee,"java/lang/NullPointerException",NULL);
    else {
        descriptor = selectDescriptor(font,descriptor);
        WCHAR *name = descriptor ? awtUnicode(unhand(descriptor)->nativeName) : NULL;
        if (name) { convert = wcsstr(name,L"NEED_CONVERTED") != NULL; free(name); }
    }
    sp[0].i = convert; return sp+1;
}
STUB(getMFCharSegmentWidth)
{
    Hjava_awt_Font *font = (Hjava_awt_Font *)sp[1].p;
    Hsun_awt_FontDescriptor *descriptor = (Hsun_awt_FontDescriptor *)sp[2].p;
    BOOL converted = sp[3].i != 0; HArrayOfChar *chars = (HArrayOfChar *)sp[4].p;
    HArrayOfByte *bytes = (HArrayOfByte *)sp[7].p;
    long offset = sp[5].i, count = converted ? sp[8].i : sp[6].i; SIZE size = {0,0};
    if (!font || !descriptor || (converted ? !bytes : !chars)) SignalError(ee,"java/lang/NullPointerException",NULL);
    else if (count < 0 || (converted ? count > obj_length(bytes) :
        (offset < 0 || offset > obj_length(chars) || count > obj_length(chars)-offset)))
        SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL);
    else {
        HDC dc = GetDC(NULL); HFONT nativeFont = createFont(font,descriptor);
        if (dc && nativeFont) {
            HGDIOBJ old = SelectObject(dc,nativeFont);
            if (converted) GetTextExtentPoint32A(dc,(char *)unhand(bytes)->body,count,&size);
            else GetTextExtentPoint32W(dc,(WCHAR *)unhand(chars)->body+offset,count,&size);
            SelectObject(dc,old);
        } else SignalError(ee,"java/lang/InternalError","cannot measure font segment");
        if (nativeFont) DeleteObject(nativeFont); if (dc) ReleaseDC(NULL,dc);
    }
    sp[0].i = size.cx; return sp+1;
}
/* GDI and EUDC file tables share the recovered Unicode format-4 parser. */
static BOOL fontContains(HDC dc,WCHAR ch)
{
    DWORD size=GetFontData(dc,0x70616d63,0,NULL,0);
    if (size==GDI_ERROR || size<4) return FALSE;
    BYTE *data=(BYTE *)malloc(size); if (!data) return FALSE;
    BOOL result=GetFontData(dc,0x70616d63,0,data,size)==size && awtCmapContains(data,size,ch);
    free(data); return result;
}
static BOOL eudcContains(const WCHAR *name,WCHAR ch)
{
    WCHAR face[MAX_PATH],facePath[MAX_PATH]={0},defaultPath[MAX_PATH]={0};
    /* 1000FCEB uses the user-default C locale's codepage suffix. It can
     * differ from GetACP when the user's locale and system locale differ. */
    const char *locale=setlocale(LC_CTYPE,"");
    const char *codepage=locale?strrchr(locale,'.'):NULL;
    if(!codepage)return FALSE;
    char keyName[80];_snprintf(keyName,sizeof(keyName),"EUDC\\%s",codepage+1);keyName[sizeof(keyName)-1]=0;
    HKEY key;
    if(RegOpenKeyExA(HKEY_CURRENT_USER,keyName,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return FALSE;
    int length=(int)wcscspn(name,L","); if (length>=MAX_PATH) length=MAX_PATH-1;
    wcsncpy(face,name,length); face[length]=0;
    DWORD bytes=sizeof(facePath)-sizeof(WCHAR),type=0;
    BOOL faceFound=RegQueryValueExW(key,face,NULL,&type,(BYTE *)facePath,&bytes)==ERROR_SUCCESS;
    BOOL defaultFound=FALSE;
    if (!faceFound) {
        bytes=sizeof(defaultPath)-sizeof(WCHAR);
        defaultFound=RegQueryValueExW(key,L"SystemDefaultEUDCFont",NULL,&type,(BYTE *)defaultPath,&bytes)==ERROR_SUCCESS;
    }
    RegCloseKey(key);
    const WCHAR *path=awtEudcChoice(faceFound,facePath,defaultFound,defaultPath);
    return path && awtEudcFileContains(path,ch);
}
extern "C" __declspec(dllexport) stack_item *Java_sun_awt_windows_WDefaultFontCharset_canConvert_stub(stack_item *sp,struct execenv *ee)
{
    Hsun_awt_windows_WDefaultFontCharset *peer = (Hsun_awt_windows_WDefaultFontCharset *)sp[0].p;
    WCHAR ch = (WCHAR)sp[1].i; BOOL result = FALSE;
    WCHAR *name = awtUnicode(unhand(peer)->fontName);
    if (name) {
        LOGFONTW info; namedFontInfo(name,0,20,&info);
        HFONT font = CreateFontIndirectW(&info); HDC dc = GetDC(NULL);
        if (font && dc) {
            HGDIOBJ old = SelectObject(dc,font);
            result = fontContains(dc,ch);
            SelectObject(dc,old);
        }
        if (font) DeleteObject(font); if (dc) ReleaseDC(NULL,dc);
        if (!result) result=eudcContains(name,ch);
        free(name);
    }
    sp[0].i = result; return sp+1;
}

/* Multi-font control text routine at 1000DCE3. The Java peer chooses descriptor
 * segments and substitutions; each selected native font contributes its extent. */
SIZE awtFontText(HDC dc,Hjava_awt_Font *font,Hjava_lang_String *string,int x,int y,BOOL draw)
{
    SIZE total={0,0};
    if (!string || !javaStringLength(string)) return total;
    ExecEnv *ee=EE();
    HArrayOfObject *segments=NULL;
    if (font && fontDescriptors(font))
        segments=(HArrayOfObject *)execute_java_dynamic_method(ee,(HObject *)unhand(font)->peer,
            "makeMultiCharsetString","(Ljava/lang/String;)[Lsun/awt/CharsetString;",string);
    if (exceptionOccurred(ee)) return total;
    int count=segments?obj_length(segments):0;
    if (!count) {
        WCHAR *text=awtUnicode(string); HFONT nativeFont=font?createFont(font,NULL):NULL;
        HGDIOBJ old=nativeFont?SelectObject(dc,nativeFont):NULL;
        if (text) {
            int length=javaStringLength(string);
            if (draw) TextOutW(dc,x,y,text,length);
            GetTextExtentPoint32W(dc,text,length,&total); free(text);
        }
        if (nativeFont) { SelectObject(dc,old); DeleteObject(nativeFont); }
        return total;
    }
    for (int i=0;i<count && !exceptionOccurred(ee);i++) {
        Hsun_awt_CharsetString *segment=(Hsun_awt_CharsetString *)unhand(segments)->body[i];
        Hsun_awt_FontDescriptor *descriptor=unhand(segment)->fontDescriptor;
        int offset=unhand(segment)->offset,length=unhand(segment)->length;
        WCHAR *name=awtUnicode(unhand(descriptor)->nativeName);
        BOOL converted=name && wcsstr(name,L"NEED_CONVERTED"); free(name);
        HArrayOfByte *bytes=NULL; int byteCount=0;
        if (converted) {
            HObject *converter=(HObject *)unhand(descriptor)->fontCharset;
            int capacity=execute_java_dynamic_method(ee,converter,"getMaxBytesPerChar","()I")*length;
            if (exceptionOccurred(ee)) break;
            bytes=(HArrayOfByte *)ArrayAlloc(T_BYTE,capacity);
            if (!bytes) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); break; }
            byteCount=execute_java_dynamic_method(ee,converter,"convert","([CII[BII)I",
                unhand(segment)->charsetChars,offset,offset+length,bytes,0,capacity);
            if (exceptionOccurred(ee)) break;
        }
        HFONT nativeFont=createFont(font,descriptor);
        if (!nativeFont) { SignalError(ee,"java/lang/OutOfMemoryError",NULL); break; }
        HGDIOBJ old=SelectObject(dc,nativeFont); SIZE size={0,0};
        if (converted) {
            char *text=(char *)unhand(bytes)->body;
            if (draw) TextOutA(dc,x,y,text,byteCount);
            GetTextExtentPoint32A(dc,text,byteCount,&size);
        } else {
            WCHAR *text=(WCHAR *)unhand(unhand(segment)->charsetChars)->body+offset;
            if (draw) TextOutW(dc,x,y,text,length);
            GetTextExtentPoint32W(dc,text,length,&size);
        }
        SelectObject(dc,old); DeleteObject(nativeFont);
        x+=size.cx; total.cx+=size.cx; total.cy=max(total.cy,size.cy);
    }
    return total;
}
