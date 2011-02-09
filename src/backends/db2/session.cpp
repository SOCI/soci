//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2011 Denis Chapligin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_DB2_SOURCE
#include "soci-db2.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


db2_session_backend::db2_session_backend(
    std::string const &  connectString /* DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;AutoCommit=off */)
{
    SQLRETURN cliRC = SQL_SUCCESS;

    /* Prepare handles */
    cliRC = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv);
    if (cliRC != SQL_SUCCESS) {
        throw db2_soci_error("Error while allocating the enironment handle",cliRC);
    }
    
    cliRC = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (cliRC != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error("Error while allocating the connection handle",cliRC);
    }

    /* Set autocommit */
    cliRC = SQLSetConnectAttr(hDbc,SQL_ATTR_AUTOCOMMIT, this->autocommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_NTS);
    if (cliRC != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error("Error while setting autocommit attribute",cliRC);
    }

    /* Connect to database */
    cliRC = SQLConnect(hDbc,(SQLCHAR*)connectString.c_str(),SQL_NTS,(SQLCHAR*)username.c_str(),SQL_NTS,(SQLCHAR*)password.c_str(),SQL_NTS);
    if (cliRC != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error("Error connecting to database",cliRC);
    }
}

db2_session_backend::~db2_session_backend()
{
    clean_up();
}

void db2_session_backend::begin()
{
    //Transations begin implicitly
}

void db2_session_backend::commit()
{
    if (!autocommit) {
        SQLRETURN cliRC = SQLEndTran(SQL_HANDLE_DBC,hDbc,SQL_COMMIT);
        if (cliRC != SQL_SUCCESS) {
            throw db2_soci_error("Commit failed",cliRC);
        }
    }
}

void db2_session_backend::rollback()
{
    if (!autocommit) {
        SQLRETURN cliRC = SQLEndTran(SQL_HANDLE_DBC,hDbc,SQL_ROLLBACK);
        if (cliRC != SQL_SUCCESS) {
            throw db2_soci_error("Rollback failed",cliRC);
        }
    }
}

void db2_session_backend::clean_up()
{
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
}

db2_statement_backend * db2_session_backend::make_statement_backend()
{
    return new db2_statement_backend(*this);
}

db2_rowid_backend * db2_session_backend::make_rowid_backend()
{
    return new db2_rowid_backend(*this);
}

db2_blob_backend * db2_session_backend::make_blob_backend()
{
    return new db2_blob_backend(*this);
}
