/* Sun mmedia_g.dll 10001500; local names reconstructed. */
#include "audio.h"
int ulaw2linear(signed char sample)
{
    static const int exp_lut[8] = {0,132,396,924,1980,4092,8316,16764};
    int value = ~sample;
    int exponent = (value >> 4) & 7;
    int magnitude = ((value & 15) << (exponent + 3)) + exp_lut[exponent];
    return value < 0 ? -magnitude : magnitude;
}
