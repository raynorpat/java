/* Solaris libawt_g.so 3f804..40b1c: font selection, ownership and metrics. */
#include "awt118.h"
#include "java_awt_Font.h"
#include "sun_awt_FontDescriptor.h"
#include "sun_awt_CharsetString.h"
#include "sun_awt_motif_MFontPeer.h"
#include "sun_awt_motif_X11FontMetrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    XFontStruct *base;
    int count;
    XFontStruct **fonts;
} AwtFont;

/* Solaris first tries the supplied XLFD, then relaxes foundry and size. */
static XFontStruct *load_font(const char *name, int size)
{
    XFontStruct *font = XLoadQueryFont(awt_display, name);
    char copy[1024], pattern[1024], *fields[14], *p;
    int n=0, delta, pass;
    if (font) return font;
    if (strlen(name) < sizeof(copy) && name[0]=='-') {
        strcpy(copy, name+1);
        p=copy; fields[n++]=p;
        while (*p && n<14) { if (*p=='-') { *p=0; fields[n++]=p+1; } p++; }
        if (n==14) {
            /* Linux core Symbol/Dingbats fonts use Adobe's registry for the
             * same encoding that the Solaris properties call sun-fontspecific. */
            if (!strcmp(fields[12],"sun") && !strcmp(fields[13],"fontspecific")) fields[12]="adobe";
            for (pass=0; pass<2; pass++) {
                snprintf(pattern,sizeof(pattern),"-*-%s-%s-%s-*-*-%s-%d-*-*-*-*-%s-%s",
                    fields[1],fields[2],fields[3],pass ? "0" : "*",size*10,fields[12],fields[13]);
                font=XLoadQueryFont(awt_display,pattern);
                if (font) return font;
            }
            for (delta=0; delta<=4; delta++) for (pass=0; pass<4; pass++) {
                int pixels=size + ((pass&1) ? -delta : delta);
                if (pixels<=0) continue;
                snprintf(pattern,sizeof(pattern),"-*-%s-%s-%s-*-*-%d-*-*-*-*-*-%s-%s",
                    pass<2 ? fields[1] : "*",pass<2 ? fields[2] : "*",pass<2 ? fields[3] : "*",
                    pixels,fields[12],fields[13]);
                font=XLoadQueryFont(awt_display,pattern);
                if (font) return font;
            }
        }
    }
    return XLoadQueryFont(awt_display,"-*-helvetica-*-*-*-*-*-*-12-*-*-*-iso8859-1");
}

static XFontStruct *load_descriptor(Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor)
{
    char name[1024],pattern[1200],*slot;
    javaString2CString(unhand(descriptor)->nativeName,name,sizeof(name));
    slot=strstr(name,"%d");
    if(slot) {
        *slot=0;
        snprintf(pattern,sizeof(pattern),"%s%ld%s",name,unhand(font)->size*10,slot+2);
    } else strcpy(pattern,name);
    return load_font(pattern,unhand(font)->size);
}

static AwtFont *font_data(Hjava_awt_Font *font)
{
    Hsun_awt_motif_MFontPeer *peer;
    AwtFont *data;
    char name[256], pattern[1024];
    const char *family="helvetica", *foundry="adobe", *style;
    int count,i;
    if (!font || !unhand(font)->peer) {
        SignalError(EE(),"java/lang/NullPointerException",NULL); return NULL;
    }
    peer=(Hsun_awt_motif_MFontPeer *)unhand(font)->peer;
    data=(AwtFont *)unhand(peer)->pData;
    if (data) return data;
    data=(AwtFont *)calloc(1,sizeof(*data));
    if (!data) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return NULL; }
    count=unhand(peer)->componentFonts ? obj_length(unhand(peer)->componentFonts) : 0;
    if (count) {
        data->count=count;
        data->fonts=(XFontStruct **)calloc(count,sizeof(XFontStruct *));
        if (!data->fonts) { free(data); SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return NULL; }
        /* Release 21b54 uses the first Latin-1 descriptor for byte metrics.
         * Choosing a separate family fallback disagrees with string segments
         * when the host has additional scalable fonts installed. */
        for(i=0;i<count && !data->base;i++) {
            Hsun_awt_FontDescriptor *descriptor=(Hsun_awt_FontDescriptor *)unhand(unhand(peer)->componentFonts)->body[i];
            Hjava_lang_String *encoding=(Hjava_lang_String *)execute_java_dynamic_method(EE(),
                (HObject *)unhand(descriptor)->fontCharset,"getCharacterEncoding","()Ljava/lang/String;");
            if(exceptionOccurred(EE())) { free(data->fonts); free(data); return NULL; }
            javaString2CString(encoding,name,sizeof(name));
            if(strstr(name,"8859_1")) data->base=data->fonts[i]=load_descriptor(font,descriptor);
        }
    }
    javaString2CString(unhand(font)->name,name,sizeof(name));
    if (!strcmp(name,"Serif") || !strcmp(name,"TimesRoman")) family="times";
    else if (!strcmp(name,"Monospaced") || !strcmp(name,"Courier")) family="courier";
    else if (!strcmp(name,"Dialog")) { family="lucida"; foundry="b&h"; }
    else if (!strcmp(name,"DialogInput")) { family="lucidatypewriter"; foundry="b&h"; }
    style=unhand(font)->style==1 ? "bold-r" : unhand(font)->style==2 ? "medium-i" :
        unhand(font)->style==3 ? "bold-i" : "medium-r";
    snprintf(pattern,sizeof(pattern),"-%s-%s-%s-*-*-%ld-*-*-*-*-*-iso8859-1",foundry,family,style,unhand(font)->size);
    if (!data->base) data->base=load_font(pattern,unhand(font)->size);
    if (!data->base) {
        free(data->fonts); free(data);
        SignalError(EE(),"java/io/FileNotFoundException","X11 font"); return NULL;
    }
    unhand(peer)->pData=(long)data;
    return data;
}

XFontStruct *awt118_font(Hjava_awt_Font *font)
{ AwtFont *data=font_data(font); return data ? data->base : NULL; }

XFontStruct *awt118_segment_font(Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor)
{
    AwtFont *data=font_data(font);
    Hsun_awt_motif_MFontPeer *peer;
    int i;
    if (!data) return NULL;
    peer=(Hsun_awt_motif_MFontPeer *)unhand(font)->peer;
    for (i=0;i<data->count;i++) if ((HObject *)descriptor==unhand(unhand(peer)->componentFonts)->body[i]) {
        if (!data->fonts[i]) {
            data->fonts[i]=load_descriptor(font,descriptor);
        }
        return data->fonts[i];
    }
    return data->base;
}

static char *font_tag(int index,char *buffer)
{
    if (!index) return XmFONTLIST_DEFAULT_TAG;
    sprintf(buffer,"awt118-%d",index); return buffer;
}

/* Solaris debug getFontList 6e02c and makeMultiFontString 6dac0: use matching
 * descriptor tags for converted Java segments and native Motif font entries. */
XmFontList awt118_font_list(Hjava_awt_Font *font)
{
    AwtFont *data=font_data(font);
    Hsun_awt_motif_MFontPeer *peer;
    XmFontList list=NULL;
    XmFontListEntry entry;
    XFontStruct *xfont;
    char tag[32]; int i;
    if (!data) return NULL;
    peer=(Hsun_awt_motif_MFontPeer *)unhand(font)->peer;
    if (!data->count) return XmFontListCreate(data->base,XmFONTLIST_DEFAULT_TAG);
    for (i=0;i<data->count;i++) {
        xfont=awt118_segment_font(font,(Hsun_awt_FontDescriptor *)unhand(unhand(peer)->componentFonts)->body[i]);
        if (xfont) {
            entry=XmFontListEntryCreate(font_tag(i,tag),XmFONT_IS_FONT,xfont);
            list=XmFontListAppendEntry(list,entry); XmFontListEntryFree(&entry);
        }
    }
    return list;
}

XmString awt118_string(Hjava_lang_String *value,HObject *target)
{
    Hjava_awt_Font *font;
    Hsun_awt_motif_MFontPeer *peer;
    HArrayOfObject *segments;
    Hsun_awt_CharsetString *segment;
    Hsun_awt_FontDescriptor *descriptor;
    HArrayOfByte *bytes;
    XmString result=NULL,part,joined;
    int i,j,capacity,length,maxBytes;
    char tag[32];
    if (!value || !javaStringLength(value)) return XmStringCreateLocalized("");
    font=(Hjava_awt_Font *)execute_java_dynamic_method(EE(),target,"getFont","()Ljava/awt/Font;");
    if (exceptionOccurred(EE())) return NULL;
    if (!font) {
        char *text=allocCString(value);
        if (!text) return NULL;
        result=XmStringCreateLocalized(text); free(text); return result;
    }
    peer=(Hsun_awt_motif_MFontPeer *)unhand(font)->peer;
    segments=(HArrayOfObject *)execute_java_dynamic_method(EE(),(HObject *)peer,
        "makeMultiCharsetString","(Ljava/lang/String;)[Lsun/awt/CharsetString;",value);
    if (exceptionOccurred(EE()) || !segments) return NULL;
    for (i=0;i<obj_length(segments);i++) {
        segment=(Hsun_awt_CharsetString *)unhand(segments)->body[i];
        descriptor=unhand(segment)->fontDescriptor;
        maxBytes=execute_java_dynamic_method(EE(),(HObject *)unhand(descriptor)->fontCharset,"getMaxBytesPerChar","()I");
        if (exceptionOccurred(EE())) break;
        if (maxBytes<=0 || unhand(segment)->length>(INT_MAX-1)/maxBytes) {
            SignalError(EE(),"java/lang/OutOfMemoryError",NULL); break;
        }
        capacity=unhand(segment)->length*maxBytes;
        bytes=(HArrayOfByte *)ArrayAlloc(T_BYTE,capacity+1);
        if (!bytes) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); break; }
        execute_java_dynamic_method(EE(),(HObject *)unhand(descriptor)->fontCharset,"reset","()V");
        length=execute_java_dynamic_method(EE(),(HObject *)unhand(descriptor)->fontCharset,
            "convert","([CII[BII)I",unhand(segment)->charsetChars,unhand(segment)->offset,
            unhand(segment)->offset+unhand(segment)->length,bytes,0,capacity);
        if (exceptionOccurred(EE())) break;
        unhand(bytes)->body[length]=0;
        for (j=0;j<obj_length(unhand(peer)->componentFonts);j++)
            if ((HObject *)descriptor==unhand(unhand(peer)->componentFonts)->body[j]) break;
        part=XmStringCreate(unhand(bytes)->body,font_tag(j,tag));
        joined=result ? XmStringConcat(result,part) : XmStringCopy(part);
        if (result) XmStringFree(result);
        XmStringFree(part); result=joined;
    }
    if (exceptionOccurred(EE())) { if (result) XmStringFree(result); return NULL; }
    return result ? result : XmStringCreateLocalized("");
}

void sun_awt_motif_MFontPeer_dispose(Hsun_awt_motif_MFontPeer *self)
{
    AwtFont *data;
    int i;
    AWT_LOCK();
    data=(AwtFont *)unhand(self)->pData;
    if (data) {
        for (i=0;i<data->count;i++) if (data->fonts[i] && data->fonts[i]!=data->base) XFreeFont(awt_display,data->fonts[i]);
        XFreeFont(awt_display,data->base);
        free(data->fonts); free(data); unhand(self)->pData=0;
    }
    AWT_UNLOCK();
}

void sun_awt_motif_X11FontMetrics_init(Hsun_awt_motif_X11FontMetrics *self)
{
    XFontStruct *font;
    Classsun_awt_motif_X11FontMetrics *metrics;
    int i;
    AWT_LOCK();
    font=awt118_font(unhand(self)->font);
    if (font) {
        metrics=unhand(self);
        metrics->ascent=font->ascent; metrics->descent=font->descent; metrics->leading=1;
        metrics->maxAscent=font->max_bounds.ascent; metrics->maxDescent=font->max_bounds.descent;
        metrics->maxAdvance=font->max_bounds.width;
        metrics->height=metrics->ascent+metrics->descent+1;
        metrics->maxHeight=metrics->maxAscent+metrics->maxDescent+1;
        {
            HArrayOfInt *widths=(HArrayOfInt *)ArrayAlloc(T_INT,256);
            metrics=unhand(self);
            metrics->widths=widths;
        }
        if (!metrics->widths) SignalError(EE(),"java/lang/OutOfMemoryError",NULL);
        else for (i=0;i<256;i++) {
            unhand(metrics->widths)->body[i]=i<font->min_char_or_byte2 || i>font->max_char_or_byte2 ? 0 :
                font->per_char ? font->per_char[i-font->min_char_or_byte2].width : font->max_bounds.width;
        }
    }
    AWT_UNLOCK();
}

long sun_awt_motif_X11FontMetrics_bytesWidth(Hsun_awt_motif_X11FontMetrics *self,HArrayOfByte *bytes,long offset,long length)
{
    long width=0, i;
    if (!bytes) { SignalError(EE(),"java/lang/NullPointerException",NULL); return 0; }
    if (offset<0 || length<0 || offset>obj_length(bytes) || length>obj_length(bytes)-offset) {
        SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return 0;
    }
    for (i=0;i<length;i++) width+=unhand(self)->widths ?
        unhand(unhand(self)->widths)->body[(unsigned char)unhand(bytes)->body[offset+i]] : unhand(self)->maxAdvance;
    return width;
}

long sun_awt_motif_X11FontMetrics_getMFCharSegmentWidth(Hsun_awt_motif_X11FontMetrics *self,
    Hjava_awt_Font *font,Hsun_awt_FontDescriptor *descriptor,HArrayOfByte *bytes,long length)
{
    XFontStruct *xfont;
    long width=0;
    if (!font || !descriptor || !bytes) { SignalError(EE(),"java/lang/NullPointerException",NULL); return 0; }
    if (length<0 || length>obj_length(bytes)) { SignalError(EE(),"java/lang/ArrayIndexOutOfBoundsException",NULL); return 0; }
    AWT_LOCK();
    xfont=awt118_segment_font(font,descriptor);
    if (xfont) width=xfont->min_byte1 || xfont->max_byte1 ?
        XTextWidth16(xfont,(XChar2b *)unhand(bytes)->body,length/2) : XTextWidth(xfont,unhand(bytes)->body,length);
    AWT_UNLOCK();
    return width;
}
