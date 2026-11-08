#ifndef sun_java_inttypes_md_h___
#define sun_java_inttypes_md_h___

/* The Rhapsody 5 SDK predates stdint.h but supplies BSD fixed-width types. */
#if defined(__APPLE__) && defined(NS_TARGET_MAJOR) && NS_TARGET_MAJOR == 5
#include <sys/types.h>
typedef u_int32_t uint32_t;
typedef u_int64_t uint64_t;
#else
#include <stdint.h>
#endif

#endif
