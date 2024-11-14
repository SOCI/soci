#ifndef SOCI_TYPES_H_INCLUDED
#define SOCI_TYPES_H_INCLUDED

#include "soci/soci-platform.h"

// This should normally be defined in soci-config.h included from
// soci-platform.h, but provide defaults for legacy non-CMake builds.
#ifndef SOCI_SIZEOF_LONG
    #if defined(_WIN32) || defined(_WIN64)
        // Under Windows long is always 32 bits, even for 64-bit builds.
        #define SOCI_SIZEOF_LONG 4
    #elif defined(__LP64__)
        // Note that currently we assume that all non-macOS Unix systems with
        // 64-bit long int64_t define it as long, which seems to be true for
        // all supported platforms.
        #ifndef __APPLE__
            #define SOCI_INT64_T_IS_LONG
        #endif
        #define SOCI_SIZEOF_LONG 8
    #else
        // Also note that on 32-bit platforms long is never the same thing as
        // int32_t, as the latter is defined as int instead.
        #define SOCI_SIZEOF_LONG 4
    #endif
#endif

#if SOCI_SIZEOF_LONG == 8
    #define SOCI_LONG_IS_64_BIT 1
#endif

#endif // SOCI_TYPES_H_INCLUDED
