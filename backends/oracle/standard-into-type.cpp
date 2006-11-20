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

OracleStandardIntoTypeBackEnd * OracleStatementBackEnd::makeIntoTypeBackEnd()
{
    return new OracleStandardIntoTypeBackEnd(*this);
}

OracleStandardUseTypeBackEnd * OracleStatementBackEnd::makeUseTypeBackEnd()
{
    return new OracleStandardUseTypeBackEnd(*this);
}

OracleVectorIntoTypeBackEnd *
OracleStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new OracleVectorIntoTypeBackEnd(*this);
}

OracleVectorUseTypeBackEnd *
OracleStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new OracleVectorUseTypeBackEnd(*this);
}

void OracleStandardIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType = 0; // dummy initialization to please the compiler
    sb4 size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case eXChar:
        oracleType = SQLT_AFC;
        size = sizeof(char);
        break;
    case eXShort:
        oracleType = SQLT_INT;
        size = sizeof(short);
        break;
    case eXInteger:
        oracleType = SQLT_INT;
        size = sizeof(int);
        break;
    case eXUnsignedLong:
        oracleType = SQLT_UIN;
        size = sizeof(unsigned long);
        break;
    case eXDouble:
        oracleType = SQLT_FLT;
        size = sizeof(double);
        break;

    // cases that require adjustments and buffer management
    case eXCString:
        {
            details::CStringDescriptor *desc
                = static_cast<CStringDescriptor *>(data);
            oracleType = SQLT_STR;
            data = desc->str_;
            size = static_cast<sb4>(desc->bufSize_);
        }
        break;
    case eXStdString:
        oracleType = SQLT_STR;
        size = 32769;  // support selecting strings from LONG columns
        buf_ = new char[size];
        data = buf_;
        break;
    case eXStdTm:
        oracleType = SQLT_DAT;
        size = 7 * sizeof(ub1);
        buf_ = new char[size];
        data = buf_;
        break;

    // cases that require special handling
    case eXStatement:
        {
            oracleType = SQLT_RSET;

            Statement *st = static_cast<Statement *>(data);
            st->alloc();

            OracleStatementBackEnd *stbe
                = static_cast<OracleStatementBackEnd *>(st->getBackEnd());
            size = 0;
            data = &stbe->stmtp_;
        }
        break;
    case eXRowID:
        {
            oracleType = SQLT_RDD;

            RowID *rid = static_cast<RowID *>(data);

            OracleRowIDBackEnd *rbe
                = static_cast<OracleRowIDBackEnd *>(rid->getBackEnd());

            size = 0;
            data = &rbe->rowidp_;
        }
        break;
    case eXBLOB:
        {
            oracleType = SQLT_BLOB;

            BLOB *b = static_cast<BLOB *>(data);

            OracleBLOBBackEnd *bbe
                = static_cast<OracleBLOBBackEnd *>(b->getBackEnd());

            size = 0;
            data = &bbe->lobp_;
        }
        break;
    }

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
            statement_.session_.errhp_,
            position++, data, size, oracleType,
            &indOCIHolder_, 0, &rCode_, OCI_DEFAULT);

    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardIntoTypeBackEnd::preFetch()
{
    // nothing to do except with Statement into objects

    if (type_ == eXStatement)
    {
        Statement *st = static_cast<Statement *>(data_);
        st->unDefAndBind();
    }
}

void OracleStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        // only std::string, std::tm and Statement need special handling
        if (type_ == eXStdString)
        {
            if (indOCIHolder_ != -1)
            { 
                std::string *s = static_cast<std::string *>(data_);
                *s = buf_;
            }
        }
        else if (type_ == eXStdTm)
        {
            if (indOCIHolder_ != -1)
            {
                std::tm *t = static_cast<std::tm *>(data_);

                ub1 *pos = reinterpret_cast<ub1*>(buf_);
                t->tm_isdst = -1;
                t->tm_year = (*pos++ - 100) * 100;
                t->tm_year += *pos++ - 2000;
                t->tm_mon = *pos++ - 1;
                t->tm_mday = *pos++;
                t->tm_hour = *pos++ - 1;
                t->tm_min = *pos++ - 1;
                t->tm_sec = *pos++ - 1;
                
                // normalize and compute the remaining fields
                std::mktime(t);
            }
        }
        else if (type_ == eXStatement)
        {
            Statement *st = static_cast<Statement *>(data_);
            st->defineAndBind();
        }
    }

    // then - deal with indicators
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }
    if (ind != NULL)
    {
        if (gotData == false)
        {
            *ind = eNoData;
        }
        else
        {
            if (indOCIHolder_ == 0)
            {
                *ind = eOK;
            }
            else if (indOCIHolder_ == -1)
            {
                *ind = eNull;
            }
            else
            {
                *ind = eTruncated;
            }
        }
    }
    else
    {
        if (indOCIHolder_ == -1)
        {
            // fetched null and no indicator - programming error!
            throw SOCIError("Null value fetched and no indicator defined.");
        }

        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void OracleStandardIntoTypeBackEnd::cleanUp()
{
    if (defnp_ != NULL)
    {
        OCIHandleFree(defnp_, OCI_HTYPE_DEFINE);
        defnp_ = NULL;
    }

    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}


