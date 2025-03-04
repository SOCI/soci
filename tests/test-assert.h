//
// Copyright (C) 2024 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_ASSERT_H_INCLUDED
#define SOCI_TEST_ASSERT_H_INCLUDED

#include "soci-compiler.h"

#include <cmath>
#include <iomanip>

#include "test-context.h"

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

// Compare doubles for approximate equality. This has to be used everywhere
// where we write "3.14" (or "6.28") to the database as a string and then
// compare the value read back with the literal 3.14 floating point constant
// because they are not the same.
//
// It is also used for the backends which currently don't handle doubles
// correctly.
//
// Notice that this function is normally not used directly but rather from the
// macro below.
inline bool are_doubles_approx_equal(double const a, double const b)
{
    // The formula taken from CATCH test framework
    // https://github.com/philsquared/Catch/
    // Thanks to Richard Harris for his help refining this formula
    double const epsilon(std::numeric_limits<float>::epsilon() * 100);
    double const scale(1.0);
    return std::fabs(a - b) < epsilon * (scale + (std::max)(std::fabs(a), std::fabs(b)));
}

// Compare two floating point numbers either exactly or approximately depending
// on test_context::has_fp_bug() return value.
inline bool
are_doubles_equal(test_context_base const& tc, double a, double b)
{
    return tc.has_fp_bug()
                ? are_doubles_approx_equal(a, b)
                : are_doubles_exactly_equal(a, b);
}

} // namespace tests

} // namespace soci

// Define macros for comparing floating point numbers.
//
// All of them can only be used inside CATCH test cases.

// This is a macro to ensure we use the correct line numbers. The weird
// do/while construction is used to make this a statement and the even weirder
// condition in while ensures that the loop is executed exactly once without
// triggering warnings from MSVC about the condition being always false.
#define ASSERT_EQUAL_APPROX(a, b) \
    do { \
      if (!are_doubles_approx_equal((a), (b))) { \
        FAIL( "Approximate equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )


#define ASSERT_EQUAL_EXACT(a, b) \
    do { \
      if (!are_doubles_exactly_equal((a), (b))) { \
        FAIL( "Exact equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )

// This macro should be used when where we don't have any problems with string
// literals vs floating point literals mismatches described above and would
// ideally compare the numbers exactly but, unfortunately, currently can't do
// this unconditionally because at least some backends are currently buggy and
// don't handle the floating point values correctly.
//
// This can be only used from inside the common_tests class as it relies on
// having an accessible "tc_" variable to determine whether exact or
// approximate comparison should be used.
#define ASSERT_EQUAL(a, b) \
    do { \
      if (!are_doubles_equal(tc_, (a), (b))) { \
        FAIL( "Equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )


#endif // SOCI_TEST_ASSERT_H_INCLUDED
