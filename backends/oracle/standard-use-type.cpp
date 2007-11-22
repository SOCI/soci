//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define soci_ORACLE_SOURCE
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

void oracle_standard_use_type_backend::prepare_for_bind(
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
            details::cstring_descriptor *desc
                = static_cast<cstring_descriptor *>(data);
            oracleType = SQLT_STR;
            data = desc->str_;
            size = static_cast<sb4>(desc->bufSize_);
        }
        break;
    case eXStdString:
        oracleType = SQLT_STR;
        // 4000 is Oracle max VARCHAR2 size; 32768 is max LONG size
        size = 32769; 
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
}

void oracle_standard_use_type_backend::bind_by_pos(
    int &position, void *data, eExchangeType type, bool readOnly)
{
    if (statement_.boundByName_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepare_for_bind(data, size, oracleType);

    sword res = OCIBindByPos(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, statement_.session_.errhp_);
    }

    statement_.boundByPos_ = true;
}

void oracle_standard_use_type_backend::bind_by_name(
    std::string const &name, void *data, eExchangeType type, bool readOnly)
{
    if (statement_.boundByPos_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepare_for_bind(data, size, oracleType);

    sword res = OCIBindByName(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(name.c_str())),
        static_cast<sb4>(name.size()),
        data, size, oracleType,
        &indOCIHolder_, 0, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throw_oracle_soci_error(res, statement_.session_.errhp_);
    }

    statement_.boundByName_ = true;
}

void oracle_standard_use_type_backend::pre_use(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXStdString)
    {
        std::string *s = static_cast<std::string *>(data_);

        // 4000 is Oracle max VARCHAR2 size; 32768 is max LONG size 
        std::size_t const bufSize = 32769;
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
        statement *s = static_cast<statement *>(data_);

        s->undefine_and_bind();
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

void oracle_standard_use_type_backend::post_use(bool gotData, eIndicator *ind)
{
    // TODO: Is it possible to have the bound element being overwritten
    // by the database? (looks like yes)
    // If not, then nothing to do here, please remove this comment
    //         and most likely the code below is also unnecessary.
    // If yes, then use the value of the readOnly parameter:
    // - true:  the given object should not be modified and the backend
    //          should detect if the modification was performed on the
    //          isolated buffer and throw an exception if the buffer was modified
    //          (this indicates logic error, because the user used const object
    //          and executed a query that attempted to modified it)
    // - false: the modification should be propagated to the given object (as below).
    //
    // From the code below I conclude that ODBC allows the database to modify the bound object
    // and the code below correctly deals with readOnly == false.
    // The point is that with readOnly == true the propagation of modification should not
    // take place and in addition the attempt of modification should be detected and reported.
    // ...

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
            statement *s = static_cast<statement *>(data_);
            s->define_and_bind();
        }
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

        if (gotData == false)
        {
            // no data fetched and no indicator - programming error!
            throw soci_error("No data fetched and no indicator defined.");
        }
    }
}

void oracle_standard_use_type_backend::clean_up()
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
