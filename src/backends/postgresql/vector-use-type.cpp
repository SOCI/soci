//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include "common.h"
#include <soci.h>
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <limits>
#include <sstream>

#ifdef SOCI_PGSQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_PGSQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355 4996)
#define snprintf _snprintf
#else
using std::snprintf;
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::PostgreSQL;


void PostgreSQLVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void PostgreSQLVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void PostgreSQLVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    std::size_t const vsize = size();
    for (size_t i = 0; i != vsize; ++i)
    {
        char *buf;

        // the data in vector can be either eOK or eNull
        if (ind != NULL && ind[i] == eNull)
        {
            buf = NULL;
        }
        else
        {
            // allocate and fill the buffer with text-formatted client data
            switch (type_)
            {
            case eXChar:
                {
                    std::vector<char> *pv
                        = static_cast<std::vector<char> *>(data_);
                    std::vector<char> &v = *pv;

                    buf = new char[2];
                    buf[0] = v[i];
                    buf[1] = '\0';
                }
                break;
            case eXStdString:
                {
                    std::vector<std::string> *pv
                        = static_cast<std::vector<std::string> *>(data_);
                    std::vector<std::string> &v = *pv;

                    buf = new char[v[i].size() + 1];
                    std::strcpy(buf, v[i].c_str());
                }
                break;
            case eXShort:
                {
                    std::vector<short> *pv
                        = static_cast<std::vector<short> *>(data_);
                    std::vector<short> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<short>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%d", static_cast<int>(v[i]));
                }
                break;
            case eXInteger:
                {
                    std::vector<int> *pv
                        = static_cast<std::vector<int> *>(data_);
                    std::vector<int> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int>::digits10 + 3;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%d", v[i]);
                }
                break;
            case eXUnsignedLong:
                {
                    std::vector<unsigned long> *pv
                        = static_cast<std::vector<unsigned long> *>(data_);
                    std::vector<unsigned long> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<unsigned long>::digits10 + 2;
                    buf = new char[bufSize];
                    snprintf(buf, bufSize, "%lu", v[i]);
                }
                break;
            case eXDouble:
                {
                    // no need to overengineer it (KISS)...

                    std::vector<double> *pv
                        = static_cast<std::vector<double> *>(data_);
                    std::vector<double> &v = *pv;

                    std::size_t const bufSize = 100;
                    buf = new char[bufSize];

                    snprintf(buf, bufSize, "%.20g", v[i]);
                }
                break;
            case eXStdTm:
                {
                    std::vector<std::tm> *pv
                        = static_cast<std::vector<std::tm> *>(data_);
                    std::vector<std::tm> &v = *pv;

                    std::size_t const bufSize = 20;
                    buf = new char[bufSize];

                    snprintf(buf, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                        v[i].tm_year + 1900, v[i].tm_mon + 1, v[i].tm_mday,
                        v[i].tm_hour, v[i].tm_min, v[i].tm_sec);
                }
                break;

            default:
                throw SOCIError(
                    "Use vector element used with non-supported type.");
            }
        }

        buffers_.push_back(buf);
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buffers_[0];
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buffers_[0];
    }
}

std::size_t PostgreSQLVectorUseTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:         sz = getVectorSize<char>         (data_); break;
    case eXShort:        sz = getVectorSize<short>        (data_); break;
    case eXInteger:      sz = getVectorSize<int>          (data_); break;
    case eXUnsignedLong: sz = getVectorSize<unsigned long>(data_); break;
    case eXDouble:       sz = getVectorSize<double>       (data_); break;
    case eXStdString:    sz = getVectorSize<std::string>  (data_); break;
    case eXStdTm:        sz = getVectorSize<std::tm>      (data_); break;

    default:
        throw SOCIError("Use vector element used with non-supported type.");
    }

    return sz;
}

void PostgreSQLVectorUseTypeBackEnd::cleanUp()
{
    std::size_t const bsize = buffers_.size();
    for (std::size_t i = 0; i != bsize; ++i)
    {
        delete [] buffers_[i];
    }
}
