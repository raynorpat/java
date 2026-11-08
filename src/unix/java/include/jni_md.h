#ifndef sun_java_jni_md_h___
#define sun_java_jni_md_h___

/* deviation: Unix uses the existing pthreads port's fixed-width integer
 * convention; no Sun Unix JNI platform header is available in this tree. */
#include "inttypes_md.h"

#define JNIEXPORT
#define JNICALL

typedef int32_t jint;
typedef int64_t jlong;
typedef signed char jbyte;

#endif /* sun_java_jni_md_h___ */
