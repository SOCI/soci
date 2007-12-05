//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci-sqlite3.h"
#include "rowid.h"
#include "common.h"
#include "blob.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#define strtoll(s, p, b) static_cast<long long>(_strtoi64(s, p, b))
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::sqlite3;

void sqlite3_standard_into_type_backend::define_by_pos(int & position, void * data,
                                                       eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void sqlite3_standard_into_type_backend::pre_fetch()
{
    // ...
}

void sqlite3_standard_into_type_backend::post_fetch(bool gotData,
                                               bool calledFromFetch,
                                               eIndicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    // sqlite columns start at 0
    int pos = position_ - 1;

    if (gotData)
    {
        // first, deal with indicators
        if (sqlite3_column_type(statement_.stmt_, pos) == SQLITE_NULL)
        {
            if (ind == NULL)
            {
                throw soci_error(
                    "Null value fetched and no indicator defined.");
            }

            *ind = eNull;
            return;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = eOK;
            }
        }

        const char *buf =
        reinterpret_cast<const char*>(sqlite3_column_text(
                                          statement_.stmt_,
                                          pos));

        if (!buf)
            buf = "";


        switch (type_)
        {
        case eXChar:
        {
            char *dest = static_cast<char*>(data_);
            *dest = *buf;
        }
        break;
        case eXCString:
        {
            cstring_descriptor *strDescr = static_cast<cstring_descriptor *>(data_);

            std::strncpy(strDescr->str_, buf, strDescr->bufSize_ - 1);
            strDescr->str_[strDescr->bufSize_ - 1] = '\0';

            if (std::strlen(buf) >= strDescr->bufSize_ && ind != NULL)
            {
                *ind = eTruncated;
            }
        }
        break;
        case eXStdString:
        {
            std::string *dest = static_cast<std::string *>(data_);
            dest->assign(buf);
        }
        break;
        case eXShort:
        {
            short *dest = static_cast<short*>(data_);
            long val = strtol(buf, NULL, 10);
            *dest = static_cast<short>(val);
        }
        break;
        case eXInteger:
        {
            int *dest = static_cast<int*>(data_);
            long val = strtol(buf, NULL, 10);
            *dest = static_cast<int>(val);
        }
        break;
        case eXUnsignedLong:
        {
            unsigned long *dest = static_cast<unsigned long *>(data_);
            long long val = strtoll(buf, NULL, 10);
            *dest = static_cast<unsigned long>(val);
        }
        break;
        case eXLongLong:
        {
            long long *dest = static_cast<long long *>(data_);
            *desc = strtoll(buf, NULL, 10);
        }
        break;
        case eXDouble:
        {
            double *dest = static_cast<double*>(data_);
            double val = strtod(buf, NULL);
            *dest = static_cast<double>(val);
        }
        break;
        case eXStdTm:
        {
            // attempt to parse the string and convert to std::tm
            std::tm *dest = static_cast<std::tm *>(data_);
            parseStdTm(buf, *dest);
        }
        break;
        case eXRowID:
        {
            // RowID is internally identical to unsigned long

            rowid *rid = static_cast<rowid *>(data_);
            sqlite3_rowid_backend *rbe = static_cast<sqlite3_rowid_backend *>(rid->get_backend());
            long long val = strtoll(buf, NULL, 10);
            rbe->value_ = static_cast<unsigned long>(val);
        }
        break;
        case eXBLOB:
        {
            blob *b = static_cast<blob *>(data_);
            sqlite3_blob_backend *bbe =
                static_cast<sqlite3_blob_backend *>(b->get_backend());

            buf = reinterpret_cast<const char*>(sqlite3_column_blob(
                                                   statement_.stmt_,
                                                   pos));

            int len = sqlite3_column_bytes(statement_.stmt_, pos);
            bbe->set_data(buf, len);
        }
        break;
        default:
            throw soci_error("Into element used with non-supported type.");
        }
    }
}

void sqlite3_standard_into_type_backend::clean_up()
{
    // ...
}
