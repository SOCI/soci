//
// Copyright (C) 2006-2008 Mateusz Loskot
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PLATFORM_H_INCLUDED
#define SOCI_PLATFORM_H_INCLUDED

//disable MSVC deprecated warnings
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdarg.h>
#include <string.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <memory>

#include "soci/soci-config.h" // for SOCI_HAVE_CXX11

#if defined(_MSC_VER)
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

// Define if you have the snprintf variants.
#if _MSC_VER < 1900
# define snprintf _snprintf
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

#ifdef _WIN32
# ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0502 //_WIN32_WINNT_WS03, VS2015 support: https://msdn.microsoft.com/de-de/library/6sehtctf.aspx
# endif

# ifdef SOCI_DLL
#  define SOCI_DECL_EXPORT __declspec(dllexport)
#  define SOCI_DECL_IMPORT __declspec(dllimport)
# endif

#elif defined(SOCI_HAVE_VISIBILITY_SUPPORT)
# define SOCI_DECL_EXPORT __attribute__ (( visibility("default") ))
# define SOCI_DECL_IMPORT __attribute__ (( visibility("default") ))
#endif

#ifndef SOCI_DECL_EXPORT
# define SOCI_DECL_EXPORT
# define SOCI_DECL_IMPORT
#endif

// Define SOCI_DECL
#ifdef SOCI_SOURCE
# define SOCI_DECL SOCI_DECL_EXPORT
#else
# define SOCI_DECL SOCI_DECL_IMPORT
#endif

// C++11 features are always available in MSVS as it has no separate C++98
// mode, we just need to check for the minimal compiler version supporting them
// (see https://msdn.microsoft.com/en-us/library/hh567368.aspx).

#if defined(SOCI_HAVE_CXX11) || (defined(_MSC_VER) && _MSC_VER >= 1800)
# define SOCI_OVERRIDE override
#else
# define SOCI_OVERRIDE
#endif

namespace soci
{

namespace cxx_details
{

#if defined(SOCI_HAVE_CXX11) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    template <typename T>
    using auto_ptr = std::unique_ptr<T>;
#else // std::unique_ptr<> not available
    using std::auto_ptr;
#endif

#if defined(SOCI_HAVE_CXX11) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <memory>
    template <typename T>
    using shared_ptr = std::shared_ptr<T>;

    template<class T, class U>
    inline shared_ptr<T> static_pointer_cast(shared_ptr<U> const& r)
    { return std::static_pointer_cast<T>(r); }

    template<class T, class U>
    inline shared_ptr<T> dynamic_pointer_cast(shared_ptr<U> const& r)
    { return std::dynamic_pointer_cast<T>(r); }

    template<class T, class U>
    inline shared_ptr<T> const_pointer_cast(shared_ptr<U> const& r)
    { return std::const_pointer_cast<T>(r); }

#else // std::shared_ptr<> not available
    template <typename T>
    class shared_ptr
    {
        public:
        shared_ptr() : ptr_(NULL), refs_(new unsigned int(0))
        { }

        shared_ptr(T * ptr) : ptr_(ptr), refs_(new unsigned int(1))
        { }

        shared_ptr(T * ptr, unsigned int* refs) : ptr_(ptr), refs_(refs)
        { (*refs_)++; }

        shared_ptr(const shared_ptr & obj)
        {
            this->ptr_ = obj.ptr_;
            this->refs_ = obj.refs_;
            if (obj.ptr_ != NULL)
            {
                (*this->refs_)++; 
            }
        }
        
        shared_ptr& operator=(const shared_ptr & obj)
        {
            decRef();
            this->ptr_ = obj.ptr_;
            this->refs_ = obj.refs_;
            if (obj.ptr_ != NULL)
            {
                (*this->refs_)++; 
            }
            return *this;
        }

        ~shared_ptr()
        {
            decRef();
        }

        T* operator->() const { return this->ptr_; }
        T& operator*() const { return *this->ptr_; }

        unsigned int* get_counter() const
        {
            return this->refs_;
        }

        T* get() const
        {
            return this->ptr_;
        }
        
        private:
            void decRef()
            {
                if (--(*this->refs_) == 0)
                {
                    if (ptr_ != NULL)
                    {
                        delete ptr_;
                    }
                    delete refs_;
                }
            }
            T *ptr_;
            unsigned int *refs_;
    };
    
    template<class T, class U>
    inline T* static_pointer_cast(U *ptr)
    { return static_cast<T*>(ptr); }

    template<class T, class U>
    inline T* dynamic_pointer_cast(U *ptr)
    { return dynamic_cast<T*>(ptr); }

    template<class T, class U>
    inline T* const_pointer_cast(U *ptr)
    { return const_cast<T*>(ptr); }

    template<class T, class U>
    inline shared_ptr<T> static_pointer_cast(shared_ptr<U> const& r)
    { return shared_ptr<T>(static_pointer_cast<T>(r.get()), r.get_counter()); }

    template<class T, class U>
    inline shared_ptr<T> dynamic_pointer_cast(shared_ptr<U> const& r)
    { return shared_ptr<T>(dynamic_pointer_cast<T>(r.get()), r.get_counter()); }

    template<class T, class U>
    inline shared_ptr<T> const_pointer_cast(shared_ptr<U> const& r)
    { return shared_ptr<T>(const_pointer_cast<T>(r.get()), r.get_counter()); }

#endif

} // namespace cxx_details

} // namespace soci

#if defined(SOCI_HAVE_CXX11) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #define SOCI_NOT_ASSIGNABLE(classname) \
        classname& operator=(const classname&) = delete;
    #define SOCI_NOT_COPYABLE(classname) \
        classname(const classname&) = delete; \
        SOCI_NOT_ASSIGNABLE(classname)
#else // no C++11 deleted members support
    #define SOCI_NOT_ASSIGNABLE(classname) \
        classname& operator=(const classname&);
    #define SOCI_NOT_COPYABLE(classname) \
        classname(const classname&); \
        SOCI_NOT_ASSIGNABLE(classname)
#endif // C++11 deleted members available

#define SOCI_UNUSED(x) (void)x;

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
#if defined(_MSC_VER) && defined(NDEBUG)
    #define SOCI_DUMMY_RETURN(x)
#else
    #define SOCI_DUMMY_RETURN(x) return x
#endif

#if defined(SOCI_HAVE_CXX11) || (defined(_MSC_VER) && _MSC_VER >= 1900)
    #define SOCI_NOEXCEPT noexcept
    #define SOCI_NOEXCEPT_FALSE noexcept(false)
#else
    #if defined(__cplusplus) && __cplusplus >= 201103L
        // Otherwise throwing from a dtor not marked with noexcept(false) would
        // simply result in terminating the program.
        #error "SOCI must be configured with C++11 support when using C++11"
    #endif

    #define SOCI_NOEXCEPT throw()
    #define SOCI_NOEXCEPT_FALSE
#endif

#endif // SOCI_PLATFORM_H_INCLUDED
