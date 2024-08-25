//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci/postgresql/soci-postgresql.h"
#include "soci/blob.h"
#include "soci/rowid.h"
#include "soci/type-wrappers.h"
#include "soci/soci-platform.h"
#include "soci-dtocstr.h"
#include "soci-exchange-cast.h"
#include "soci-mktime.h"
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>

using namespace soci;
using namespace soci::details;

void postgresql_standard_use_type_backend::bind_by_pos(
    int & position, void * data, exchange_type type, bool /* readOnly */)
{
    // readOnly is ignored, because PostgreSQL does not support
    // any data to be written back to used (bound) objects.

    data_ = data;
    type_ = type;
    position_ = position++;
}

void postgresql_standard_use_type_backend::bind_by_name(
    std::string const & name, void * data, exchange_type type, bool /* readOnly */)
{
    // readOnly is ignored, because PostgreSQL does not support
    // any data to be written back to used (bound) objects.

    data_ = data;
    type_ = type;
    name_ = name;
}

void postgresql_standard_use_type_backend::pre_use(indicator const * ind)
{
    if (ind != NULL && *ind == i_null)
    {
        // leave the working buffer as NULL
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case x_char:
            {
                buf_ = new char[2];
                buf_[0] = exchange_type_cast<x_char>(data_);
                buf_[1] = '\0';
            }
            break;
        case x_stdstring:
            copy_from_string(exchange_type_cast<x_stdstring>(data_));
            break;
        case x_int8:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int8_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%d", exchange_type_cast<x_int8>(data_));
            }
            break;
        case x_uint8:
            {
                std::size_t const bufSize
                    = std::numeric_limits<uint8_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%u", exchange_type_cast<x_uint8>(data_));
            }
            break;
        case x_int16:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int16_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%d", exchange_type_cast<x_int16>(data_));
            }
            break;
        case x_uint16:
            {
                std::size_t const bufSize
                    = std::numeric_limits<uint16_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%u", exchange_type_cast<x_uint16>(data_));
            }
            break;
        case x_int32:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int32_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%d", exchange_type_cast<x_int32>(data_));
            }
            break;
        case x_uint32:
            {
                std::size_t const bufSize
                    = std::numeric_limits<uint32_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%u", exchange_type_cast<x_uint32>(data_));
            }
            break;
        case x_int64:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int64_t>::digits10 + 3;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%" LL_FMT_FLAGS "d",
                    static_cast<long long>(exchange_type_cast<x_int64>(data_)));
            }
            break;
        case x_uint64:
            {
                std::size_t const bufSize
                    = std::numeric_limits<uint64_t>::digits10 + 2;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%" LL_FMT_FLAGS "u",
                    static_cast<unsigned long long>(exchange_type_cast<x_uint64>(data_)));
            }
            break;
        case x_double:
            copy_from_string(double_to_cstring(exchange_type_cast<x_double>(data_)));
            break;
        case x_stdtm:
            {
                std::size_t const bufSize = 80;
                buf_ = new char[bufSize];

                format_std_tm(exchange_type_cast<x_stdtm>(data_), buf_, bufSize);
            }
            break;
        case x_rowid:
            {
                // RowID is internally identical to unsigned long

                rowid * rid = static_cast<rowid *>(data_);
                postgresql_rowid_backend * rbe
                    = static_cast<postgresql_rowid_backend *>(
                        rid->get_backend());

                std::size_t const bufSize
                    = std::numeric_limits<unsigned long>::digits10 + 2;
                buf_ = new char[bufSize];

                snprintf(buf_, bufSize, "%lu", rbe->value_);
            }
            break;
        case x_blob:
            {
                blob * b = static_cast<blob *>(data_);
                postgresql_blob_backend * bbe =
                    static_cast<postgresql_blob_backend *>(b->get_backend());

                // In case the backend does not reference a proper BLOB yet, this will create a
                // new, empty one
                bbe->init();

                unsigned long oid = bbe->get_blob_details().oid;

                if (oid == InvalidOid)
                {
                    throw soci_error("Cannot insert invalid BLOB.");
                }

                // In case the blob backend has created a new BLOB in the DB, we have to tell it
                // to not destroy it again, because we will now actually insert it into the DB.
                bbe->set_destroy_on_close(false);

                // We don't want any subsequent changes made to the blob object to leak to the
                // object that we are currently writing into the DB
                bbe->set_clone_before_modify(true);

                std::size_t const bufSize
                    = std::numeric_limits<unsigned long>::digits10 + 2;
                buf_ = new char[bufSize];
                snprintf(buf_, bufSize, "%lu", oid);
            }
            break;
        case x_xmltype:
            copy_from_string(exchange_type_cast<x_xmltype>(data_).value);
            break;
        case x_longstring:
            copy_from_string(exchange_type_cast<x_longstring>(data_).value);
            break;

        default:
            throw soci_error("Use element used with non-supported type.");
        }
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buf_;
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buf_;
    }
}

void postgresql_standard_use_type_backend::post_use(
    bool /* gotData */, indicator * /* ind */)
{
    // PostgreSQL does not support any data moving back the same channel,
    // so there is nothing to do here.
    // In particular, there is nothing to protect, because both const and non-const
    // objects will never be modified.

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    clean_up();
}

void postgresql_standard_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}

void postgresql_standard_use_type_backend::copy_from_string(std::string const& s)
{
    buf_ = new char[s.size() + 1];
    std::strcpy(buf_, s.c_str());
}
