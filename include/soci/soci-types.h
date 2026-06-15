#ifndef SOCI_TYPES_H_INCLUDED
#define SOCI_TYPES_H_INCLUDED

#include <cstdint>
#include <type_traits>

// In several places, we want to handle "long" or "long long" as the
// corresponding fixed size integer types (knowing that the other type is
// already handled by the specialization for either int32_t or int64_t).
//
// Define the type which is not handled by an existing specialization and the
// type as which it should be handled.
//
// Note that this assumes that int64_t is always either "long long" or "long",
// which is not, strictly speaking, guaranteed by the C++ standard, but is true
// for all known compilers and platforms (however note that some LP64 platforms
// still define int64_t as "long long" and not just "long", e.g. macOS does it).

// This is the "other" type, i.e. the one which is not the same as int64_t.
using soci_l_or_ll_t =
    std::conditional<std::is_same<long long, int64_t>::value,
                     long, long long>::type;

// And this is the corresponding intN_t type of the same size.
using soci_l_or_ll_int_t =
    std::conditional<sizeof(soci_l_or_ll_t) == 4,
                     int32_t, int64_t>::type;

// Same as above but for unsigned types.
using soci_ul_or_ull_t =
    std::conditional<std::is_same<unsigned long long, uint64_t>::value,
                     unsigned long, unsigned long long>::type;

using soci_ul_or_ull_int_t =
    std::conditional<sizeof(soci_ul_or_ull_t) == 4,
                     uint32_t, uint64_t>::type;

// We also specialize some templates for both char and int8_t, but they can be
// the same type, so in this case we define some other type to specialize the
// template for as we just can't do it for int8_t itself if it's the same as
// char, for which we always provide a specialization.
struct soci_int8_t_wrapper
{
    int8_t value = 0;
};

using soci_int8_t =
    std::conditional<std::is_same<int8_t, char>::value,
                     soci_int8_t_wrapper, int8_t>::type;

#endif // SOCI_TYPES_H_INCLUDED
