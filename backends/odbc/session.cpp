//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"

using namespace soci;
using namespace soci::details;

odbc_session_backend::odbc_session_backend(std::string const & connectString)
    : henv_(0), hdbc_(0)
{
    SQLRETURN rc;

    // Allocate environment handle
    rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);
    if (is_odbc_error(rc))
    {
        throw soci_error("Unable to get environment handle");
    }

    // Set the ODBC version environment attribute
    rc = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_ENV, henv_,
                         "Setting ODBC version");
    }

    // Allocate connection handle
    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                         "Allocating connection handle");
    }

    SQLCHAR outConnString[1024];
    SQLSMALLINT strLength;

    rc = SQLDriverConnect(hdbc_, NULL, // windows handle
                          (SQLCHAR *)connectString.c_str(),
                          (SQLSMALLINT)connectString.size(),
                          outConnString, 1024,
                          &strLength, SQL_DRIVER_NOPROMPT);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                         "Error Connecting to database");
    }

    reset_transaction();
}

odbc_session_backend::~odbc_session_backend()
{
    clean_up();
}

void odbc_session_backend::begin()
{
    SQLRETURN rc = SQLSetConnectAttr( hdbc_, SQL_ATTR_AUTOCOMMIT,
                    (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0 );
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                         "Begin Transaction");
    }
}

void odbc_session_backend::commit()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_COMMIT);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                         "Commiting");
    }
    reset_transaction();
}

void odbc_session_backend::rollback()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_ROLLBACK);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                         "Rolling back");
    }
    reset_transaction();
}

void odbc_session_backend::reset_transaction()
{
    SQLRETURN rc = SQLSetConnectAttr( hdbc_, SQL_ATTR_AUTOCOMMIT,
                    (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0 );
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                            "Set Auto Commit");
    }
}


void odbc_session_backend::clean_up()
{
    SQLRETURN rc = SQLDisconnect(hdbc_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                            "SQLDisconnect");
    }

    rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, hdbc_,
                            "SQLFreeHandle DBC");
    }

    rc = SQLFreeHandle(SQL_HANDLE_ENV, henv_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_ENV, henv_,
                            "SQLFreeHandle ENV");
    }
}

odbc_statement_backend * odbc_session_backend::make_statement_backend()
{
    return new odbc_statement_backend(*this);
}

odbc_rowid_backend * odbc_session_backend::make_rowid_backend()
{
    return new odbc_rowid_backend(*this);
}

odbc_blob_backend * odbc_session_backend::make_blob_backend()
{
    return new odbc_blob_backend(*this);
}
