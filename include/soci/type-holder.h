//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_HOLDER_H_INCLUDED
#define SOCI_TYPE_HOLDER_H_INCLUDED

#include "soci/soci-backend.h"
#include "soci/soci-platform.h"
#include "soci/soci-types.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <limits>
#include <type_traits>
#include <typeinfo>

namespace soci
{

namespace details
{

// Returns U* as T*, if the dynamic type of the pointer is really T.
//
// This should be used instead of dynamic_cast<> because using it doesn't work
// when using libc++ and ELF visibility together. Luckily, when we don't need
// the full power of the cast, but only need to check if the types are the
// same, it can be done by comparing their type info objects.
//
// This function does _not_ replace dynamic_cast<> in all cases and notably
// doesn't allow the input pointer to be null.
template <typename T, typename U>
T* checked_ptr_cast(U* ptr)
{
    // Check if they're identical first, as an optimization, and then compare
    // their names to make it actually work with libc++.
    std::type_info const& ti_ptr = typeid(*ptr);
    std::type_info const& ti_ret = typeid(T);

    if (&ti_ptr != &ti_ret && std::strcmp(ti_ptr.name(), ti_ret.name()) != 0)
    {
        return NULL;
    }

    return static_cast<T*>(ptr);
}

// Type safe conversion that fails at compilation if instantiated
template <typename T, typename U, typename Enable = void>
struct soci_cast
{
    static inline T cast(U)
    {
        throw std::bad_cast();
    }
};

// Type safe conversion that is a noop
template <typename T, typename U>
struct soci_cast<
    T, U,
    typename std::enable_if<std::is_same<T, U>::value>::type>
{
    static inline T cast(U val)
    {
        return val;
    }
};

// Type safe conversion that is widening the type
template <typename T, typename U>
struct soci_cast<
    T, U,
    typename std::enable_if<(
        !std::is_same<T, U>::value &&
        std::is_integral<T>::value &&
        std::is_integral<U>::value
    )>::type>
{
    static inline T cast(U val) {
        intmax_t t_min = static_cast<intmax_t>((std::numeric_limits<T>::min)());
        intmax_t u_min = static_cast<intmax_t>((std::numeric_limits<U>::min)());
        uintmax_t t_max = static_cast<uintmax_t>((std::numeric_limits<T>::max)());
        uintmax_t u_max = static_cast<uintmax_t>((std::numeric_limits<U>::max)());

        if ((t_min > u_min && val < static_cast<U>(t_min)) ||
            (t_max < u_max && val > static_cast<U>(t_max)))
        {
            throw std::bad_cast();
        }

        return static_cast<T>(val);
    }
};

union type_holder
{
    std::string* s;
    int8_t* i8;
    int16_t* i16;
    int32_t* i32;
    int64_t* i64;
    uint8_t* u8;
    uint16_t* u16;
    uint32_t* u32;
    uint64_t* u64;
    double* d;
    std::tm* t;
};

template <typename T>
struct type_holder_trait
{
    static_assert(std::is_same<T, void>::value, "Unmatched raw type");
    // dummy value to satisfy the template engine, never used
    static const db_type type = (db_type)0;
};

template <>
struct type_holder_trait<std::string>
{
    static const db_type type = db_string;
};

template <>
struct type_holder_trait<int8_t>
{
    static const db_type type = db_int8;
};

template <>
struct type_holder_trait<int16_t>
{
    static const db_type type = db_int16;
};

template <>
struct type_holder_trait<int32_t>
{
    static const db_type type = db_int32;
};

template <>
struct type_holder_trait<int64_t>
{
    static const db_type type = db_int64;
};

template <>
struct type_holder_trait<uint8_t>
{
    static const db_type type = db_uint8;
};

template <>
struct type_holder_trait<uint16_t>
{
    static const db_type type = db_uint16;
};

template <>
struct type_holder_trait<uint32_t>
{
    static const db_type type = db_uint32;
};

template <>
struct type_holder_trait<uint64_t>
{
    static const db_type type = db_uint64;
};

#if defined(SOCI_INT64_IS_LONG)
template <>
struct type_holder_trait<long long> : type_holder_trait<int64_t>
{
};

template <>
struct type_holder_trait<unsigned long long> : type_holder_trait<uint64_t>
{
};
#elif defined(SOCI_LONG_IS_64_BIT)
template <>
struct type_holder_trait<long> : type_holder_trait<int64_t>
{
};

template <>
struct type_holder_trait<unsigned long> : type_holder_trait<uint64_t>
{
};
#else
template <>
struct type_holder_trait<long> : type_holder_trait<int32_t>
{
};

template <>
struct type_holder_trait<unsigned long> : type_holder_trait<uint32_t>
{
};
#endif

template <>
struct type_holder_trait<double>
{
    static const db_type type = db_double;
};

template <>
struct type_holder_trait<std::tm>
{
    static const db_type type = db_date;
};

// Base class for storing type data instances in a container of holder objects
class holder
{
public:
    template <typename T>
    static holder* make_holder(T* val)
    {
         return new holder(type_holder_trait<T>::type, val);
    }

    ~holder()
    {
        switch (dt_)
        {
        case db_double:
            delete val_.d;
            break;
        case db_int8:
            delete val_.i8;
            break;
        case db_int16:
            delete val_.i16;
            break;
        case db_int32:
            delete val_.i32;
            break;
        case db_int64:
            delete val_.i64;
            break;
        case db_uint8:
            delete val_.u8;
            break;
        case db_uint16:
            delete val_.u16;
            break;
        case db_uint32:
            delete val_.u32;
            break;
        case db_uint64:
            delete val_.u64;
            break;
        case db_date:
            delete val_.t;
            break;
        case db_blob:
        case db_xml:
        case db_string:
            delete val_.s;
            break;
        default:
            break;
        }
    }

    template <typename T>
    T get()
    {
        switch (dt_)
        {
        case db_int8:
            return soci_cast<T, int8_t>::cast(*val_.i8);
        case db_int16:
            return soci_cast<T, int16_t>::cast(*val_.i16);
        case db_int32:
            return soci_cast<T, int32_t>::cast(*val_.i32);
        case db_int64:
            return soci_cast<T, int64_t>::cast(*val_.i64);
        case db_uint8:
            return soci_cast<T, uint8_t>::cast(*val_.u8);
        case db_uint16:
            return soci_cast<T, uint16_t>::cast(*val_.u16);
        case db_uint32:
            return soci_cast<T, uint32_t>::cast(*val_.u32);
        case db_uint64:
            return soci_cast<T, uint64_t>::cast(*val_.u64);
        case db_double:
            return soci_cast<T, double>::cast(*val_.d);
        case db_date:
            return soci_cast<T, std::tm>::cast(*val_.t);
        case db_blob:
        case db_xml:
        case db_string:
            return soci_cast<T, std::string>::cast(*val_.s);
        default:
            throw std::bad_cast();
        }
    }

private:
    holder(db_type dt, void* val) : dt_(dt)
    {
        switch (dt_)
        {
        case db_double:
            val_.d = static_cast<double*>(val);
            break;
        case db_int8:
            val_.i8 = static_cast<int8_t*>(val);
            break;
        case db_int16:
            val_.i16 = static_cast<int16_t*>(val);
            break;
        case db_int32:
            val_.i32 = static_cast<int32_t*>(val);
            break;
        case db_int64:
            val_.i64 = static_cast<int64_t*>(val);
            break;
        case db_uint8:
            val_.u8 = static_cast<uint8_t*>(val);
            break;
        case db_uint16:
            val_.u16 = static_cast<uint16_t*>(val);
            break;
        case db_uint32:
            val_.u32 = static_cast<uint32_t*>(val);
            break;
        case db_uint64:
            val_.u64 = static_cast<uint64_t*>(val);
            break;
        case db_date:
            val_.t = static_cast<std::tm*>(val);
            break;
        case db_blob:
        case db_xml:
        case db_string:
            val_.s = static_cast<std::string*>(val);
            break;
        default:
            break;
        }
    }

    const db_type dt_;
    type_holder val_;
};

} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
