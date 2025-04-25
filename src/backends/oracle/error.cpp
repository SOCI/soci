//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci/oracle/soci-oracle.h"
#include "error.h"
#include <limits>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::oracle;

oracle_soci_error::oracle_soci_error(std::string const & msg, int errNum)
    : soci_error(msg), err_num_(errNum)
{
}

soci_error::error_category oracle_soci_error::get_error_category() const
{
    if (err_num_ ==  3113 ||
        err_num_ ==  3114 ||
        err_num_ == 12162 ||
        err_num_ == 12541 ||
        err_num_ == 25403)
    {
        return connection_error;
    }

    if (err_num_ == 1031)
    {
        return no_privilege;
    }

    if (err_num_ == 1400)
    {
        return constraint_violation;
    }

    if (err_num_ == 1466 ||
        err_num_ == 2055 ||
        err_num_ == 2067 ||
        err_num_ == 2091 ||
        err_num_ == 2092 ||
        err_num_ == 25401 ||
        err_num_ == 25402 ||
        err_num_ == 25405 ||
        err_num_ == 25408 ||
        err_num_ == 25409)
    {
        return unknown_transaction_state;
    }

    return unknown;
}

void soci::details::oracle::get_error_details(sword res, OCIError *errhp,
    std::string &msg, int &errNum)
{
    text errbuf[512];
    sb4 errcode;
    errNum = 0;

    switch (res)
    {
    case OCI_NO_DATA:
        msg = "soci error: No data";
        break;
    case OCI_ERROR:
    case OCI_SUCCESS_WITH_INFO:
        OCIErrorGet(errhp, 1, 0, &errcode,
             errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
        msg = reinterpret_cast<char*>(errbuf);
        errNum = static_cast<int>(errcode);
        break;
    case OCI_INVALID_HANDLE:
        msg = "soci error: Invalid handle";
        break;
    default:
        msg = "soci error: Unknown error code";
    }
}

void soci::details::oracle::throw_oracle_soci_error(sword res, OCIError *errhp)
{
    std::string msg;
    int errNum;

    get_error_details(res, errhp, msg, errNum);
    throw oracle_soci_error(msg, errNum);
}
