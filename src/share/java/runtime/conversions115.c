/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Sun conversion semantics: Rhapsody double2l 5c137efc/double2ll 5c137f88;
 * Win32 behavior checked by NumericConversions115 on original javai.dll. */
#include "typedefs.h"

int32_t double2l(double value)
{
    if (value != value) return 0;
    if (value >= 2147483648.0) return (int32_t)0x7fffffff;
    if (value <= -2147483648.0) return (int32_t)0x80000000U;
    return (int32_t)value;
}

int32_t float2l(float value)
{
    return double2l(value);
}

int64_t double2ll(double value)
{
    if (value != value) return 0;
    if (value >= 9223372036854775808.0)
        return (int64_t)(((uint64_t)1 << 63) - 1);
    if (value <= -9223372036854775808.0)
        return (int64_t)((uint64_t)1 << 63);
    return (int64_t)value;
}

int64_t float2ll(float value)
{
    return double2ll(value);
}

#ifdef XP_PC
/* Original helper avoids the historical MSVC shift optimizer bug. */
int64_t ll_shl(int64_t value, int bits)
{
    return bits >= 64 ? 0 : (int64_t)((uint64_t)value << bits);
}
#endif
