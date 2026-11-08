/* Legacy stack ABI used by the unchanged DR2 image classes. */
#include "StubPreamble.h"
#include "sun_awt_image_ImageRepresentation.h"
#include "sun_awt_image_OffScreenImageSource.h"

JAVA_PUBLIC_API(stack_item *)
Java_sun_awt_image_ImageRepresentation_offscreenInit_stub(stack_item *args, struct execenv *ee)
{
    sun_awt_image_ImageRepresentation_offscreenInit(args[0].p, args[1].p);
    return args;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_awt_image_ImageRepresentation_disposeImage_stub(stack_item *args, struct execenv *ee)
{
    sun_awt_image_ImageRepresentation_disposeImage(args[0].p);
    return args;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_awt_image_ImageRepresentation_imageDraw_stub(stack_item *args, struct execenv *ee)
{
    sun_awt_image_ImageRepresentation_imageDraw(args[0].p, args[1].p, args[2].i, args[3].i, args[4].p);
    return args;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_awt_image_OffScreenImageSource_sendPixels_stub(stack_item *args, struct execenv *ee)
{
    sun_awt_image_OffScreenImageSource_sendPixels(args[0].p);
    return args;
}
