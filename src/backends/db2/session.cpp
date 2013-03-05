//
// Copyright (C) 2011-2013 Denis Chapligin
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
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

const std::string db2_soci_error::sqlState(std::string const & msg,const SQLSMALLINT htype,const SQLHANDLE hndl) {
        std::stringstream ss(msg);


    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode;
    SQLSMALLINT length;

    if ( SQLGetDiagRec(htype,
                       hndl,
                       1,
                       sqlstate,
                       &sqlcode,
                       message,
                       SQL_MAX_MESSAGE_LENGTH + 1,
                       &length) == SQL_SUCCESS ) {
        ss<<" SQLMESSAGE: ";
        ss<<message;
    }
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
    std::string const &  connectString /* DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;AutoCommit=off */)
{
    parseConnectString(connectString);
    SQLRETURN cliRC = SQL_SUCCESS;

    /* Prepare handles */
    cliRC = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv);
    if (cliRC != SQL_SUCCESS) {
        throw db2_soci_error("Error while allocating the enironment handle",cliRC);
    }
    
    cliRC = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (cliRC != SQL_SUCCESS) {
        std::string msg=db2_soci_error::sqlState("Error while allocating the connection handle",SQL_HANDLE_ENV,hEnv);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error(msg,cliRC);
    }

    /* Set autocommit */
    if(this->autocommit) {
        cliRC = SQLSetConnectAttr(hDbc,SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_NTS);
    } else {
        cliRC = SQLSetConnectAttr(hDbc,SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_NTS);
    }
    if (cliRC != SQL_SUCCESS) {
        std::string msg=db2_soci_error::sqlState("Error while setting autocommit attribute",SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error(msg,cliRC);
    }

    /* Connect to database */
    cliRC = SQLConnect(hDbc,(SQLCHAR*)dsn.c_str(),SQL_NTS,(SQLCHAR*)username.c_str(),SQL_NTS,(SQLCHAR*)password.c_str(),SQL_NTS);
    if (cliRC != SQL_SUCCESS) {
        std::string msg=db2_soci_error::sqlState("Error connecting to database",SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC,hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV,hEnv);
        throw db2_soci_error(msg,cliRC);
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
