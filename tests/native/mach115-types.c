/* Compile and run with the DR2 SDK: both VM and public JNI types must agree. */
#include "typedefs_md.h"
#include "jni.h"
#include <stdio.h>

int main(void)
{
    uint64_t wide = ((uint64_t)1 << 63);
    if (sizeof(void *) != 4 || sizeof(int32_t) != 4 || sizeof(uint32_t) != 4 ||
        sizeof(int64_t) != 8 || sizeof(uint64_t) != 8 ||
        sizeof(jint) != 4 || sizeof(jlong) != 8 || sizeof(jbyte) != 1)
        return 1;
    if ((uint32_t)-1 != 0xffffffffU || (wide >> 63) != 1 ||
        ll_ushr((int64_t)-1, 63) != 1 || (jlong)-1 >= 0 || (jbyte)-1 >= 0)
        return 2;
    puts("DR2 VM/JNI integer widths and signedness passed");
    return 0;
}
