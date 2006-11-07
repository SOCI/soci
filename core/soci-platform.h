#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

// Portability hacks for Microsoft Visual C++ compiler
#ifdef _MSC_VER

// Define if you have the vsnprintf variants.
#define HAVE_VSNPRINTF 1
#define vsnprintf _vsnprintf

// Define if you have the snprintf variants.
#define HAVE_SNPRINTF 1
#define snprintf _snprintf

// Define if you have the strtoll variants.
#if _MSC_VER >= 1300
# define HAVE_STRTOLL 1
# define strtoll(nptr, endptr, base) _strtoi64(nptr, endptr, base)
#else
# undef HAVE_STRTOLL
#endif // _MSC_VER >= 1300

#endif // _MSC_VER

#endif // SOCI_PLATFORM_H_INCLUDED