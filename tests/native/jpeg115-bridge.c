/* Test receiver with the same callback protocol as JPEGImageDecoder.
 * Forward the original public stub unchanged, including receiver slot.
 */
#include "native.h"
extern stack_item *Java_sun_awt_image_JPEGImageDecoder_readImage_stub(stack_item *, ExecEnv *);
#ifdef WIN32
__declspec(dllexport)
#endif
stack_item *Java_JpegLibrary115_decode_stub(stack_item *sp, ExecEnv *ee)
{
    return Java_sun_awt_image_JPEGImageDecoder_readImage_stub(sp, ee);
}
