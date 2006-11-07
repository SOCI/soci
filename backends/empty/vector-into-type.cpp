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


void EmptyVectorIntoTypeBackEnd::defineByPos(
    int & /* position */, void * /* data */, eExchangeType /* type */)
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::preFetch()
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::postFetch(
    bool /* gotData */, eIndicator * /* ind */)
{
    // ...
}

void EmptyVectorIntoTypeBackEnd::resize(std::size_t /* sz */)
{
    // ...
}

std::size_t EmptyVectorIntoTypeBackEnd::size()
{
    // ...
    return 1;
}

void EmptyVectorIntoTypeBackEnd::cleanUp()
{
    // ...
}
