//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include "error.h"
#include <soci.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Oracle;

OracleSessionBackEnd::OracleSessionBackEnd(std::string const & serviceName,
    std::string const & userName, std::string const & password)
    : envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL)
{
    sword res;

    // create the environment
    res = OCIEnvCreate(&envhp_, OCI_DEFAULT, 0, 0, 0, 0, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot create environment");
    }

    // create the server handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&srvhp_),
        OCI_HTYPE_SERVER, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create server handle");
    }

    // create the error handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&errhp_),
        OCI_HTYPE_ERROR, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create error handle");
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
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // create service context handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&svchp_),
        OCI_HTYPE_SVCCTX, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot create service context");
    }

    // set the server attribute in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, srvhp_, 0,
        OCI_ATTR_SERVER, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // allocate user session handle
    res = OCIHandleAlloc(envhp_, reinterpret_cast<dvoid**>(&usrhp_),
        OCI_HTYPE_SESSION, 0, 0);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot allocate user session handle");
    }

    // set username attribute in the user session handle
    sb4 userNameLen = static_cast<sb4>(userName.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(userName.c_str())),
        userNameLen, OCI_ATTR_USERNAME, errhp_);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot set username");
    }

    // set password attribute
    sb4 passwordLen = static_cast<sb4>(password.size());
    res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
        reinterpret_cast<dvoid*>(const_cast<char*>(password.c_str())),
        passwordLen, OCI_ATTR_PASSWORD, errhp_);
    if (res != OCI_SUCCESS)
    {
        cleanUp();
        throw SOCIError("Cannot set password");
    }

    // begin the session
    res = OCISessionBegin(svchp_, errhp_, usrhp_,
        OCI_CRED_RDBMS, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }

    // set the session in the context handle
    res = OCIAttrSet(svchp_, OCI_HTYPE_SVCCTX, usrhp_,
        0, OCI_ATTR_SESSION, errhp_);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        getErrorDetails(res, errhp_, msg, errNum);
        cleanUp();
        throw OracleSOCIError(msg, errNum);
    }
}

OracleSessionBackEnd::~OracleSessionBackEnd()
{
    cleanUp();
}

void OracleSessionBackEnd::begin()
{
// This code is commented out because it causes one of the transaction
// tests in CommonTests::test10() to fail with error 'Invalid handle'
// With the code commented out, all tests pass.

//    sword res = OCITransStart(svchp_, errhp_, 0, OCI_TRANS_NEW);
//    if (res != OCI_SUCCESS)
//    {
//        throwOracleSOCIError(res, errhp_);
//    }
}

void OracleSessionBackEnd::commit()
{
    sword res = OCITransCommit(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, errhp_);
    }
}

void OracleSessionBackEnd::rollback()
{
    sword res = OCITransRollback(svchp_, errhp_, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, errhp_);
    }
}

void OracleSessionBackEnd::cleanUp()
{
    if (svchp_ != NULL && errhp_ != NULL && usrhp_ != NULL)
    {
        OCISessionEnd(svchp_, errhp_, usrhp_, OCI_DEFAULT);
    }

    if (usrhp_) OCIHandleFree(usrhp_, OCI_HTYPE_SESSION);
    if (svchp_) OCIHandleFree(svchp_, OCI_HTYPE_SVCCTX);
    if (srvhp_)
    {
        OCIServerDetach(srvhp_, errhp_, OCI_DEFAULT);
        OCIHandleFree(srvhp_, OCI_HTYPE_SERVER);
    }
    if (errhp_) OCIHandleFree(errhp_, OCI_HTYPE_ERROR);
    if (envhp_) OCIHandleFree(envhp_, OCI_HTYPE_ENV);
}

OracleStatementBackEnd * OracleSessionBackEnd::makeStatementBackEnd()
{
    return new OracleStatementBackEnd(*this);
}

OracleRowIDBackEnd * OracleSessionBackEnd::makeRowIDBackEnd()
{
    return new OracleRowIDBackEnd(*this);
}

OracleBLOBBackEnd * OracleSessionBackEnd::makeBLOBBackEnd()
{
    return new OracleBLOBBackEnd(*this);
}
