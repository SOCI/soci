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


EmptySessionBackEnd::EmptySessionBackEnd(
    std::string const & /* connectString */)
{
    // ...
}

EmptySessionBackEnd::~EmptySessionBackEnd()
{
    cleanUp();
}

void EmptySessionBackEnd::begin()
{
    // ...
}

void EmptySessionBackEnd::commit()
{
    // ...
}

void EmptySessionBackEnd::rollback()
{
    // ...
}

void EmptySessionBackEnd::cleanUp()
{
    // ...
}

EmptyStatementBackEnd * EmptySessionBackEnd::makeStatementBackEnd()
{
    return new EmptyStatementBackEnd(*this);
}

EmptyRowIDBackEnd * EmptySessionBackEnd::makeRowIDBackEnd()
{
    return new EmptyRowIDBackEnd(*this);
}

EmptyBLOBBackEnd * EmptySessionBackEnd::makeBLOBBackEnd()
{
    return new EmptyBLOBBackEnd(*this);
}
