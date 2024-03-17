#ifndef SOCI_TYPES_H_INCLUDED
#define SOCI_TYPES_H_INCLUDED

#include "soci/soci-platform.h"

#if defined(__GNUC__) || defined(__clang__)
    #if defined(__LP64__)
        #define SOCI_LONG_IS_64_BIT 1
        #if SOCI_OS == SOCI_OS_LINUX || SOCI_OS == SOCI_OS_FREE_BSD
            #define SOCI_INT64_IS_LONG 1
        #endif
    #endif
#endif

#endif // SOCI_TYPES_H_INCLUDED
