//
// Copyright (C) 2014 Vadim Zeitlin.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_PRIVATE_SOCI_CSTRTOD_H_INCLUDED
#define SOCI_PRIVATE_SOCI_CSTRTOD_H_INCLUDED

#include "error.h"

#include <locale>
#include <sstream>

namespace soci
{

namespace details
{

// Locale-independent, i.e. always using "C" locale, function for converting
// strings to numbers.
//
// The string must contain a floating point number in "C" locale, i.e. using
// point as decimal separator, and nothing but it. If it does, the converted
// number is returned, otherwise an exception is thrown.
inline
double cstring_to_double(std::string const & str)
{
    using namespace std;

    double d;
    istringstream is(str);
    is.imbue(locale::classic());
    is >> d;

    if (!is || !is.eof())
    {
      throw soci_error(string("Cannot convert data: string \"") + str + "\" "
                       "is not a number.");
    }

    return d;
}

} // namespace details

} // namespace soci

#endif // SOCI_PRIVATE_SOCI_CSTRTOD_H_INCLUDED
