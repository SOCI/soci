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
    void *&data, sb4 &size, ub2 &oracleType, bool readOnly)
{
    readOnly_ = readOnly;

    switch (type_)
    {
    // simple cases
    case eXChar:
        oracleType = SQLT_AFC;
        size = sizeof(char);
        if (readOnly)
        {
            buf_ = new char[size];
            data = buf_;
        }
        break;
    case eXShort:
        oracleType = SQLT_INT;
        size = sizeof(short);
        if (readOnly)
        {
            buf_ = new char[size];
            data = buf_;
        }
        break;
    case eXInteger:
        oracleType = SQLT_INT;
        size = sizeof(int);
        if (readOnly)
        {
            buf_ = new char[size];
            data = buf_;
        }
        break;
    case eXUnsignedLong:
        oracleType = SQLT_UIN;
        size = sizeof(unsigned long);
        if (readOnly)
        {
            buf_ = new char[size];
            data = buf_;
        }
        break;
    case eXDouble:
        oracleType = SQLT_FLT;
        size = sizeof(double);
        if (readOnly)
        {
            buf_ = new char[size];
            data = buf_;
        }
        break;

    // cases that require adjustments and buffer management
    case eXCString:
        {
            details::cstring_descriptor *desc
                = static_cast<cstring_descriptor *>(data);
            oracleType = SQLT_STR;
            size = static_cast<sb4>(desc->bufSize_);

            if (readOnly_)
            {
                buf_ = new char[size];
                data = buf_;
            }
            else
            {
                data = desc->str_;
            }
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

    prepare_for_bind(data, size, oracleType, readOnly);

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

    prepare_for_bind(data, size, oracleType, readOnly);

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
    switch (type_)
    {
    case eXChar:
        if (readOnly_)
        {
            buf_[0] = *static_cast<char *>(data_);
        }
        break;
    case eXShort:
        if (readOnly_)
        {
            *static_cast<short *>(static_cast<void *>(buf_)) = *static_cast<short *>(data_);
        }
        break;
    case eXInteger:
        if (readOnly_)
        {
            *static_cast<int *>(static_cast<void *>(buf_)) = *static_cast<int *>(data_);
        }
        break;
    case eXUnsignedLong:
        if (readOnly_)
        {
            *static_cast<unsigned long *>(static_cast<void *>(buf_))
                = *static_cast<unsigned long *>(data_);
        }
        break;
    case eXDouble:
        if (readOnly_)
        {
            *static_cast<double *>(static_cast<void *>(buf_)) = *static_cast<double *>(data_);
        }
        break;
    case eXCString:
        if (readOnly_)
        {
            details::cstring_descriptor *desc
                = static_cast<cstring_descriptor *>(data_);

            strcpy(buf_, desc->str_);
        }
        break;
    case eXStdString:
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
        break;
    case eXStdTm:
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
        break;
    case eXStatement:
        {
            statement *s = static_cast<statement *>(data_);

            s->undefine_and_bind();
        }
        break;
    case eXRowID:
    case eXBLOB:
        // nothing to do
        break;
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
    // It is possible to have the bound element being overwritten
    // by the database.
    //
    // With readOnly_ == true the propagation of modification should *not*
    // take place and in addition the attempt of modification should be detected and reported.
    //
    // For simple (fundamental) data types there is nothing to do even if modifications
    // are allowed, because they were performed directly on the data provided by user code.

    // first, deal with data
    if (gotData)
    {
        switch (type_)
        {
        case eXChar:
            if (readOnly_)
            {
                const char original = *static_cast<char *>(data_);
                const char bound = buf_[0];

                if (original != bound)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXShort:
            if (readOnly_)
            {
                const short original = *static_cast<short *>(data_);
                const short bound = *static_cast<short *>(static_cast<void *>(buf_));

                if (original != bound)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXInteger:
            if (readOnly_)
            {
                const int original = *static_cast<int *>(data_);
                const int bound = *static_cast<int *>(static_cast<void *>(buf_));

                if (original != bound)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXUnsignedLong:
            if (readOnly_)
            {
                const unsigned long original = *static_cast<unsigned long *>(data_);
                const unsigned long bound
                    = *static_cast<unsigned long *>(static_cast<void *>(buf_));

                if (original != bound)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXDouble:
            if (readOnly_)
            {
                const double original = *static_cast<double *>(data_);
                const double bound = *static_cast<double *>(static_cast<void *>(buf_));

                if (original != bound)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXCString:
            if (readOnly_)
            {
                details::cstring_descriptor *original_descr
                    = static_cast<cstring_descriptor *>(data_);

                char * original = original_descr->str_;
                char * bound = buf_;

                if (strcmp(original, bound) != 0)
                {
                    throw soci_error("Attempted modification of const use element");
                }
            }
            break;
        case eXStdString:
            {
                std::string & original = *static_cast<std::string *>(data_);
                if (original != buf_)
                {
                    if (readOnly_)
                    {
                        throw soci_error("Attempted modification of const use element");
                    }
                    else
                    {
                        original = buf_;
                    }
                }
            }
            break;
        case eXStdTm:
            {
                std::tm & original = *static_cast<std::tm *>(data_);

                std::tm bound;
                ub1 *pos = reinterpret_cast<ub1*>(buf_);
                bound.tm_isdst = -1;
                bound.tm_year = (*pos++ - 100) * 100;
                bound.tm_year += *pos++ - 2000;
                bound.tm_mon = *pos++ - 1;
                bound.tm_mday = *pos++;
                bound.tm_hour = *pos++ - 1;
                bound.tm_min = *pos++ - 1;
                bound.tm_sec = *pos++ - 1;

                if (original.tm_year != bound.tm_year ||
                    original.tm_mon != bound.tm_mon ||
                    original.tm_mday != bound.tm_mday ||
                    original.tm_hour != bound.tm_hour ||
                    original.tm_min != bound.tm_min ||
                    original.tm_sec != bound.tm_sec)
                {
                    if (readOnly_)
                    {
                        throw soci_error("Attempted modification of const use element");
                    }
                    else
                    {
                        original = bound;

                        // normalize and compute the remaining fields
                        std::mktime(&original);
                    }
                }
            }
            break;
        case eXStatement:
            {
                statement *s = static_cast<statement *>(data_);
                s->define_and_bind();
            }
            break;
        case eXRowID:
        case eXBLOB:
            // nothing to do here
            break;
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
