#ifndef sun_java_typedefs_md_h___
#define sun_java_typedefs_md_h___

#include "bool.h"
#include <sys/types.h>
/* Required before the original sys_api.h stat prototypes. */
#include <sys/stat.h>
#include <stdlib.h>

/* Win32 (MSVC): native 64-bit integers */
#define HAVE_LONG_LONG

typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef unsigned int uint_t;
typedef unsigned long uintVP_t; /* unsigned that is same size as a void pointer */

/* DLL Entry modifiers... */
#ifdef JAVA_INTERNAL_BUILD
/* The runtime DLL exports only the supported Sun names in javai.def.
 * Legacy native adapters are resolved by its private registry. */
#define EXPORTED extern
#define JAVA_PUBLIC_API(ResultType) ResultType
#else
#define EXPORTED                	extern __declspec(dllexport)
#define JAVA_PUBLIC_API(ResultType)	_declspec(dllexport) ResultType
#endif
#define JAVA_CALLBACK

/************************************************************************/

#define ll_high(a)              ((long)((a)>>32))
#define ll_low(a)               ((long)(a))
#define int2ll(a)               ((int64_t)(a))
#define uint2ll(a)              ((int64_t)(uint32_t)(a))
#define ll2int(a)               ((int)(a))
#define ll_add(a, b)    ((a) + (b))
#define ll_and(a, b)    ((a) & (b))
#define ll_div(a, b)    ((a) / (b))
#define ll_mul(a, b)    ((a) * (b))
#define ll_neg(a)               (-(a))
#define ll_not(a)               (~(a))
#define ll_or(a, b)             ((a) | (b))
#define ll_shl(a, n)    ((a) << (n))
#define ll_shr(a, n)    ((a) >> (n))
#define ll_sub(a, b)    ((a) - (b))
#define ll_ushr(a, n)   ((int64_t)((uint64_t)(a) >> (n)))
#define ll_xor(a, b)    ((a) ^ (b))
#define ll_rem(a,b)             ((a) % (b))

#define float2ll(f)             ((int64_t) (f))
#define ll2float(a)             ((float) (a))
#define ll2double(a)    ((double) (a))
#define double2ll(f)    ((int64_t) (f))

/* comparison operators */
#define ll_ltz(ll)              ((ll)<0)
#define ll_gez(ll)              ((ll)>=0)
#define ll_eqz(a)               ((a) == 0)
#define ll_eq(a, b)             ((a) == (b))
#define ll_ne(a,b)              ((a) != (b))
#define ll_ge(a,b)              ((a) >= (b))
#define ll_le(a,b)              ((a) <= (b))
#define ll_lt(a,b)              ((a) < (b))
#define ll_gt(a,b)              ((a) > (b))

#define ll_zero_const   ((int64_t) 0)
#define ll_one_const    ((int64_t) 1)

#define a2ll(x) _atoi64(x)

extern void ll2str(int64_t a, char *s, char *limit);

/************************************************************************/

/* XXX The bucket for stuff that seems to have no home... */
extern struct tm * gmtime_r( const time_t*, struct tm * );
extern struct tm * localtime_r( const time_t*, struct tm * );

extern void srand48( long seed );
extern double drand48();

#define popen    _popen

#endif /* sun_java_typedefs_md_h___ */
