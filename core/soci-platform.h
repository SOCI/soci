//
// Copyright (C) 2006-2008 Mateusz Loskot
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

// Portability hacks for Microsoft Visual C++ compiler
#ifdef _MSC_VER

// Define if you have the vsnprintf variants.
#if _MSC_VER < 1500
# define HAVE_VSNPRINTF 1
# define vsnprintf _vsnprintf
#endif

// Define if you have the snprintf variants.
#define HAVE_SNPRINTF 1
#define snprintf _snprintf

// Define if you have the strtoll variants.
#if _MSC_VER >= 1300
# define HAVE_STRTOLL 1
# define strtoll(nptr, endptr, base) _strtoi64(nptr, endptr, base)
#else
# undef HAVE_STRTOLL
# error "Visual C++ versions prior 1300 don't support strtoi64"
#endif // _MSC_VER >= 1300

#endif // _MSC_VER

#endif // SOCI_PLATFORM_H_INCLUDED
