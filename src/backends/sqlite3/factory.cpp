//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include <sstream>
#include "soci-sqlite3.h"
#include <backend-loader.h>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

#include "../../../build/windows/MSVC_MEMORY_BEGIN.def"

using namespace soci;
using namespace soci::details;
using namespace sqlite_api;

// concrete factory for Empty concrete strategies
sqlite3_session_backend * sqlite3_backend_factory::make_session(
     connection_parameters const & parameters) const
{
     return new sqlite3_session_backend(parameters);
}

void error_msg(sqlite_api::sqlite3* db, const std::string& error)
{
    std::string zErrMsg = sqlite3_errmsg(db);
    std::ostringstream ss;
    ss << error << (zErrMsg == "not an error" ? "" : zErrMsg);
    sqlite3_close_v2(db);
    throw soci::soci_error(ss.str());
}

void sqlite3_backend_factory::create_database(const std::string& path) const
{
    sqlite_api::sqlite3* db=NULL;

    //check if database can be opened for read/write meaning databse already exists
    if( sqlite3_open_v2(path.c_str(), &db,SQLITE_OPEN_READWRITE, NULL) == SQLITE_OK )
        error_msg(db,"Database already exists.");
    //close db handle even if this code
    sqlite3_close_v2(db);db=NULL;
    if( sqlite3_open_v2(path.c_str(), &db,SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK )
        error_msg(db,"");
    //close database handle before exit
    sqlite3_close_v2(db);
}

sqlite3_backend_factory const soci::sqlite3;

extern "C"
{

// for dynamic backend loading
SOCI_SQLITE3_DECL backend_factory const * factory_sqlite3()
{
    return &soci::sqlite3;
}

SOCI_SQLITE3_DECL void register_factory_sqlite3()
{
    soci::dynamic_backends::register_backend("sqlite3", soci::sqlite3);
}

} // extern "C"
