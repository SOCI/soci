//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include <connection-parameters.h>
#include <backend-loader.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

// concrete factory for Empty concrete strategies
oracle_session_backend * oracle_backend_factory::make_session() const
{
    return new oracle_session_backend();
}

oracle_backend_factory const soci::oracle;

extern "C"
{

// for dynamic backend loading
SOCI_ORACLE_DECL backend_factory const * factory_oracle()
{
    return &soci::oracle;
}

SOCI_ORACLE_DECL void register_factory_oracle()
{
    soci::dynamic_backends::register_backend("oracle", soci::oracle);
}

} // extern "C"
