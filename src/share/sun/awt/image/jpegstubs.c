/* Classic javah stub, recovered from jpeg.dll at 1000E6E0. */
#include "native.h"
#include "sun_awt_image_JPEGImageDecoder.h"
#if defined(__GNUC__) && __GNUC__ >= 4
__attribute__((visibility("default")))
#endif
stack_item *Java_sun_awt_image_JPEGImageDecoder_readImage_stub(stack_item *sp, ExecEnv *ee)
{
    sun_awt_image_JPEGImageDecoder_readImage((Hsun_awt_image_JPEGImageDecoder *)sp[0].p,
                                            (struct Hjava_io_InputStream *)sp[1].p,
                                            (HArrayOfByte *)sp[2].p);
    return sp;
}
