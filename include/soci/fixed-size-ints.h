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

template <>
struct type_conversion<soci_l_or_ll_t>
{
    typedef soci_l_or_ll_int_t base_type;

    static void from_base(base_type const & in, indicator ind, soci_l_or_ll_t & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<soci_l_or_ll_t>(in);
    }

    static void to_base(soci_l_or_ll_t const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};

template <>
struct type_conversion<soci_ul_or_ull_t>
{
    typedef soci_ul_or_ull_int_t base_type;

    static void from_base(base_type const & in, indicator ind, soci_ul_or_ull_t & out)
    {
        if (ind == i_null)
        {
            throw soci_error("Null value not allowed for this type.");
        }

        out = static_cast<soci_ul_or_ull_t>(in);
    }

    static void to_base(soci_ul_or_ull_t const & in, base_type & out, indicator & ind)
    {
        out = static_cast<base_type>(in);
        ind = i_ok;
    }
};

} // namespace soci

#endif // SOCI_FIXED_SIZE_INTS_H_INCLUDED
