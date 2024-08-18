//
// Copyright (C) 2015 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_COMPILER_H_INCLUDED
#define SOCI_PRIVATE_SOCI_COMPILER_H_INCLUDED

#include "soci-cpp.h"

// SOCI_CHECK_GCC(major,minor) evaluates to 1 when using g++ of at least this
// version or 0 when using g++ of lesser version or not using g++ at all.
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#   define SOCI_CHECK_GCC(major, minor) \
        ((__GNUC__ > (major)) \
            || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#   define SOCI_CHECK_GCC(major, minor) 0
#endif

// GCC_WARNING_{SUPPRESS,RESTORE} macros can be used to bracket the code
// producing a specific warning to disable it.
//
// They only work with g++ 4.6+ or clang, warnings are not disabled for earlier
// g++ versions.
#if defined(__clang__) || SOCI_CHECK_GCC(4, 6)
#   define SOCI_GCC_WARNING_SUPPRESS(x) \
        _Pragma (SOCI_STRINGIZE(GCC diagnostic push)) \
        _Pragma (SOCI_STRINGIZE(GCC diagnostic ignored SOCI_STRINGIZE(SOCI_CONCAT(-W,x))))
#   define SOCI_GCC_WARNING_RESTORE(x) \
        _Pragma (SOCI_STRINGIZE(GCC diagnostic pop))
#else /* gcc < 4.6 or not gcc and not clang at all */
#   define SOCI_GCC_WARNING_SUPPRESS(x)
#   define SOCI_GCC_WARNING_RESTORE(x)
#endif

// SOCI_FALLTHROUGH macro can be used on both g++, mingw, and msvc to generate the
// correct behavior for expected fallthrough in switch statements.
#if defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#   if __cplusplus >= 201402L
#       define SOCI_FALLTHROUGH [[fallthrough]]
#   else
#       define SOCI_FALLTHROUGH ((void)0)
#   endif
#elif defined(_MSC_VER)
#   if _MSC_VER >= 1910 && __cplusplus >= 201703L
#       define SOCI_FALLTHROUGH [[fallthrough]]
#   else
#       define SOCI_FALLTHROUGH ((void)0)
#   endif
#else
#   define SOCI_FALLTHROUGH ((void)0)
#endif

#endif // SOCI_PRIVATE_SOCI_COMPILER_H_INCLUDED
