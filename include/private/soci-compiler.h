//
// Copyright (C) 2015 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
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

// SOCI_MSVC_WARNING_{SUPPRESS,RESTORE} macros are similar but for MSVC (they
// work for all the supported versions).
#if defined(_MSC_VER)
#   define SOCI_MSVC_WARNING_SUPPRESS(x) \
        __pragma(warning(push)) \
        __pragma(warning(disable:x))
#   define SOCI_MSVC_WARNING_RESTORE(x) \
        __pragma(warning(pop))
#else
#   define SOCI_MSVC_WARNING_SUPPRESS(x)
#   define SOCI_MSVC_WARNING_RESTORE(x)
#endif

// CHECK_CXX_STD(version) evaluates to 1 if the C++ version is at least the
// version specified.
#if defined(_MSVC_LANG)
#    define SOCI_CHECK_CXX_STD(ver) (_MSVC_LANG >= (ver))
#elif defined(__cplusplus)
#    define SOCI_CHECK_CXX_STD(ver) (__cplusplus >= (ver))
#else
#    define SOCI_CHECK_CXX_STD(ver) 0
#endif

// HAS_CLANG_FEATURE(feature) evaluates to 1 if clang is available and the
// provided feature is available.
#if !defined(__clang__) || !defined(__has_feature)
#   define SOCI_HAS_CLANG_FEATURE(x) 0
#else
#   define SOCI_HAS_CLANG_FEATURE(x) __has_feature(x)
#endif

// FALLTHROUGH macro defines a cross-platform/version to mark fallthrough
// behavior in switch statement.
#if SOCI_CHECK_CXX_STD(201703L)
#    define SOCI_FALLTHROUGH [[fallthrough]]
#elif SOCI_HAS_CLANG_FEATURE(cxx_attributes)
#    define SOCI_FALLTHROUGH [[clang::fallthrough]]
#elif defined(__has_cpp_attribute)
#    if __has_cpp_attribute(fallthrough)
#        define SOCI_FALLTHROUGH [[fallthrough]]
#    endif
#endif

#ifndef SOCI_FALLTHROUGH
    #define SOCI_FALLTHROUGH ((void)0)
#endif

#endif // SOCI_PRIVATE_SOCI_COMPILER_H_INCLUDED
