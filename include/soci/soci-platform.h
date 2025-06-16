//
// Copyright (C) 2006-2008 Mateusz Loskot
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

#include <stdarg.h>
#include <string.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>

#include "soci/soci-config.h"

#if defined(_MSC_VER)
#define LL_FMT_FLAGS "I64"
#else
#define LL_FMT_FLAGS "ll"
#endif

#ifdef _MSC_VER
//Disables warnings about STL objects need to have dll-interface and/or
//base class must have dll interface
#pragma warning(disable:4251 4275)
#endif

#if defined(__CYGWIN__) || defined(__MINGW32__)
#include <stdlib.h>
namespace std {
    using ::strtoll;
    using ::strtoull;
}
#endif

#ifdef _WIN32
# ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0502 //_WIN32_WINNT_WS03, VS2015 support: https://msdn.microsoft.com/de-de/library/6sehtctf.aspx
# endif
// SOCI_DLL should be defined when SOCI is used as a DLL/DSO
# ifdef SOCI_DLL
#  define SOCI_DECL_EXPORT __declspec(dllexport)
#  define SOCI_DECL_IMPORT __declspec(dllimport)
# endif
// assuming GCC-like compiler
#else
// note: public visibility only applied when SOCI_DLL is defined. current build
// config will use -fvisibility=hidden or equivalent for unexported symbols
# ifdef SOCI_DLL
#  define SOCI_DECL_EXPORT __attribute__ (( visibility("default") ))
#  define SOCI_DECL_IMPORT __attribute__ (( visibility("default") ))
# endif
#endif

// default to no import/export (default visibility for GCC)
#ifndef SOCI_DECL_EXPORT
# define SOCI_DECL_EXPORT
# define SOCI_DECL_IMPORT
#endif

// Define SOCI_DECL (controls symbols visibility).
// SOCI_SOURCE should *only* be defined during SOCI's own compilation
#ifdef SOCI_SOURCE
# define SOCI_DECL SOCI_DECL_EXPORT
#else
# define SOCI_DECL SOCI_DECL_IMPORT
#endif

// C++11 features are always available in MSVS as it has no separate C++98
// mode, we just need to check for the minimal compiler version supporting them
// (see https://msdn.microsoft.com/en-us/library/hh567368.aspx).

#ifdef _MSC_VER
    #if _MSC_VER < 1900
        #error This version of SOCI requires MSVS 2015 or later.
    #endif
    #if _MSVC_LANG >= 201703L
        #define SOCI_HAVE_CXX17
    #endif
#else
    #if __cplusplus < 201402L
        #error This version of SOCI requires C++14.
    #endif
    #if __cplusplus >= 201703L
        #define SOCI_HAVE_CXX17
    #endif
#endif

#define SOCI_NOT_ASSIGNABLE(classname) \
public: \
    classname(const classname&) = default; \
private: \
    classname& operator=(const classname&) = delete;
#define SOCI_NOT_COPYABLE(classname) \
    classname(const classname&) = delete; \
    classname& operator=(const classname&) = delete;

#define SOCI_UNUSED(x) (void)x

// This macro can be used to avoid warnings from MSVC (and sometimes from gcc,
// if initialization is indirect) about "uninitialized" variables that are
// actually always initialized. Using this macro makes it clear that the
// initialization is only necessary to avoid compiler warnings and also will
// allow us to define it as doing nothing if we ever use a compiler warning
// about initializing variables unnecessarily.
#define SOCI_DUMMY_INIT(x) (x)

// And this one can be used to return after calling a "[[noreturn]]" function.
// Here the problem is that MSVC complains about unreachable code in this case
// (but only in release builds, where optimizations are enabled), while other
// compilers complain about missing return statement without it.
// Note: this macro is no longer used in SOCI's codebase. It is only retained
// in case downstream code is depending on it.
#if defined(_MSC_VER) && defined(NDEBUG)
    #define SOCI_DUMMY_RETURN(x)
#else
    #define SOCI_DUMMY_RETURN(x) return x
#endif

// Provide some wrappers for standard functions avoiding deprecation warnings
// from MSVC.
namespace soci
{

#ifdef _MSC_VER

inline FILE* fopen(const char* filename, const char* mode)
{
    FILE* file = nullptr;
    fopen_s(&file, filename, mode);
    return file;
}

inline const char* getenv(const char* name)
{
  #pragma warning(push)
  #pragma warning(disable:4996)

  return std::getenv(name);

  #pragma warning(pop)
}

inline int sscanf(const char* str, const char* format, ...)
{
  va_list args;
  va_start(args, format);

  const int result = vsscanf_s(str, format, args);

  va_end(args);
  return result;
}

inline char* strncpy(char* dest, const char* src, size_t n)
{
  strncpy_s(dest, n, src, _TRUNCATE);
  return dest;
}

#else // !_MSC_VER

using std::fopen;
using std::getenv;
using std::sscanf;
using std::strncpy;

#endif // MSC_VER/!_MSC_VER
}

#endif // SOCI_PLATFORM_H_INCLUDED
