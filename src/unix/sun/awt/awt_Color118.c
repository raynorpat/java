/* Solaris 3e058/3df14/3ec48: allocated colors, matching and indexed model.
 * Linux uses a deterministic color cube plus gray ramp in the host colormap. */
#include "awt118.h"
#include <stdlib.h>

static XColor colors[256];
static unsigned char allocated[256];
static int colorCount;

static void allocate_color(int red,int green,int blue)
{
    XColor color;
    color.red=red*257; color.green=green*257; color.blue=blue*257;
    color.flags=DoRed|DoGreen|DoBlue;
    if(XAllocColor(awt_display,awt_cmap,&color) && color.pixel<256) {
        colors[color.pixel]=color; allocated[color.pixel]=1;
    }
}
void awt118_init_colors(void)
{
    unsigned long freePixels[256],reserved[32],planes;
    unsigned char available[256]={0};
    int i,step,freeCount=0,reservedCount=0,r,g,b;
    if(awt_visInfo.class==TrueColor) return;
    colorCount=awt_visInfo.colormap_size;
    if(colorCount>256) colorCount=256;
    for(i=0;i<colorCount;i++) colors[i].pixel=i;
    XQueryColors(awt_display,awt_cmap,colors,colorCount);
    /* Retain existing shared colors; don't match against unallocated cells. */
    if(awt_visInfo.class==PseudoColor || awt_visInfo.class==GrayScale) {
        for(step=128;step;step/=2)
            if(freeCount+step<=colorCount && XAllocColorCells(awt_display,awt_cmap,False,&planes,0,freePixels+freeCount,step)) freeCount+=step;
        for(i=0;i<freeCount;i++) available[freePixels[i]]=1;
        if(freeCount) XFreeColors(awt_display,awt_cmap,freePixels,freeCount,0);
        /* Leave cells for subsequent Motif widget colors and other clients. */
        for(step=32;step;step/=2)
            if(XAllocColorCells(awt_display,awt_cmap,False,&planes,0,reserved,step)) { reservedCount=step; break; }
    }
    for(i=0;i<colorCount;i++) if(!available[i]) allocate_color(colors[i].red>>8,colors[i].green>>8,colors[i].blue>>8);
    allocate_color(0,0,0); allocate_color(255,255,255);
    if(awt_visInfo.class!=GrayScale && awt_visInfo.class!=StaticGray) {
        allocate_color(255,0,0); allocate_color(0,255,0); allocate_color(0,0,255);
        allocate_color(255,255,0); allocate_color(255,0,255); allocate_color(0,255,255);
    }
    allocate_color(128,128,128); allocate_color(192,192,192);
    for(i=0;i<256;i+=17) allocate_color(i,i,i);
    if(awt_visInfo.class!=GrayScale && awt_visInfo.class!=StaticGray)
        for(r=0;r<256;r+=51) for(g=0;g<256;g+=51) for(b=0;b<256;b+=51) allocate_color(r,g,b);
    if(reservedCount) XFreeColors(awt_display,awt_cmap,reserved,reservedCount,0);
}
unsigned long awt118_index_pixel(unsigned long rgb)
{
    int r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
    int i,best=0,distance=0x7fffffff,gray=r==g && g==b;
    for(i=0;i<colorCount;i++) if(allocated[i]) {
        int cr=colors[i].red>>8,cg=colors[i].green>>8,cb=colors[i].blue>>8;
        int dr=r-cr,dg=g-cg,db=b-cb,value=dr*dr+dg*dg+db*db;
        if(gray && (cr!=cg || cg!=cb)) continue;
        if(value<distance) { best=i; distance=value; if(!value) break; }
    }
    return best;
}
struct Hjava_awt_image_ColorModel *awt118_index_model(void)
{
    HArrayOfByte *red=(HArrayOfByte *)ArrayAlloc(T_BYTE,256);
    HArrayOfByte *green=(HArrayOfByte *)ArrayAlloc(T_BYTE,256);
    HArrayOfByte *blue=(HArrayOfByte *)ArrayAlloc(T_BYTE,256);
    int i;
    if(!red || !green || !blue) { SignalError(EE(),"java/lang/OutOfMemoryError",NULL); return NULL; }
    for(i=0;i<colorCount;i++) {
        unhand(red)->body[i]=colors[i].red>>8;
        unhand(green)->body[i]=colors[i].green>>8;
        unhand(blue)->body[i]=colors[i].blue>>8;
    }
    return (struct Hjava_awt_image_ColorModel *)execute_java_constructor(EE(),
        "java/awt/image/IndexColorModel",NULL,"(II[B[B[B)",awt_visInfo.depth,colorCount,red,green,blue);
}
