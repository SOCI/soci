//
// Copyright (C) 2006-2008 Mateusz Loskot
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

// Check some C++11 Features
#if     defined(_MSC_VER)
// Bug in Visual Studio: __cplusplus still means C++03
// https://connect.microsoft.com/VisualStudio/feedback/details/763051/
#define SOCI_HAVE_NOEXCEPT (_MSC_VER >= 1900)
#define SOCI_HAVE_LAMBDA   (_MSC_VER >= 1600)
#elif   defined(__clang__) || defined(__GNUG__)
#if     defined(__cplusplus) && __cplusplus >= 201103L
// This simple check leaves the user with a too old compiler barfing.
#define SOCI_HAVE_NOEXCEPT 1
#define SOCI_HAVE_LAMBDA   1
#else
#define SOCI_HAVE_NOEXCEPT 0
#define SOCI_HAVE_LAMBDA   0
#endif // __cplusplus
#else
#error Unknown Compiler. Please update these checks.
#endif // clang/gcc


#if defined(_MSC_VER) || defined(__MINGW32__)
#define LL_FMT_FLAGS "I64"
#else
#define LL_FMT_FLAGS "ll"
#endif

// Portability hacks for Microsoft Visual C++ compiler
#ifdef _MSC_VER
#include <stdlib.h>

// Define if you have the vsnprintf variants.
#if _MSC_VER < 1500
# define vsnprintf _vsnprintf
#endif

// Define if you have the snprintf variants.
#define snprintf _snprintf

// Define if you have the strtoll and strtoull variants.
#if _MSC_VER < 1300
# error "Visual C++ versions prior 1300 don't support _strtoi64 and _strtoui64"
#elif _MSC_VER >= 1300 && _MSC_VER < 1800
namespace std {
    inline long long strtoll(char const* str, char** str_end, int base)
    {
        return _strtoi64(str, str_end, base);
    }

    inline unsigned long long strtoull(char const* str, char** str_end, int base)
    {
        return _strtoui64(str, str_end, base);
    }
}
#endif // _MSC_VER < 1800
#endif // _MSC_VER

#if defined(__CYGWIN__) || defined(__MINGW32__)
#include <stdlib.h>
namespace std {
    using ::strtoll;
    using ::strtoull;
}
#endif

#endif // SOCI_PLATFORM_H_INCLUDED
