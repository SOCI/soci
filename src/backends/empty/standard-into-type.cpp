//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_EMPTY_SOURCE
#include "soci.h"
#include "soci-empty.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


void EmptyStandardIntoTypeBackEnd::defineByPos(
    int & /* position */, void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::preFetch()
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::postFetch(
    bool /* gotData */, bool /* calledFromFetch */, eIndicator * /* ind */)
{
    // ...
}

void EmptyStandardIntoTypeBackEnd::cleanUp()
{
    // ...
}
