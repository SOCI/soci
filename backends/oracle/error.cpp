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
#include <limits>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Oracle;


OracleSOCIError::OracleSOCIError(std::string const & msg, int errNum)
    : SOCIError(msg), errNum_(errNum)
{
}


void SOCI::details::Oracle::getErrorDetails(sword res, OCIError *errhp,
    std::string &msg, int &errNum)
{
    text errbuf[512];
    sb4 errcode;
    errNum = 0;

    switch (res)
    {
    case OCI_NO_DATA:
        msg = "SOCI error: No data";
        break;
    case OCI_ERROR:
        OCIErrorGet(errhp, 1, 0, &errcode,
             errbuf, sizeof(errbuf), OCI_HTYPE_ERROR);
        msg = reinterpret_cast<char*>(errbuf);
        errNum = static_cast<int>(errcode);
        break;
    case OCI_INVALID_HANDLE:
        msg = "SOCI error: Invalid handle";
        break;
    default:
        msg = "SOCI error: Unknown error code";
    }
}

void SOCI::details::Oracle::throwOracleSOCIError(sword res, OCIError *errhp)
{
    std::string msg;
    int errNum;

    getErrorDetails(res, errhp, msg, errNum);
    throw OracleSOCIError(msg, errNum);
}



