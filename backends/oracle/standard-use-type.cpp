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

void OracleStandardUseTypeBackEnd::prepareForBind(
    void *&data, sb4 &size, ub2 &oracleType)
{
    switch (type_)
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
        size = 4000;               // this is also Oracle limit
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
}

void OracleStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    sword res = OCIBindByPos(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    sword res = OCIBindByName(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(name.c_str())),
        static_cast<sb4>(name.size()),
        data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXStdString)
    {
        std::string *s = static_cast<std::string *>(data_);

        std::size_t const bufSize = 4000;
        std::size_t const sSize = s->size();
        std::size_t const toCopy =
            sSize < bufSize -1 ? sSize + 1 : bufSize - 1;
        strncpy(buf_, s->c_str(), toCopy);
        buf_[toCopy] = '\0';
    }
    else if (type_ == eXStdTm)
    {
        std::tm *t = static_cast<std::tm *>(data_);
        ub1* pos = reinterpret_cast<ub1*>(buf_);

        *pos++ = static_cast<ub1>(100 + (1900 + t->tm_year) / 100);
        *pos++ = static_cast<ub1>(100 + t->tm_year % 100);
        *pos++ = static_cast<ub1>(t->tm_mon + 1);
        *pos++ = static_cast<ub1>(t->tm_mday);
        *pos++ = static_cast<ub1>(t->tm_hour + 1);
        *pos++ = static_cast<ub1>(t->tm_min + 1);
        *pos = static_cast<ub1>(t->tm_sec + 1);
    }
    else if (type_ == eXStatement)
    {
        Statement *s = static_cast<Statement *>(data_);

        s->unDefAndBind();
    }

    // then handle indicators
    if (ind != NULL && *ind == eNull)
    {
        indOCIHolder_ = -1; // null
    }
    else
    {
        indOCIHolder_ = 0;  // value is OK
    }
}

void OracleStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // first, deal with data
    if (gotData)
    {
        if (type_ == eXStdString)
        {
            std::string *s = static_cast<std::string *>(data_);

            *s = buf_;
        }
        else if (type_ == eXStdTm)
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
        else if (type_ == eXStatement)
        {
            Statement *s = static_cast<Statement *>(data_);
            s->defineAndBind();
        }
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

void OracleStandardUseTypeBackEnd::cleanUp()
{
    if (bindp_ != NULL)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_DEFINE);
        bindp_ = NULL;
    }

    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
