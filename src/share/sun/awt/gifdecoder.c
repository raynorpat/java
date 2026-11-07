/*
 * @(#)gifdecoder.c	1.2 95/10/11 Jim Graham
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/* GIF LZW loop retained from Sun gifdecoder.c, adapted to the 1.1.5 ABI recovered
 * at winawt_g1002B930. Tables belong to the Java decoder (4096/4096/4097),
 * scanlines include frame offsets and interlace replication, and all data
 * subblocks are consumed before returning to Java's frame parser. */
#include "StubPreamble.h"
#include "sun_awt_image_GifImageDecoder.h"
#include "java_awt_image_IndexColorModel.h"
struct GifInput {
    Hsun_awt_image_GifImageDecoder *decoder;
    HArrayOfByte *block;
    ExecEnv *ee;
    int position,length,ended;
};
static int gif_read(struct GifInput *input,int count)
{
    long missing=execute_java_dynamic_method(input->ee,(HObject *)input->decoder,"readBytes","([BII)I",input->block,0,count);
    return !missing && !exceptionOccurred(input->ee);
}
static int gif_byte(struct GifInput *input)
{
    if (input->ended) return -1;
    if (input->position==input->length) {
        if (!gif_read(input,1)) { input->ended=TRUE; return -1; }
        input->length=(unsigned char)unhand(input->block)->body[0]; input->position=0;
        if (!input->length || !gif_read(input,input->length)) { input->ended=TRUE; return -1; }
    }
    return (unsigned char)unhand(input->block)->body[input->position++];
}
static int gif_finish(struct GifInput *input)
{
    while (!input->ended) {
        int count;
        if (!gif_read(input,1)) return !exceptionOccurred(input->ee);
        count=(unsigned char)unhand(input->block)->body[0];
        if (!count) { input->ended=TRUE; break; }
        if (!gif_read(input,count)) return !exceptionOccurred(input->ee);
    }
    return !exceptionOccurred(input->ee);
}
static int gifImage(stack_item *sp,ExecEnv *ee)
{
    Hsun_awt_image_GifImageDecoder *decoder=(Hsun_awt_image_GifImageDecoder *)sp[0].p;
    int left=sp[1].i,top=sp[2].i,width=sp[3].i,height=sp[4].i,initial=sp[6].i;
    int interlace=sp[5].i!=0;
    HArrayOfByte *block=(HArrayOfByte *)sp[7].p,*line=(HArrayOfByte *)sp[8].p;
    Hjava_awt_image_IndexColorModel *model=(Hjava_awt_image_IndexColorModel *)sp[9].p;
    if (!decoder || !block || !line || !model || !unhand(decoder)->input ||
        !unhand(decoder)->prefix || !unhand(decoder)->suffix || !unhand(decoder)->outCode) {
        SignalError(ee,"java/lang/NullPointerException",NULL); return FALSE;
    }
    HArrayOfShort *prefix=unhand(decoder)->prefix;
    HArrayOfByte *suffix=unhand(decoder)->suffix,*output=unhand(decoder)->outCode;
    if (obj_length(prefix)!=4096 || obj_length(suffix)!=4096 || obj_length(output)!=4097 ||
        obj_length(block)<256 || width>obj_length(line)) {
        SignalError(ee,"java/lang/ArrayIndexOutOfBoundsException",NULL); return FALSE;
    }
    if (initial<2 || initial>8 || width<=0 || height<=0) {
        SignalError(ee,"java/lang/IllegalArgumentException",NULL); return FALSE;
    }
    struct GifInput input={decoder,block,ee,0,0,FALSE};
    int clear=1<<initial,end=clear+1,codeSize=initial+1,maxCode=1<<codeSize,mask=maxCode-1;
    int freeCode=clear+2,oldCode=0,previous=0,bits=0,offset=0,row=0;
    unsigned data=0; int tableMask=unhand(model)->map_size-1;
    int step=interlace?8:1,replicate=step;
    for (;;) {
        while (bits<codeSize) {
            int byte=gif_byte(&input);
            if (byte<0) {
                if (offset && !exceptionOccurred(ee))
                    execute_java_dynamic_method(ee,(HObject *)decoder,"sendPixels",
                        "(IIII[BLjava/awt/image/ColorModel;)I",left,top+row,width,replicate,line,model);
                return !exceptionOccurred(ee);
            }
            data|=(unsigned)byte<<bits; bits+=8;
        }
        int code=data&mask; data>>=codeSize; bits-=codeSize;
        if (code==clear) {
            freeCode=clear+1; codeSize=initial+1; maxCode=1<<codeSize; mask=maxCode-1;
            if (row<height) continue;
            code=end;
        }
        if (code==end) return gif_finish(&input);
        int current=code,count=4097;
        if (current>=freeCode) {
            if (current>freeCode) return gif_finish(&input);
            current=oldCode; unhand(output)->body[--count]=(char)previous;
        }
        while (current>tableMask) {
            if (current>=4096 || current<0 || !count) return gif_finish(&input);
            unhand(output)->body[--count]=unhand(suffix)->body[current];
            current=unhand(prefix)->body[current];
        }
        if (!count) return gif_finish(&input);
        previous=current; unhand(output)->body[--count]=(char)previous;
        while (count<4097) {
            unhand(line)->body[offset++]=unhand(output)->body[count++];
            if (offset==width) {
                long consumers=execute_java_dynamic_method(ee,(HObject *)decoder,"sendPixels",
                    "(IIII[BLjava/awt/image/ColorModel;)I",left,top+row,width,replicate,line,model);
                if (consumers<=0 || exceptionOccurred(ee)) return FALSE;
                offset=0; row+=step;
                while (row>=height) {
                    step=replicate; replicate>>=1; row=replicate;
                    if (!replicate) return gif_finish(&input);
                }
            }
        }
        unhand(prefix)->body[freeCode]=(short)oldCode;
        unhand(suffix)->body[freeCode]=(char)previous; oldCode=code;
        if (++freeCode>=maxCode) {
            if (codeSize<12) { codeSize++; maxCode<<=1; mask=maxCode-1; }
            else freeCode=maxCode-1;
        }
    }
}
JAVA_PUBLIC_API(stack_item *) Java_sun_awt_image_GifImageDecoder_parseImage_stub(stack_item *sp,struct execenv *ee)
{ sp[0].i=gifImage(sp,ee); return sp+1; }
