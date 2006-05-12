//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-empty.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


// concrete factory for Empty concrete strategies
EmptySessionBackEnd * EmptyBackEndFactory::makeSession(
     std::string const &connectString) const
{
     return new EmptySessionBackEnd(connectString);
}

EmptyBackEndFactory const SOCI::empty;
