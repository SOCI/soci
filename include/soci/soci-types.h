#ifndef SOCI_TYPES_H_INCLUDED
#define SOCI_TYPES_H_INCLUDED

#include <cstdint>
#include <type_traits>

// Define typedefs for std::[u]intN_t type of the same size as [unsigned] long
// assuming it can only be 32 or 64 bits.
using soci_long_t = std::conditional<sizeof(long) == 8, int64_t, int32_t>::type;
using soci_ulong_t = std::conditional<sizeof(long) == 8, uint64_t, uint32_t>::type;

#endif // SOCI_TYPES_H_INCLUDED
