//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

void Sqlite3StandardUseTypeBackEnd::bindByPos(int & position, void * data, 
                                              eExchangeType type)
{
    if (statement_.boundByName_)
    {
        throw SOCIError(
         "Binding for use elements must be either by position or by name.");
    }

    data_ = data;
    type_ = type;
    position_ = position++;

    statement_.boundByPos_ = true;
}

void Sqlite3StandardUseTypeBackEnd::bindByName(std::string const & name, 
                                               void * data,
                                               eExchangeType type)
{
    if (statement_.boundByPos_)
    {
        throw SOCIError(
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
        throw SOCIError(ss.str());
    }
    statement_.boundByName_ = true;
}

void Sqlite3StandardUseTypeBackEnd::preUse(eIndicator const * ind)
{
    statement_.useData_.resize(1);
    int pos = position_ - 1;

    if (statement_.useData_[0].size() < static_cast<std::size_t>(position_))
        statement_.useData_[0].resize(position_);

    if (ind != NULL && *ind == eNull)
    {
        statement_.useData_[0][pos].isNull_ = true;
        statement_.useData_[0][pos].data_ = "";
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
            CStringDescriptor *strDescr
            = static_cast<CStringDescriptor *>(data_);

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
            std::snprintf(buf_, bufSize, "%d",
                          static_cast<int>(*static_cast<short*>(data_)));
        }
        break;
        case eXInteger:
        {
            std::size_t const bufSize
            = std::numeric_limits<int>::digits10 + 3;
            buf_ = new char[bufSize];
            std::snprintf(buf_, bufSize, "%d",
                          *static_cast<int*>(data_));
        }
        break;
        case eXUnsignedLong:
        {
            std::size_t const bufSize
            = std::numeric_limits<unsigned long>::digits10 + 2;
            buf_ = new char[bufSize];
            std::snprintf(buf_, bufSize, "%lu",
                          *static_cast<unsigned long*>(data_));
        }
        break;
        case eXDouble:
        {
            // no need to overengineer it (KISS)...

            std::size_t const bufSize = 100;
            buf_ = new char[bufSize];

            std::snprintf(buf_, bufSize, "%.20g",
                          *static_cast<double*>(data_));
        }
        break;
        case eXStdTm:
        {
            std::size_t const bufSize = 20;
            buf_ = new char[bufSize];

            std::tm *t = static_cast<std::tm *>(data_);
            std::snprintf(buf_, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                          t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                          t->tm_hour, t->tm_min, t->tm_sec);
        }
        break;
        case eXRowID:
        {
            // RowID is internally identical to unsigned long

            RowID *rid = static_cast<RowID *>(data_);
            Sqlite3RowIDBackEnd *rbe
            = static_cast<Sqlite3RowIDBackEnd *>(
                rid->getBackEnd());

            std::size_t const bufSize
            = std::numeric_limits<unsigned long>::digits10 + 2;
            buf_ = new char[bufSize];

            std::snprintf(buf_, bufSize, "%lu", rbe->value_);
        }
        break;

        default:
            throw SOCIError("Use element used with non-supported type.");
        }

        statement_.useData_[0][pos].isNull_ = false;
        statement_.useData_[0][pos].data_ = buf_;
    }
}

void Sqlite3StandardUseTypeBackEnd::postUse(
    bool /* gotData */, eIndicator * /* ind */)
{
    // TODO: if sqlite3 allows to *get* data via this channel,
    // write it back to client buffers (variable)

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    cleanUp();
}

void Sqlite3StandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
