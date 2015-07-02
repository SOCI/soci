//
// Copyright (C) 2006-2008 Mateusz Loskot
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

#include <stdarg.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>


#if defined(_MSC_VER) || defined(__MINGW32__)
#define LL_FMT_FLAGS "I64"
#else
#define LL_FMT_FLAGS "ll"
#endif

// Portability hacks for Microsoft Visual C++ compiler
#ifdef _MSC_VER
#include <stdlib.h>

//Disables warnings about STL objects need to have dll-interface and/or
//base class must have dll interface
#pragma warning(disable:4251 4275)

// Define if you have the vsnprintf variants.
#if _MSC_VER < 1500
# define vsnprintf _vsnprintf
#endif

#if _MSC_VER >= 1400 //use secure version removing compiler warnings
    #define sprintf sprintf_s //just use secure version( same prototype )
    #define sscanf sscanf_s //just use secure version( same prototype )
    #define strncpy strncpy_secure //use inline wrapper bellow
    #define strcpy strcpy_secure //use inline wrapper bellow
    #define snprintf snprintf_secure //use inline wrapper bellow

    inline char* strncpy_secure(char* buf, const char* src, size_t count)
    {
        errno_t err = strncpy_s(buf,count,src,count);
        if( err != 0 )
            return 0;
        return buf;
    }

    inline char* strcpy_secure(char* dest, const char* src)
    {
        errno_t err = strcpy_s(dest,strlen(src)+1,src);
        if( err != 0 )
            return 0;
        return dest;
    }

    inline char* snprintf_secure(char* buf,size_t sizeOfBuf, const char* format, ... )
    {
        va_list va;
        va_start(va,format);
    #if defined(_MSC_VER) && _MSC_VER >= 1400
        vsnprintf_s(buf, sizeOfBuf,sizeOfBuf,format,va);
    #else
        vsnprintf(buf,sizeOfBuf,format,va);
    #endif
        va_end(va);
        return buf;
    }

    //based on windows cstring.h header - will include global functions into std namespace
    namespace std
    {
        using ::sscanf_s; //include secure version into std namespace
        using ::sprintf_s;
        using ::strcpy_secure; //include secure wrapped version into std namespace
        using ::snprintf_secure;
    }
#else //fallback to previous variant
// Define if you have the snprintf variants.
#define snprintf _snprintf
#endif

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

//wrapper functions moved into soci namespace 
namespace soci
{
    //will return value for environment variable name or empty string if variable is not defined or it's value is ""
    inline std::string getenv(const char* envVarName)
    {
        char* penv;
    #if defined(_MSC_VER) && _MSC_VER >= 1400
        size_t size;
        errno_t err = _dupenv_s(&penv,&size,envVarName);
        if( err == 0 && penv != 0)
        {
            std::string result = penv;
            free( penv );
            return result;
        }
    #else
        penv = std::getenv(envVarName);
        if( penv != 0 )
            return std::string(penv);
    #endif
        return std::string();
    }
    //wrap localtime version and use it to remove warnings about using unsecured version warning
    inline int localtime(struct std::tm& tstruct, const time_t& timer)
    {
    #if defined(_MSC_VER) && _MSC_VER >= 1400
        errno_t err = localtime_s(&tstruct,&timer);
        return err;
    #else //fallback on std localtime for all other systems
        tm* result = std::localtime(&timer);
        if( result == 0 )
            return -1; //indicate error
        tstruct = *result;
        return 0;
    #endif
    }
}


//define DLL import/export on WIN32
#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_SOURCE
#   define SOCI_DECL __declspec(dllexport)
#  else
#   define SOCI_DECL __declspec(dllimport)
#  endif // SOCI_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_DECL isn't defined yet define it now
#ifndef SOCI_DECL
# define SOCI_DECL
#endif


#endif // SOCI_PLATFORM_H_INCLUDED
