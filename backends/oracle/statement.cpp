//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-oracle.h"
#include "error.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Oracle;

OracleStatementBackEnd::OracleStatementBackEnd(OracleSessionBackEnd &session)
    : session_(session), stmtp_(NULL)
{
}

void OracleStatementBackEnd::alloc()
{
    sword res = OCIHandleAlloc(session_.envhp_,
        reinterpret_cast<dvoid**>(&stmtp_),
        OCI_HTYPE_STMT, 0, 0);
    if (res != OCI_SUCCESS)
    {
        throw SOCIError("Cannot allocate statement handle");
    }
}

void OracleStatementBackEnd::cleanUp()
{
    // deallocate statement handle
    if (stmtp_ != NULL)
    {
        OCIHandleFree(stmtp_, OCI_HTYPE_STMT);
        stmtp_ = NULL;
    }
}

void OracleStatementBackEnd::prepare(std::string const &query)
{
    sb4 stmtLen = static_cast<sb4>(query.size());
    sword res = OCIStmtPrepare(stmtp_,
        session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(query.c_str())),
        stmtLen, OCI_V7_SYNTAX, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }
}

StatementBackEnd::execFetchResult OracleStatementBackEnd::execute(int number)
{
    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        static_cast<ub4>(number), 0, 0, 0, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        return eSuccess;
    }
    else if (res == OCI_NO_DATA)
    {
        return eNoData;
    }
    else
    {
        throwOracleSOCIError(res, session_.errhp_);
        return eNoData; // unreachable dummy return to please the compiler
    }
}

StatementBackEnd::execFetchResult OracleStatementBackEnd::fetch(int number)
{
    sword res = OCIStmtFetch(stmtp_, session_.errhp_,
        static_cast<ub4>(number), OCI_FETCH_NEXT, OCI_DEFAULT);

    if (res == OCI_SUCCESS || res == OCI_SUCCESS_WITH_INFO)
    {
        return eSuccess;
    }
    else if (res == OCI_NO_DATA)
    {
        return eNoData;
    }
    else
    {
        throwOracleSOCIError(res, session_.errhp_);
        return eNoData; // unreachable dummy return to please the compiler
    }
}

int OracleStatementBackEnd::getNumberOfRows()
{
    int rows;
    sword res = OCIAttrGet(static_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT), static_cast<dvoid*>(&rows),
        0, static_cast<ub4>(OCI_ATTR_ROWS_FETCHED), session_.errhp_);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return rows;
}

std::string OracleStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    std::string newQuery("begin ");
    newQuery += query;
    newQuery += "; end;";
    return newQuery;
}

int OracleStatementBackEnd::prepareForDescribe()
{
    sword res = OCIStmtExecute(session_.svchp_, stmtp_, session_.errhp_,
        1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    int cols;
    res = OCIAttrGet(static_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT), static_cast<dvoid*>(&cols),
        0, static_cast<ub4>(OCI_ATTR_PARAM_COUNT), session_.errhp_);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return cols;
}

void OracleStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName)
{
    int size;
    int precision;
    int scale;

    ub2 dbtype;
    text* dbname;
    ub4 nameLength;

    ub2 dbsize;
    sb2 dbprec;
    ub1 dbscale; //sb2 in some versions of Oracle?

    // Get the column handle
    OCIParam* colhd;
    sword res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_),
        static_cast<ub4>(OCI_HTYPE_STMT),
        reinterpret_cast<OCIError*>(session_.errhp_),
        reinterpret_cast<dvoid**>(&colhd),
        static_cast<ub4>(colNum));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // Get the column name
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid**>(&dbname),
        reinterpret_cast<ub4*>(&nameLength),
        static_cast<ub4>(OCI_ATTR_NAME),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // Get the column type
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbtype),
        0,
        static_cast<ub4>(OCI_ATTR_DATA_TYPE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // get the data size
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbsize),
        0,
        static_cast<ub4>(OCI_ATTR_DATA_SIZE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // get the precision
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbprec),
        0,
        static_cast<ub4>(OCI_ATTR_PRECISION),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // get the scale
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
        static_cast<ub4>(OCI_DTYPE_PARAM),
        reinterpret_cast<dvoid*>(&dbscale),
        0,
        static_cast<ub4>(OCI_ATTR_SCALE),
        reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    columnName.assign(dbname, dbname + nameLength);
    size = static_cast<int>(dbsize);
    precision = static_cast<int>(dbprec);
    scale = static_cast<int>(dbscale);

    switch (dbtype)
    {
    case SQLT_CHR:
    case SQLT_AFC:
        type = eString;
        break;
    case SQLT_NUM:
        if (scale > 0)
        {
            type = eDouble;
        }
        else if (precision < std::numeric_limits<int>::digits10)
        {
            type = eInteger;
        }
        else
        {
            type = eUnsignedLong;
        }
        break;
    case SQLT_DAT:
        type = eDate;
        break;
    }
}

std::size_t OracleStatementBackEnd::columnSize(int position)
{
    // Note: we may want to optimize so that the OCI_DESCRIBE_ONLY call
    // happens only once per statement.
    // Possibly use existing statement::describe() / make column prop
    // access lazy at same time

    int colSize(0);

    sword res = OCIStmtExecute(session_.svchp_, stmtp_,
         session_.errhp_, 1, 0, 0, 0, OCI_DESCRIBE_ONLY);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    // Get The Column Handle
    OCIParam* colhd;
    res = OCIParamGet(reinterpret_cast<dvoid*>(stmtp_),
         static_cast<ub4>(OCI_HTYPE_STMT),
         reinterpret_cast<OCIError*>(session_.errhp_),
         reinterpret_cast<dvoid**>(&colhd),
         static_cast<ub4>(position));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

     // Get The Data Size
    res = OCIAttrGet(reinterpret_cast<dvoid*>(colhd),
         static_cast<ub4>(OCI_DTYPE_PARAM),
         reinterpret_cast<dvoid*>(&colSize),
         0,
         static_cast<ub4>(OCI_ATTR_DATA_SIZE),
         reinterpret_cast<OCIError*>(session_.errhp_));
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, session_.errhp_);
    }

    return static_cast<std::size_t>(colSize);
}
