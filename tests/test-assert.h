//
// Copyright (C) 2024 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_ASSERT_H_INCLUDED
#define SOCI_TEST_ASSERT_H_INCLUDED

#include "soci-compiler.h"

namespace soci
{

namespace tests
{

// Exact double comparison function. We need one, instead of writing "a == b",
// only in order to have some place to put the pragmas disabling gcc warnings.
inline bool
are_doubles_exactly_equal(double a, double b)
{
    // Avoid g++ warnings: we do really want the exact equality here.
    SOCI_GCC_WARNING_SUPPRESS(float-equal)

    return a == b;

    SOCI_GCC_WARNING_RESTORE(float-equal)
}

} // namespace tests

} // namespace soci

// This macro can be only used inside CATCH tests.
#define ASSERT_EQUAL_EXACT(a, b) \
    do { \
      if (!are_doubles_exactly_equal((a), (b))) { \
        FAIL( "Exact equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )

#endif // SOCI_TEST_ASSERT_H_INCLUDED
