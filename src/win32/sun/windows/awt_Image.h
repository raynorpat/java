#ifndef AWT_IMAGE_H
#define AWT_IMAGE_H
#include "awt_Graphics.h"
extern "C" {
#include "sun_awt_image_ImageRepresentation.h"
}
AwtSurface *awtImageSurface(Hsun_awt_image_ImageRepresentation *, BOOL create);
HObject *awtImageColorModel(struct execenv *);
HPALETTE awtImagePalette();
void awtCleanupImagePalette();
#endif
