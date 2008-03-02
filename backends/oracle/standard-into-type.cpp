//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include "blob.h"
#include "error.h"
#include "rowid.h"
#include "statement.h"
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

oracle_standard_into_type_backend *
oracle_statement_backend::make_into_type_backend()
{
    return new oracle_standard_into_type_backend(*this);
}

oracle_standard_use_type_backend *
oracle_statement_backend::make_use_type_backend()
{
    return new oracle_standard_use_type_backend(*this);
}

oracle_vector_into_type_backend *
oracle_statement_backend::make_vector_into_type_backend()
{
    return new oracle_vector_into_type_backend(*this);
}

oracle_vector_use_type_backend *
oracle_statement_backend::make_vector_use_type_backend()
{
    return new oracle_vector_use_type_backend(*this);
}

void oracle_standard_into_type_backend::define_by_pos(
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
            details::cstring_descriptor *desc
                = static_cast<cstring_descriptor *>(data);
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

            statement *st = static_cast<statement *>(data);
            st->alloc();

            oracle_statement_backend *stbe
                = static_cast<oracle_statement_backend *>(st->get_backend());
            size = 0;
            data = &stbe->stmtp_;
        }
        break;
    case eXRowID:
        {
            oracleType = SQLT_RDD;

            rowid *rid = static_cast<rowid *>(data);

            oracle_rowid_backend *rbe
                = static_cast<oracle_rowid_backend *>(rid->get_backend());

            size = 0;
            data = &rbe->rowidp_;
        }
        break;
    case eXBLOB:
        {
            oracleType = SQLT_BLOB;

            blob *b = static_cast<blob *>(data);

            oracle_blob_backend *bbe
                = static_cast<oracle_blob_backend *>(b->get_backend());

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
        throw_oracle_soci_error(res, statement_.session_.errhp_);
    }
}

void oracle_standard_into_type_backend::pre_fetch()
{
    // nothing to do except with Statement into objects

    if (type_ == eXStatement)
    {
        statement *st = static_cast<statement *>(data_);
        st->undefine_and_bind();
    }
}

void oracle_standard_into_type_backend::post_fetch(
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
            statement *st = static_cast<statement *>(data_);
            st->define_and_bind();
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
        if (gotData)
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
            throw soci_error("Null value fetched and no indicator defined.");
        }
    }
}

void oracle_standard_into_type_backend::clean_up()
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
