#ifndef SOCI_FIXED_SIZE_INTS_H_INCLUDED
#define SOCI_FIXED_SIZE_INTS_H_INCLUDED

#include "soci/soci-types.h"
#include "soci/type-conversion-traits.h"

#include <cstdint>

namespace soci
{

// Completion of dt_[u]int* bindings for all architectures.
// This allows us to extract values on types where the type definition is
// specific to the underlying architecture. E.g. Unix defines a int64_t as
// long. This would make it impossible to extract a dt_int64 value as both
// long and long long. With the following type_conversion specializations,
// this becomes possible.

#if defined(SOCI_INT64_IS_LONG)
template <>
struct type_conversion<long long>
{
    typedef int64_t base_type;

    static void from_base(base_type const & in, indicator ind, long long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<long long>(in);
    }

    static void to_base(long long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};

template <>
struct type_conversion<unsigned long long>
{
    typedef uint64_t base_type;

    static void from_base(base_type const & in, indicator ind, unsigned long long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<unsigned long long>(in);
    }

    static void to_base(unsigned long long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};
#elif defined(SOCI_LONG_IS_64_BIT)
template <>
struct type_conversion<long>
{
    typedef int64_t base_type;

    static void from_base(base_type const & in, indicator ind, long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<long>(in);
    }

    static void to_base(long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};

template <>
struct type_conversion<unsigned long>
{
    typedef uint64_t base_type;

    static void from_base(base_type const & in, indicator ind, unsigned long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<unsigned long>(in);
    }

    static void to_base(unsigned long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};
#else
template <>
struct type_conversion<long>
{
    typedef int32_t base_type;

    static void from_base(base_type const & in, indicator ind, long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<long>(in);
    }

    static void to_base(long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};

template <>
struct type_conversion<unsigned long>
{
    typedef uint32_t base_type;

    static void from_base(base_type const & in, indicator ind, unsigned long & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<unsigned long>(in);
    }

    static void to_base(unsigned long const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};
#endif

} // namespace soci

#endif // SOCI_FIXED_SIZE_INTS_H_INCLUDED
