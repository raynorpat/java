#ifndef sun_java_typedefs_md_h___
#define sun_java_typedefs_md_h___

#include "bool.h"
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/*
 * Java longs need a 64-bit integer; <stdint.h> provides one even in C89
 * mode on every Unix this targets.
 */
#define HAVE_LONG_LONG

typedef unsigned int uint_t;
typedef unsigned long uintVP_t; /* unsigned that is same size as a void pointer */

#define EXPORTED                	extern
#define JAVA_PUBLIC_API(ResultType)	ResultType
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

extern void ll2str(int64_t a, char *s, char *limit);

#endif /* sun_java_typedefs_md_h___ */
