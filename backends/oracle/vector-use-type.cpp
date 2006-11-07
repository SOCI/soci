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

void OracleVectorUseTypeBackEnd::prepareIndicators(std::size_t size)
{
    if (size == 0)
    {
         throw SOCIError("Vectors of size 0 are not allowed.");
    }

    indOCIHolderVec_.resize(size);
    indOCIHolders_ = &indOCIHolderVec_[0];
}

void OracleVectorUseTypeBackEnd::prepareForBind(
    void *&data, sb4 &size, ub2 &oracleType)
{
    switch (type_)
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
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data);
            std::vector<std::string> &v(*vp);

            std::size_t maxSize = 0;
            std::size_t const vecSize = v.size();
            prepareIndicators(vecSize);
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                std::size_t sz = v[i].length();
                sizes_.push_back(static_cast<ub2>(sz));
                maxSize = sz > maxSize ? sz : maxSize;
            }

            buf_ = new char[maxSize * vecSize];
            char *pos = buf_;
            for (std::size_t i = 0; i != vecSize; ++i)
            {
                strncpy(pos, v[i].c_str(), v[i].length());
                pos += maxSize;
            }

            oracleType = SQLT_CHR;
            data = buf_;
            size = static_cast<sb4>(maxSize);
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data);

            prepareIndicators(vp->size());

            sb4 const dlen = 7; // size of SQLT_DAT
            buf_ = new char[dlen * vp->size()];

            oracleType = SQLT_DAT;
            data = buf_;
            size = dlen;
        }
        break;

    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    case eXCString:   break; // not supported
    }
}

void OracleVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    ub2 *sizesP = 0; // used only for std::string
    if (type == eXStdString)
    {
        sizesP = &sizes_[0];
    }

    sword res = OCIBindByPos(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        position++, data, size, oracleType,
        indOCIHolders_, sizesP, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data; // for future reference
    type_ = type; // for future reference

    ub2 oracleType;
    sb4 size;

    prepareForBind(data, size, oracleType);

    ub2 *sizesP = 0; // used only for std::string
    if (type == eXStdString)
    {
        sizesP = &sizes_[0];
    }

    sword res = OCIBindByName(statement_.stmtp_, &bindp_,
        statement_.session_.errhp_,
        reinterpret_cast<text*>(const_cast<char*>(name.c_str())),
        static_cast<sb4>(name.size()),
        data, size, oracleType,
        indOCIHolders_, sizesP, 0, 0, 0, OCI_DEFAULT);
    if (res != OCI_SUCCESS)
    {
        throwOracleSOCIError(res, statement_.session_.errhp_);
    }
}

void OracleVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // first deal with data
    if (type_ == eXStdString)
    {
        // nothing to do - it's already done during bind
        // (and it's probably impossible to separate them, because
        // changes in the string size could not be handled here)
    }
    else if (type_ == eXStdTm)
    {
        std::vector<std::tm> *vp
            = static_cast<std::vector<std::tm> *>(data_);
        std::vector<std::tm> &v(*vp);

        ub1* pos = reinterpret_cast<ub1*>(buf_);
        std::size_t const vsize = v.size();
        for (std::size_t i = 0; i != vsize; ++i)
        {
            *pos++ = static_cast<ub1>(100 + (1900 + v[i].tm_year) / 100);
            *pos++ = static_cast<ub1>(100 + v[i].tm_year % 100);
            *pos++ = static_cast<ub1>(v[i].tm_mon + 1);
            *pos++ = static_cast<ub1>(v[i].tm_mday);
            *pos++ = static_cast<ub1>(v[i].tm_hour + 1);
            *pos++ = static_cast<ub1>(v[i].tm_min + 1);
            *pos++ = static_cast<ub1>(v[i].tm_sec + 1);
        }
    }

    // then handle indicators
    if (ind != NULL)
    {
        std::size_t const vsize = size();
        for (std::size_t i = 0; i != vsize; ++i, ++ind)
        {
            if (*ind == eNull)
            {
                indOCIHolderVec_[i] = -1; // null
            }
            else
            {
                indOCIHolderVec_[i] = 0;  // value is OK
            }
        }
    }
    else
    {
        // no indicators - treat all fields as OK
        std::size_t const vsize = size();
        for (std::size_t i = 0; i != vsize; ++i, ++ind)
        {
            indOCIHolderVec_[i] = 0;  // value is OK
        }
    }
}

std::size_t OracleVectorUseTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *vp = static_cast<std::vector<char> *>(data_);
            sz = vp->size();
        }
        break;
    case eXShort:
        {
            std::vector<short> *vp = static_cast<std::vector<short> *>(data_);
            sz = vp->size();
        }
        break;
    case eXInteger:
        {
            std::vector<int> *vp = static_cast<std::vector<int> *>(data_);
            sz = vp->size();
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *vp
                = static_cast<std::vector<unsigned long> *>(data_);
            sz = vp->size();
        }
        break;
    case eXDouble:
        {
            std::vector<double> *vp
                = static_cast<std::vector<double> *>(data_);
            sz = vp->size();
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *vp
                = static_cast<std::vector<std::string> *>(data_);
            sz = vp->size();
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *vp
                = static_cast<std::vector<std::tm> *>(data_);
            sz = vp->size();
        }
        break;

    case eXStatement: break; // not supported
    case eXRowID:     break; // not supported
    case eXBLOB:      break; // not supported
    case eXCString:   break; // not supported
    }

    return sz;
}

void OracleVectorUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }

    if (bindp_ != NULL)
    {
        OCIHandleFree(bindp_, OCI_HTYPE_DEFINE);
        bindp_ = NULL;
    }
}
