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


EmptyBLOBBackEnd::EmptyBLOBBackEnd(EmptySessionBackEnd &session)
    : session_(session)
{
    // ...
}

EmptyBLOBBackEnd::~EmptyBLOBBackEnd()
{
    // ...
}

std::size_t EmptyBLOBBackEnd::getLen()
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t EmptyBLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void EmptyBLOBBackEnd::trim(std::size_t /* newLen */)
{
    // ...
}
