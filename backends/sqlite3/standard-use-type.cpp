//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci-sqlite3.h"
#include "rowid.h"
#include "blob.h"
#include <limits>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355 4996)
#define snprintf _snprintf
#endif

using namespace soci;
using namespace soci::details;

void sqlite3_standard_use_type_backend::bind_by_pos(int & position, void * data,
    eExchangeType type, bool readOnly)
{
    if (statement_.boundByName_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data;
    type_ = type;
    position_ = position++;

    statement_.boundByPos_ = true;
}

void sqlite3_standard_use_type_backend::bind_by_name(std::string const & name,
    void * data, eExchangeType type, bool readOnly)
{
    if (statement_.boundByPos_)
    {
        throw soci_error(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data;
    type_ = type;
    name_ = ":" + name;

    statement_.resetIfNeeded();
    position_ = sqlite3_bind_parameter_index(statement_.stmt_, name_.c_str());

    if (0 == position_)
    {
        std::ostringstream ss;
        ss << "Cannot bind to (by name) " << name_;
        throw soci_error(ss.str());
    }
    statement_.boundByName_ = true;
}

void sqlite3_standard_use_type_backend::pre_use(eIndicator const * ind)
{
    statement_.useData_.resize(1);
    int pos = position_ - 1;

    if (statement_.useData_[0].size() < static_cast<std::size_t>(position_))
        statement_.useData_[0].resize(position_);

    if (ind != NULL && *ind == eNull)
    {
        statement_.useData_[0][pos].isNull_ = true;
        statement_.useData_[0][pos].data_ = "";
        statement_.useData_[0][pos].blobBuf_ = 0;
        statement_.useData_[0][pos].blobSize_ = 0;
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case eXChar:
        {
            buf_ = new char[2];
            buf_[0] = *static_cast<char*>(data_);
            buf_[1] = '\0';
        }
        break;
        case eXCString:
        {
            cstring_descriptor *strDescr = static_cast<cstring_descriptor *>(data_);

            std::size_t len = std::strlen(strDescr->str_);
            buf_ = new char[len + 1];
            std::strcpy(buf_, strDescr->str_);
        }
        break;
        case eXStdString:
        {
            std::string *s = static_cast<std::string *>(data_);
            buf_ = new char[s->size() + 1];
            std::strcpy(buf_, s->c_str());
        }
        break;
        case eXShort:
        {
            std::size_t const bufSize
            = std::numeric_limits<short>::digits10 + 3;
            buf_ = new char[bufSize];
            snprintf(buf_, bufSize, "%d",
                          static_cast<int>(*static_cast<short*>(data_)));
        }
        break;
        case eXInteger:
        {
            std::size_t const bufSize
            = std::numeric_limits<int>::digits10 + 3;
            buf_ = new char[bufSize];
            snprintf(buf_, bufSize, "%d",
                          *static_cast<int*>(data_));
        }
        break;
        case eXUnsignedLong:
        {
            std::size_t const bufSize
            = std::numeric_limits<unsigned long>::digits10 + 2;
            buf_ = new char[bufSize];
            snprintf(buf_, bufSize, "%lu",
                          *static_cast<unsigned long*>(data_));
        }
        break;
        case eXLongLong:
        {
            std::size_t const bufSize
            = std::numeric_limits<long long>::digits10 + 3;
            buf_ = new char[bufSize];
            snprintf(buf_, bufSize, "%lld",
                          *static_cast<long long *>(data_));
        }
        break;
        case eXDouble:
        {
            // no need to overengineer it (KISS)...

            std::size_t const bufSize = 100;
            buf_ = new char[bufSize];

            snprintf(buf_, bufSize, "%.20g",
                          *static_cast<double*>(data_));
        }
        break;
        case eXStdTm:
        {
            std::size_t const bufSize = 20;
            buf_ = new char[bufSize];

            std::tm *t = static_cast<std::tm *>(data_);
            snprintf(buf_, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                          t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                          t->tm_hour, t->tm_min, t->tm_sec);
        }
        break;
        case eXRowID:
        {
            // RowID is internally identical to unsigned long

            rowid *rid = static_cast<rowid *>(data_);
            sqlite3_rowid_backend *rbe = static_cast<sqlite3_rowid_backend *>(rid->get_backend());

            std::size_t const bufSize
            = std::numeric_limits<unsigned long>::digits10 + 2;
            buf_ = new char[bufSize];

            snprintf(buf_, bufSize, "%lu", rbe->value_);
        }
        break;
        case eXBLOB:
        {
            blob *b = static_cast<blob *>(data_);
            sqlite3_blob_backend *bbe =
                static_cast<sqlite3_blob_backend *>(b->get_backend());

            std::size_t len = bbe->get_len();
            buf_ = new char[len];
            bbe->read(0, buf_, len);
            statement_.useData_[0][pos].blobBuf_ = buf_;
            statement_.useData_[0][pos].blobSize_ = len;
        }
        break;

        default:
            throw soci_error("Use element used with non-supported type.");
        }

        statement_.useData_[0][pos].isNull_ = false;
        if (type_ != eXBLOB)
        {
            statement_.useData_[0][pos].blobBuf_ = 0;
            statement_.useData_[0][pos].blobSize_ = 0;
            statement_.useData_[0][pos].data_ = buf_;
        }
    }
}

void sqlite3_standard_use_type_backend::post_use(
    bool /* gotData */, eIndicator * /* ind */)
{
    // TODO: Is it possible to have the bound element being overwritten
    // by the database?
    // If not, then nothing to do here, please remove this comment.
    // If yes, then use the value of the readOnly parameter:
    // - true:  the given object should not be modified and the backend
    //          should detect if the modification was performed on the
    //          isolated buffer and throw an exception if the buffer was modified
    //          (this indicates logic error, because the user used const object
    //          and executed a query that attempted to modified it)
    // - false: the modification should be propagated to the given object.
    // ...

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    clean_up();
}

void sqlite3_standard_use_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
