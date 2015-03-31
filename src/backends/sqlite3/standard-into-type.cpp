//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci-platform.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "soci/rowid.h"
#include "common.h"
#include "soci/blob.h"
#include "soci-cstrtod.h"
#include "soci-exchange-cast.h"
// std
#include <cstdlib>
#include <ctime>
#include <string>

using namespace soci;
using namespace soci::details;
using namespace soci::details::sqlite3;

void sqlite3_standard_into_type_backend::define_by_pos(int & position, void * data,
                                                       exchange_type type)
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
                                               indicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    // sqlite columns start at 0
    int const pos = position_ - 1;

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

            *ind = i_null;
            return;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = i_ok;
            }
        }

        const char *buf = reinterpret_cast<const char*>(
            sqlite3_column_text(statement_.stmt_,pos));

        if (buf == NULL)
        {
            buf = "";
        }

        switch (type_)
        {
        case x_char:
            exchange_type_cast<x_char>(data_) = *buf;
            break;
        case x_stdstring:
            exchange_type_cast<x_stdstring>(data_) = buf;
            break;
        case x_short:
            {
                long val = std::strtol(buf, NULL, 10);
                exchange_type_cast<x_short>(data_) = static_cast<short>(val);
            }
            break;
        case x_integer:
            {
                long val = std::strtol(buf, NULL, 10);
                exchange_type_cast<x_integer>(data_) = static_cast<int>(val);
            }
            break;
        case x_long_long:
            exchange_type_cast<x_long_long>(data_) = std::strtoll(buf, NULL, 10);
            break;
        case x_unsigned_long_long:
            exchange_type_cast<x_unsigned_long_long>(data_) = string_to_unsigned_integer<unsigned long long>(buf);
            break;
        case x_double:
            exchange_type_cast<x_double>(data_) = cstring_to_double(buf);
            break;
        case x_stdtm:
            // attempt to parse the string and convert to std::tm
            parse_std_tm(buf, exchange_type_cast<x_stdtm>(data_));
            break;
        case x_rowid:
            {
                // RowID is internally identical to unsigned long

                rowid *rid = static_cast<rowid *>(data_);
                sqlite3_rowid_backend *rbe = static_cast<sqlite3_rowid_backend *>(rid->get_backend());
                long long val = std::strtoll(buf, NULL, 10);
                rbe->value_ = static_cast<unsigned long>(val);
            }
            break;
        case x_blob:
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
