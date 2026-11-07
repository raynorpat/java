/* Native image records retain pixmaps until their last graphics owner retires. */
#ifndef AWT_IMAGE118_H
#define AWT_IMAGE118_H
#include "awt118.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "java_awt_Color.h"
typedef struct {
    Pixmap pixmap;
    int width,height,references,retired;
    unsigned long background;
    unsigned char *coverage; /* 0: undelivered, 1: transparent, 2: opaque */
    unsigned char *rows;
    int hints;
} AwtImage;
AwtImage *awt118_image(Hsun_awt_image_ImageRepresentation *self,Hjava_awt_Color *background);
void awt118_release_image(AwtImage *image);
#endif
