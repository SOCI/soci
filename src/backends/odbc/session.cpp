//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <soci.h>

using namespace SOCI;
using namespace SOCI::details;

ODBCSessionBackEnd::ODBCSessionBackEnd(std::string const & connectString)
    : henv_(0), hdbc_(0)
{
    SQLRETURN rc;
    
	// Allocate environment handle
	rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);
    if (is_odbc_error(rc))
    {
        throw SOCIError("Unable to get environment handle");
    }

 	// Set the ODBC version environment attribute
 	rc = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_ENV, henv_, 
                         "Setting ODBC version");
    }

 	// Allocate connection handle
 	rc = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_, 
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
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_, 
                         "Error Connecting to database");
    }

    reset_transaction();
}

ODBCSessionBackEnd::~ODBCSessionBackEnd()
{
    cleanUp();
}

void ODBCSessionBackEnd::begin()
{
    SQLRETURN rc = SQLSetConnectAttr( hdbc_, SQL_ATTR_AUTOCOMMIT,
                    (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0 );
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                         "Begin Transaction");
    }    
}

void ODBCSessionBackEnd::commit()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_COMMIT);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                         "Commiting");
    }
    reset_transaction();
}

void ODBCSessionBackEnd::rollback()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_ROLLBACK);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                         "Rolling back");
    }    
    reset_transaction();
}

void ODBCSessionBackEnd::reset_transaction()
{
    SQLRETURN rc = SQLSetConnectAttr( hdbc_, SQL_ATTR_AUTOCOMMIT,
                    (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0 );
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                            "Set Auto Commit");
    }    
}


void ODBCSessionBackEnd::cleanUp()
{
    SQLRETURN rc = SQLDisconnect(hdbc_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                            "SQLDisconnect");
    }

    rc = SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, hdbc_,
                            "SQLFreeHandle DBC");
    }

    rc = SQLFreeHandle(SQL_HANDLE_ENV, henv_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_ENV, henv_, 
                            "SQLFreeHandle ENV");
    }
}

ODBCStatementBackEnd * ODBCSessionBackEnd::makeStatementBackEnd()
{
    return new ODBCStatementBackEnd(*this);
}

ODBCRowIDBackEnd * ODBCSessionBackEnd::makeRowIDBackEnd()
{
    return new ODBCRowIDBackEnd(*this);
}

ODBCBLOBBackEnd * ODBCSessionBackEnd::makeBLOBBackEnd()
{
    return new ODBCBLOBBackEnd(*this);
}
