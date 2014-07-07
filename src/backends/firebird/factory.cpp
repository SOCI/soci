//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE

#include <sstream>

#include "soci-firebird.h"
#include <connection-parameters.h>
#include <backend-loader.h>

using namespace soci;

firebird_session_backend * firebird_backend_factory::make_session(
    connection_parameters const & parameters) const
{
    return new firebird_session_backend(parameters);
}

void firebird_backend_factory::create_database(const std::string& createSql) const
{
    isc_db_handle   newdb = NULL;          /* database handle */
    isc_tr_handle   trans = NULL;          /* transaction handle */
    ISC_STATUS      status[20];            /* status vector */
    long            sqlcode;               /* SQLCODE  */
    if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, createSql.c_str(),3,NULL)) //error
    {
        /* Extract SQLCODE from the status vector. */
        sqlcode = isc_sqlcode(status);
        ISC_SCHAR buf[512];
        std::ostringstream oss;
        const ISC_STATUS* start = status;
        while( fb_interpret(buf,sizeof(buf),&start) )
            oss << buf << std::endl;
        throw soci_error(oss.str());
    }
    isc_commit_transaction(status, &trans);
    isc_detach_database(status, &newdb);
}

firebird_backend_factory const soci::firebird;

extern "C"
{

// for dynamic backend loading
SOCI_FIREBIRD_DECL backend_factory const * factory_firebird()
{
    return &soci::firebird;
}

SOCI_FIREBIRD_DECL void register_factory_firebird()
{
    soci::dynamic_backends::register_backend("firebird", soci::firebird);
}

} // extern "C"
