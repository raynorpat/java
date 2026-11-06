#ifndef sun_java_jmath_md_h___
#define sun_java_jmath_md_h___

#ifdef SOLARIS
#include <ieeefp.h>
#endif

/*
** Those namespace terrorists up in Redmond are holding "exception" hostage
** Free it now !!!
*/
#if defined(exception)
#undef exception
#endif

#define DREM(a,b) fmod(a,b)

/* IEEE 754 remainder: the quotient rounds to nearest (C99, MSVC 2013+). */
#define IEEEREM(a,b) remainder(a,b)

#endif /* sun_java_jmath_md_h___ */
