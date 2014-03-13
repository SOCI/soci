//
// Copyright (C) 2011-2013 Denis Chapligin
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_DB2_SOURCE
#include "soci/db2/soci-db2.h"
#include "soci/connection-parameters.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

std::string db2_soci_error::sqlState(std::string const & msg,const SQLSMALLINT htype,const SQLHANDLE hndl)
{
    std::ostringstream ss(msg, std::ostringstream::app);

    const char* socierror = NULL;

    SQLSMALLINT length, i = 1;
    switch ( SQLGetDiagRec(htype, hndl, i, sqlstate_, &sqlcode_,
                            message_, SQL_MAX_MESSAGE_LENGTH + 1,
                            &length) )
    {
      case SQL_SUCCESS:
        // The error message was successfully retrieved.
        break;

      case SQL_INVALID_HANDLE:
        socierror = "[SOCI]: Invalid handle.";
        break;

      case SQL_ERROR:
        socierror = "[SOCI]: SQLGetDiagRec() error.";
        break;

      case SQL_SUCCESS_WITH_INFO:
        socierror = "[SOCI]: Error message too long.";
        break;

      case SQL_NO_DATA:
        socierror = "[SOCI]: No error.";
        break;

      default:
        socierror = "[SOCI]: Unexpected SQLGetDiagRec() return value.";
        break;
    }

    if (socierror)
    {
        // Use our own error message if we failed to retrieve
        strcpy(reinterpret_cast<char*>(message_), socierror);

        // Use "General warning" SQLSTATE code
        strcpy(reinterpret_cast<char*>(sqlstate_), "01000");

        sqlcode_ = 0;
    }

    ss << ": " << message_ << " (" << sqlstate_ << ")";

    return ss.str();
}

void db2_session_backend::parseKeyVal(std::string const & keyVal) {
    size_t delimiter=keyVal.find_first_of("=");
    std::string key=keyVal.substr(0,delimiter);
    std::string value=keyVal.substr(delimiter+1,keyVal.length());

    if (!key.compare("DSN")) {
        this->dsn=value;
    }
    if (!key.compare("Uid")) {
        this->username=value;
    }
    if (!key.compare("Pwd")) {
        this->password=value;
    }
    this->autocommit=true; //Default value
    if (!key.compare("autocommit")) {
        if (!value.compare("off")) {
            this->autocommit=false;
	}
    }
}

/* DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;AutoCommit=off */
void db2_session_backend::parseConnectString(std::string const &  connectString) {
    std::string processingString(connectString);
    size_t delimiter=processingString.find_first_of(";");
    while(delimiter!=std::string::npos) {
        std::string keyVal=processingString.substr(0,delimiter);
        parseKeyVal(keyVal);
        processingString=processingString.erase(0,delimiter+1);
        delimiter=processingString.find_first_of(";");
    }
    if (!processingString.empty()) {
        parseKeyVal(processingString);
    }   
}

db2_session_backend::db2_session_backend(
    connection_parameters const & parameters) :
        in_transaction(false)
{
    parseConnectString(parameters.get_connect_string());
    SQLRETURN cliRC = SQL_SUCCESS;

    /* Prepare handles */
    cliRC = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv);
    if (cliRC != SQL_SUCCESS) {
        throw soci_error("Error while allocating the DB2 environment handle");
    }
    
    cliRC = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (cliRC != SQL_SUCCESS) {
        db2_soci_error e("Error while allocating the connection handle",cliRC,SQL_HANDLE_ENV,hEnv);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw e;
    }

    /* Set autocommit */
    if(this->autocommit) {
        cliRC = SQLSetConnectAttr(hDbc,SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_NTS);
    } else {
        cliRC = SQLSetConnectAttr(hDbc,SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_NTS);
    }
    if (cliRC != SQL_SUCCESS) {
        db2_soci_error e("Error while setting autocommit attribute",cliRC,SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw e;
    }

    /* Connect to database */
    cliRC = SQLConnect(hDbc, const_cast<SQLCHAR *>((const SQLCHAR *) dsn.c_str()), SQL_NTS,
        const_cast<SQLCHAR *>((const SQLCHAR *) username.c_str()), SQL_NTS,
        const_cast<SQLCHAR *>((const SQLCHAR *) password.c_str()), SQL_NTS);
    if (cliRC != SQL_SUCCESS) {
        db2_soci_error e("Error connecting to database",cliRC,SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw e;
    }
}

db2_session_backend::~db2_session_backend()
{
    clean_up();
}

void db2_session_backend::begin()
{
    // In DB2, transations begin implicitly; however, autocommit must be disabled for the duration of the transaction
    if(autocommit)
    {
        SQLRETURN cliRC = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_OFF, SQL_NTS);
        if (cliRC != SQL_SUCCESS && cliRC != SQL_SUCCESS_WITH_INFO)
        {
            db2_soci_error e("Clearing the autocommit attribute failed", cliRC, SQL_HANDLE_DBC, hDbc);
            SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
            throw e;
        }
    }

    in_transaction = true;
}

void db2_session_backend::commit()
{
    if (!autocommit || in_transaction) {
        in_transaction = false;
        SQLRETURN cliRC = SQLEndTran(SQL_HANDLE_DBC,hDbc,SQL_COMMIT);
        if(autocommit)
        {
            SQLRETURN cliRC2 = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, SQL_NTS);
            if ((cliRC == SQL_SUCCESS || cliRC == SQL_SUCCESS_WITH_INFO) &&
                cliRC2 != SQL_SUCCESS && cliRC2 != SQL_SUCCESS_WITH_INFO)
            {
                db2_soci_error e("Setting the autocommit attribute failed", cliRC, SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
                SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
                throw e;
            }
        }
        if (cliRC != SQL_SUCCESS && cliRC != SQL_SUCCESS_WITH_INFO) {
            throw db2_soci_error("Commit failed",cliRC, SQL_HANDLE_DBC, hDbc);
        }
    }
}

void db2_session_backend::rollback()
{
    if (!autocommit || in_transaction) {
        in_transaction = false;
        SQLRETURN cliRC = SQLEndTran(SQL_HANDLE_DBC,hDbc,SQL_ROLLBACK);
        if(autocommit)
        {
            SQLRETURN cliRC2 = SQLSetConnectAttr(hDbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, SQL_NTS);
            if ((cliRC == SQL_SUCCESS || cliRC == SQL_SUCCESS_WITH_INFO) &&
                cliRC2 != SQL_SUCCESS && cliRC2 != SQL_SUCCESS_WITH_INFO)
            {
                db2_soci_error e("Setting the autocommit attribute failed", cliRC, SQL_HANDLE_DBC, hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
                SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
                throw e;
            }
        }
        if (cliRC != SQL_SUCCESS && cliRC != SQL_SUCCESS_WITH_INFO) {
            throw db2_soci_error("Rollback failed", cliRC, SQL_HANDLE_DBC, hDbc);
        }
    }
}

void db2_session_backend::clean_up()
{
    // if a transaction is in progress, it will automatically be rolled back upon when the connection is disconnected/freed
    in_transaction = false;

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
