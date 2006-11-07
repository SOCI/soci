//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ORACLE_SOURCE
#include "soci-oracle.h"
#include "error.h"
#include <soci.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Oracle;

void OracleVectorIntoTypeBackEnd::prepareIndicators(std::size_t size)
{
    if (size == 0)
    {
         throw SOCIError("Vectors of size 0 are not allowed.");
    }

    indOCIHolderVec_.resize(size);
    indOCIHolders_ = &indOCIHolderVec_[0];

    sizes_.resize(size);
    rCodes_.resize(size);
}

void OracleVectorIntoTypeBackEnd::defineByPos(
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
        {
            oracleType = SQLT_AFC;
            size = sizeof(char);
            std::vector<char> *vp = static_cast<std::vector<char> *>(data);
            std::vector<char> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXShort:
        {
            oracleType = SQLT_INT;
            size = sizeof(short);
            std::vector<short> *vp = static_cast<std::vector<short> *>(data);
            std::vector<short> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXInteger:
        {
            oracleType = SQLT_INT;
            size = sizeof(int);
            std::vector<int> *vp = static_cast<std::vector<int> *>(data);
            std::vector<int> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;
    case eXUnsignedLong:
        {
            oracleType = SQLT_UIN;
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
            oracleType = SQLT_FLT;
            size = sizeof(double);
            std::vector<double> *vp = static_cast<std::vector<double> *>(data);
            std::vector<double> &v(*vp);
            prepareIndicators(v.size());
            data = &v[0];
        }
        break;

    // cases that require adjustments and buffer management

    case eXStdString:
        {
            oracleType = SQLT_CHR;
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data);
            colSize_ = statement_.columnSize(position) + 1;
            std::size_t bufSize = colSize_ * v->size();
            buf_ = new char[bufSize];

            prepareIndicators(v->size());

            size = static_cast<sb4>(colSize_);
            data = buf_;
        }
        break;
    case eXStdTm:
        {
            oracleType = SQLT_DAT;
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data);

            prepareIndicators(v->size());

            size = 7; // 7 is the size of SQLT_DAT
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

    sword res = OCIDefineByPos(statement_.stmtp_, &defnp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        indOCIHolders_, &sizes_[0], &rCodes_[0], OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorIntoTypeBackEnd::preFetch()
{
    // nothing to do for the supported types
}

void OracleVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // first, deal with data

        // only std::string, std::tm and Statement need special handling
        if (type_ == eXStdString)
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);

            std::vector<std::string> &v(*vp);

            char *pos = buf_;
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                if (indOCIHolderVec_[i] != -1)
                {
                    v[i].assign(pos, sizes_[i]);
                }
                pos += colSize_;
            }
        }
        else if (type_ == eXStdTm)
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);

            std::vector<std::tm> &v(*vp);

            ub1 *pos = reinterpret_cast<ub1*>(buf_);
            std::size_t const vsize = v.size();
            for (std::size_t i = 0; i != vsize; ++i)
            {
                if (indOCIHolderVec_[i] == -1)
                {
                     pos += 7; // size of SQLT_DAT
                }
                else
                {
                    std::tm t;
                    t.tm_isdst = -1;

                    t.tm_year = (*pos++ - 100) * 100;
                    t.tm_year += *pos++ - 2000;
                    t.tm_mon = *pos++ - 1;
                    t.tm_mday = *pos++;
                    t.tm_hour = *pos++ - 1;
                    t.tm_min = *pos++ - 1;
                    t.tm_sec = *pos++ - 1;

                    // normalize and compute the remaining fields
                    std::mktime(&t);
                    v[i] = t;
                }
            }
        }
        else if (type_ == eXStatement)
        {
            Statement *st = static_cast<Statement *>(data_);
            st->defineAndBind();
        }

        // then - deal with indicators
        if (ind != NULL)
        {
            std::size_t const indSize = indOCIHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == 0)
                {
                    ind[i] = eOK;
                }
                else if (indOCIHolderVec_[i] == -1)
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
            std::size_t const indSize = indOCIHolderVec_.size();
            for (std::size_t i = 0; i != indSize; ++i)
            {
                if (indOCIHolderVec_[i] == -1)
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

void OracleVectorIntoTypeBackEnd::resize(std::size_t sz)
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
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
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

std::size_t OracleVectorIntoTypeBackEnd::size()
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
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
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

void OracleVectorIntoTypeBackEnd::cleanUp()
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
