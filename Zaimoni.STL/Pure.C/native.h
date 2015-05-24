/* native.h */

#ifndef ZAIMONI_STL_PURE_C_NATIVE_H
#define ZAIMONI_STL_PURE_C_NATIVE_H 1

/* wrap the results from the configuring executables */
#include "auto_int.h"

/* big-endian/little-endian support details */
#if defined(ZAIMONI_LITTLE_ENDIAN) && defined(ZAIMONI_BIG_ENDIAN)
#error ZAIMONI_LITTLE_ENDIAN and ZAIMONI_BIG_ENDIAN are mutually exclusive
#endif
#if !defined(ZAIMONI_LITTLE_ENDIAN) && !defined(ZAIMONI_BIG_ENDIAN)
#error ZAIMONI_LITTLE_ENDIAN and ZAIMONI_BIG_ENDIAN are mutually exhaustive
#endif

#ifdef ZAIMONI_LITTLE_ENDIAN
#define INLINE_LITTLE_ENDIAN inline
#define INLINE_BIG_ENDIAN
#else
#define INLINE_LITTLE_ENDIAN
#define INLINE_BIG_ENDIAN inline
#endif

#endif
