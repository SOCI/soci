//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include "error.h"
#include <connection-parameters.h>
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
using namespace soci::details::oracle;

// retrieves service name, user name and password from the
// uniform connect string
void chop_connect_string(std::string const & connectString,
    std::string & serviceName, std::string & userName,
    std::string & password, int & mode, bool & decimals_as_strings)
{
    // transform the connect string into a sequence of tokens
    // separated by spaces, this is done by replacing each first '='
    // in each original token with space
    // note: each original token is a key=value pair and only the first
    // '=' there is replaced with space, so that potential '=' signs
    // in the value part are left intact

    std::string tmp;
    bool in_value = false;
    for (std::string::const_iterator i = connectString.begin(),
             end = connectString.end(); i != end; ++i)
    {
        if (*i == '=' && in_value == false)
        {
            // this is the first '=' in the key=value pair
            tmp += ' ';
            in_value = true;
        }
        else
        {
            tmp += *i;
            if (*i == ' ' || *i == '\t')
            {
                // follow with the next key=value pair
                in_value = false;
            }
        }
    }

    serviceName.clear();
    userName.clear();
    password.clear();
    mode = OCI_DEFAULT;
    decimals_as_strings = false;

    std::istringstream iss(tmp);
    std::string key, value;
    while (iss >> key >> value)
    {
        if (key == "service")
        {
            serviceName = value;
        }
        else if (key == "user")
        {
            userName = value;
        }
        else if (key == "password")
        {
            password = value;
        }
        else if (key == "mode")
        {
            if (value == "sysdba")
            {
                mode = OCI_SYSDBA;
            }
            else if (value == "sysoper")
            {
                mode = OCI_SYSOPER;
            }
            else if (value == "default")
            {
                mode = OCI_DEFAULT;
            }
            else
            {
                throw soci_error("Invalid connection mode.");
            }
        }
        else if (key == "decimals_as_strings")
        {
            decimals_as_strings = value == "1" || value == "Y" || value == "y";
        }
    }
}


oracle_session_backend::oracle_session_backend()
    : envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL)
    , decimals_as_strings_(false)
{
}

bool oracle_session_backend::opened() const
{
    return (envhp_ != NULL);
}

void oracle_session_backend::open(
    connection_parameters const& parameters)
{
    std::string serviceName;
    std::string userName;
    std::string password;
    int mode = 0;

    chop_connect_string(parameters.get_connect_string(), serviceName, userName, password,
            mode, decimals_as_strings_);

    sword res;

    // create the environment
    res = OCIEnvCreate(&envhp_, OCI_THREADED | OCI_ENV_NO_MUTEX,
        0, 0, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw soci_error("Cannot create environment");
    }

    // create the server handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&srvhp_),
        OCI_HTYPE_SERVER, 0, 0);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot create server handle");
    }

    // create the error handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&errhp_),
        OCI_HTYPE_ERROR, 0, 0);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot create error handle");
    }

    // create the server context
    sb4 serviceNameLen = static_cast<sb4>(serviceName.size());
    res = OCIServerAttach(srvhp_, errhp_,
        reinterpret_cast<text*>(const_cast<char*>(serviceName.c_str())),
        serviceNameLen, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        get_error_details(res, errhp_, msg, errNum);
        clean_up();
        throw oracle_soci_error(msg, errNum);
    }

    // create service context handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&svchp_),
        OCI_HTYPE_SVCCTX, 0, 0);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot create service context");
    }

    // set the server attribute in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, srvhp_, 0,
        OCI_ATTR_SERVER, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        get_error_details(res, errhp_, msg, errNum);
        clean_up();
        throw oracle_soci_error(msg, errNum);
    }

    // allocate user session handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&usrhp_),
        OCI_HTYPE_SESSION, 0, 0);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot allocate user session handle");
    }

    // set username attribute in the user session handle
    sb4 userNameLen = static_cast<sb4>(userName.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(userName.c_str())),
        userNameLen, OCI_ATTR_USERNAME, errhp_);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot set username");
    }

    // set password attribute
    sb4 passwordLen = static_cast<sb4>(password.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(password.c_str())),
        passwordLen, OCI_ATTR_PASSWORD, errhp_);
    if (res != OCI_SUCCESS)
    {
        clean_up();
        throw soci_error("Cannot set password");
    }

    // begin the session
    res = OCISessionBegin(svchp_, errhp_, usrhp_,
        OCI_CRED_RDBMS, mode);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        get_error_details(res, errhp_, msg, errNum);
        clean_up();
        throw oracle_soci_error(msg, errNum);
    }

    // set the session in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, usrhp_,
        0, OCI_ATTR_SESSION, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        get_error_details(res, errhp_, msg, errNum);
        clean_up();
        throw oracle_soci_error(msg, errNum);
    }
}

oracle_session_backend::~oracle_session_backend()
{
    clean_up();
}

void oracle_session_backend::begin()
{
    // This code is commented out because it causes one of the transaction
    // tests in common_tests::test10() to fail with error 'Invalid handle'
    // With the code commented out, all tests pass.
    //    sword res = OCITransStart(svchp_, errhp_, 0, OCI_TRANS_NEW);
    //    if (res != OCI_SUCCESS)
    //    {
    //        throworacle_soci_error(res, errhp_);
    //    }
}

void oracle_session_backend::commit()
{
    sword res = OCITransCommit(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, errhp_);
    }
}

void oracle_session_backend::rollback()
{
    sword res = OCITransRollback(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, errhp_);
    }
}

void oracle_session_backend::clean_up()
{
    if (svchp_ != NULL && errhp_ != NULL && usrhp_ != NULL)
    {
        OCISessionEnd(svchp_, errhp_, usrhp_, OCI_DEFAULT);
    }

    if (usrhp_) { OCIHandleFree(usrhp_, OCI_HTYPE_SESSION); }
    if (svchp_) { OCIHandleFree(svchp_, OCI_HTYPE_SVCCTX);  }
    if (srvhp_)
    {
        OCIServerDetach(srvhp_, errhp_, OCI_DEFAULT);
        OCIHandleFree(srvhp_, OCI_HTYPE_SERVER);
    }
    if (errhp_) { OCIHandleFree(errhp_, OCI_HTYPE_ERROR); }
    if (envhp_) { OCIHandleFree(envhp_, OCI_HTYPE_ENV);   }

    envhp_ = NULL;
    srvhp_ = NULL;
    errhp_ = NULL;
    svchp_ = NULL;
    usrhp_ = NULL;
    decimals_as_strings_ = false;
}

oracle_statement_backend * oracle_session_backend::make_statement_backend()
{
    return new oracle_statement_backend(*this);
}

oracle_rowid_backend * oracle_session_backend::make_rowid_backend()
{
    return new oracle_rowid_backend(*this);
}

oracle_blob_backend * oracle_session_backend::make_blob_backend()
{
    return new oracle_blob_backend(*this);
}
