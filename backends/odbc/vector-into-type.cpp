//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-odbc.h"
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>

using namespace SOCI;
using namespace SOCI::details;

void ODBCVectorIntoTypeBackEnd::prepareIndicators(std::size_t size)
{
    if (size == 0)
    {
         throw SOCIError("Vectors of size 0 are not allowed.");
    }

    indHolderVec_.resize(size);
    indHolders_ = &indHolderVec_[0];
}

void ODBCVectorIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    SQLINTEGER size = 0;       // also dummy

    switch (type)
    {
    // simple cases
    case eXShort:
        {
            odbcType_ = SQL_C_SSHORT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXInteger:
        {
            odbcType_ = SQL_C_SLONG;
            size = sizeof(long);
            std::vector<long> *vp = static_cast<std::vector<long> *>(data);
            std::vector<long> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXUnsignedLong:
        {
            odbcType_ = SQL_C_ULONG;
            size = sizeof(unsigned long);
            std::vector<unsigned long> *vp
                = static_cast<std::vector<unsigned long> *>(data);
            std::vector<unsigned long> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXDouble:
        {
            odbcType_ = SQL_C_DOUBLE;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case eXChar:
        {
            odbcType_ = SQL_C_CHAR;

            std::vector<char> *v
                = static_cast<std::vector<char> *>(data);

            prepareIndicators(v->size());

            size = sizeof(char) * 2;
            std::size_t bufSize = size * v->size();

            colSize_ = size;
            
            buf_ = new char[bufSize];
            data = buf_;
        }
        break;
    case eXStdString:
        {
            odbcType_ = SQL_C_CHAR;
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data);
            colSize_ = statement_.columnSize(position) + 1;
            std::size_t bufSize = colSize_ * v->size();
            buf_ = new char[bufSize];

            prepareIndicators(v->size());

            size = static_cast<SQLINTEGER>(colSize_);
            data = buf_;
        }
        break;
    case eXStdTm:
        {
            odbcType_ = SQL_C_TYPE_TIMESTAMP;
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data);

            prepareIndicators(v->size());

            size = sizeof(TIMESTAMP_STRUCT);
            colSize_ = size;
            
            std::size_t bufSize = size * v->size();

            buf_ = new char[bufSize];
            data = buf_;
        }
        break;

    case eXCString:   break; // not supported
                             // (there is no specialization
                             // of IntoType<vector<char*> >)
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }

    SQLRETURN rc = SQLBindCol(statement_.hstmt_, position++, odbcType_, data, size, indHolders_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, statement_.hstmt_, 
                            "vector into type define by pos");
    }    
}

void ODBCVectorIntoTypeBackEnd::preFetch()
{
    // nothing to do for the supported types
}

void ODBCVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // first, deal with data

        // only std::string, std::tm and Statement need special handling
        if (type_ == eXChar)
        {
            std::vector<char> *vp
                = static_cast<std::vector<char> *>(data_);

            std::vector<char> &v(*vp);
            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                v[i] = *pos;
                pos += colSize_;
            }
        }
        if (type_ == eXStdString)
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);

            std::vector<std::string> &v(*vp);

            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                v[i].assign(pos, strlen(pos));
                pos += colSize_;
            }
        }
        else if (type_ == eXStdTm)
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);

            std::vector<std::tm> &v(*vp);
            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                std::tm t;

                TIMESTAMP_STRUCT * ts = reinterpret_cast<TIMESTAMP_STRUCT*>(pos);
                t.tm_isdst = -1;
                t.tm_year = ts->year - 1900;
                t.tm_mon = ts->month - 1;
                t.tm_mday = ts->day;
                t.tm_hour = ts->hour;
                t.tm_min = ts->minute;
                t.tm_sec = ts->second;

                // normalize and compute the remaining fields
                std::mktime(&t);
                v[i] = t;
                pos += colSize_;
            }
        }

        // then - deal with indicators
        if (ind != NULL)
        {
            std::size_t const indSize = indHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indHolderVec_[i] > 0)
                {
                    ind[i] = eOK;
                }
                else if (indHolderVec_[i] == SQL_NULL_DATA)
                {
                    ind[i] = eNull;
                }
                else
                {
                    ind[i] = eTruncated;
                }
            }
        }
        else
        {
            std::size_t const indSize = indHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indHolderVec_[i] == SQL_NULL_DATA)
                {
                    // fetched null and no indicator - programming error!
                    throw SOCIError(
                        "Null value fetched and no indicator defined.");
                }
            }
        }
    }
    else // gotData == false
    {
        // nothing to do here, vectors are truncated anyway
    }
}

void ODBCVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            v->resize(sz);
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            v->resize(sz);
        }
        break;
    case eXInteger:
        {
            std::vector<long> *v = static_cast<std::vector<long> *>(data_);
            v->resize(sz);
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            v->resize(sz);
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            v->resize(sz);
        }
        break;

    case eXCString:   break; // not supported
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }
}

std::size_t ODBCVectorIntoTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            sz = v->size();
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            sz = v->size();
        }
        break;
    case eXInteger:
        {
            std::vector<long> *v = static_cast<std::vector<long> *>(data_);
            sz = v->size();
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            sz = v->size();
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            sz = v->size();
        }
        break;

    case eXCString:   break; // not supported
    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    }

    return sz;
}

void ODBCVectorIntoTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}
