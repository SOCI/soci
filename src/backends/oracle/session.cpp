//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci/oracle/soci-oracle.h"
#include "soci/callbacks.h"
#include "soci/session.h"
#include "error.h"
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

namespace // unnamed
{

sb4 fo_callback(void * /* svchp */, void * /* envhp */, void * fo_ctx,
    ub4 /* fo_type */, ub4 fo_event)
{
    oracle_session_backend * backend =
        static_cast<oracle_session_backend *>(fo_ctx);

    failover_callback * callback = backend->failoverCallback_;

    if (callback != NULL)
    {
        session * sql = backend->session_;

        switch (fo_event)
        {
        case OCI_FO_BEGIN:
            // failover operation was initiated

            try
            {
                callback->started();
            }
            catch (...)
            {
                // ignore exceptions from user callbacks
            }

            break;

        case OCI_FO_END:
            // failover was successful

            try
            {
                callback->finished(*sql);
            }
            catch (...)
            {
                // ignore exceptions from user callbacks
            }

            break;

        case OCI_FO_ABORT:
            // failover was aborted with no possibility to recovery

            try
            {
                callback->aborted();
            }
            catch (...)
            {
                // ignore exceptions from user callbacks
            }

            break;

        case OCI_FO_ERROR:
            // failover failed, but can be retried

            try
            {
                bool retry = false;
                std::string newTarget;
                callback->failed(retry, newTarget);

                // newTarget is ignored, as the new target
                // is selected by Oracle client configuration

                if (retry)
                {
                    return OCI_FO_RETRY;
                }
            }
            catch (...)
            {
                // ignore exceptions from user callbacks
            }

            break;

        case OCI_FO_REAUTH:
            // nothing interesting
            break;

        default:
            // ignore unknown callback types (if any)
            break;
        }
    }

    return 0;
}

} // unnamed namespace

oracle_session_backend::oracle_session_backend(std::string const & serviceName,
    std::string const & userName, std::string const & password, int mode,
    bool decimals_as_strings, int charset, int ncharset)
    : envhp_(NULL), srvhp_(NULL), errhp_(NULL), svchp_(NULL), usrhp_(NULL),
      decimals_as_strings_(decimals_as_strings)
{
    // assume service/user/password are utf8-compatible already
    const int defaultSourceCharSetId = 871;

    // maximum length of a connect descriptor is documented as 4KiB in the
    // "Syntax Rules for Configuration Files" section of Oracle documentation
    const size_t serviceBufLen = 4096;

    // user names must be identifiers which are limited to pitiful 30
    // characters in Oracle (see "Database Object Naming Rules" section of the
    // SQL language reference) and so, apparently, are the passwords, so just
    // 32 should be enough for them.
    const size_t authBufLen = 32;

    char nlsService[serviceBufLen];
    size_t nlsServiceLen;
    char nlsUserName[authBufLen];
    size_t nlsUserNameLen;
    char nlsPassword[authBufLen];
    size_t nlsPasswordLen;

    sword res;

    // create the environment
    res = OCIEnvNlsCreate(&envhp_, OCI_THREADED | OCI_ENV_NO_MUTEX,
        0, 0, 0, 0, 0, 0, charset, ncharset);
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

    if (charset != 0)
    {
        // convert service/user/password to the expected charset

        res = OCINlsCharSetConvert(envhp_, errhp_,
            charset, nlsService, serviceBufLen,
            defaultSourceCharSetId, serviceName.c_str(), serviceName.size(), &nlsServiceLen);
        if (res != OCI_SUCCESS)
        {
            std::string msg;
            int errNum;
            get_error_details(res, errhp_, msg, errNum);
            clean_up();
            throw oracle_soci_error(msg, errNum);
        }

        res = OCINlsCharSetConvert(envhp_, errhp_,
            charset, nlsUserName, authBufLen,
            defaultSourceCharSetId, userName.c_str(), userName.size(), &nlsUserNameLen);
        if (res != OCI_SUCCESS)
        {
            std::string msg;
            int errNum;
            get_error_details(res, errhp_, msg, errNum);
            clean_up();
            throw oracle_soci_error(msg, errNum);
        }

        res = OCINlsCharSetConvert(envhp_, errhp_,
            charset, nlsPassword, authBufLen,
            defaultSourceCharSetId, password.c_str(), password.size(), &nlsPasswordLen);
        if (res != OCI_SUCCESS)
        {
            std::string msg;
            int errNum;
            get_error_details(res, errhp_, msg, errNum);
            clean_up();
            throw oracle_soci_error(msg, errNum);
        }
    }
    else
    {
        // do not perform any charset conversions

        nlsServiceLen = serviceName.size();
        if (nlsServiceLen < serviceBufLen)
        {
            std::strcpy(nlsService, serviceName.c_str());
        }
        else
        {
            throw soci_error("Service name is too long.");
        }

        nlsUserNameLen = userName.size();
        if (nlsUserNameLen < authBufLen)
        {
            std::strcpy(nlsUserName, userName.c_str());
        }
        else
        {
            throw soci_error("User name is too long.");
        }

        nlsPasswordLen = password.size();
        if (nlsPasswordLen < authBufLen)
        {
            std::strcpy(nlsPassword, password.c_str());
        }
        else
        {
            throw soci_error("Password is too long.");
        }
    }

    // create the server context
    res = OCIServerAttach(srvhp_, errhp_,
        reinterpret_cast<text*>(nlsService),
        static_cast<sb4>(nlsServiceLen), OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        std::string msg;
        int errNum;
        get_error_details(res, errhp_, msg, errNum);
        clean_up();
        throw oracle_soci_error(msg, errNum);
    }

    // register failover callback
    OCIFocbkStruct fo;
    fo.fo_ctx = this;
    fo.callback_function = &fo_callback;

    res = OCIAttrSet(srvhp_, static_cast<ub4>(OCI_HTYPE_SERVER),
        &fo, 0, static_cast<ub4>(OCI_ATTR_FOCBK), errhp_);
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

    // select credentials type - use rdbms based credentials by default
    // and switch to external credentials if username and
    // password are both not specified
    ub4 credentialType = OCI_CRED_RDBMS;
    if (userName.empty() && password.empty())
    {
        credentialType = OCI_CRED_EXT;
    }
    else
    {
        // set username attribute in the user session handle
        res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
            reinterpret_cast<dvoid*>(nlsUserName),
            static_cast<ub4>(nlsUserNameLen), OCI_ATTR_USERNAME, errhp_);
        if (res != OCI_SUCCESS)
        {
            clean_up();
            throw soci_error("Cannot set username");
        }

        // set password attribute
        res = OCIAttrSet(usrhp_, OCI_HTYPE_SESSION,
            reinterpret_cast<dvoid*>(nlsPassword),
            static_cast<ub4>(nlsPasswordLen), OCI_ATTR_PASSWORD, errhp_);
        if (res != OCI_SUCCESS)
        {
            clean_up();
            throw soci_error("Cannot set password");
        }
    }

    // begin the session
    res = OCISessionBegin(svchp_, errhp_, usrhp_,
        credentialType, mode);
    if (res != OCI_SUCCESS && res != OCI_SUCCESS_WITH_INFO)
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

bool oracle_session_backend::is_connected()
{
    return OCIPing(svchp_, errhp_, OCI_DEFAULT) == OCI_SUCCESS;
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

bool oracle_session_backend::get_next_sequence_value(
    session &s, std::string const &sequence,
    long long &value)
{
    s << "select " + sequence + ".nextval from dual", into(value);

    return true;
}

ub2 oracle_session_backend::get_double_sql_type() const
{
    // SQLT_BDOUBLE avoids unnecessary conversions which is better from both
    // performance and correctness point of view as it avoids rounding
    // problems, however it's only available starting in Oracle 10.1, so
    // normally we should do run-time Oracle version detection here, but for
    // now just assume that if we use new headers (i.e. have high enough
    // compile-time version), then the run-time is at least as high.
#ifdef SQLT_BDOUBLE
    return SQLT_BDOUBLE;
#else
    return SQLT_FLT;
#endif
}
