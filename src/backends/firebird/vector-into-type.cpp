//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "common.h"

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Firebird;

void FirebirdVectorIntoTypeBackEnd::defineByPos(
    int & position, void * data, eExchangeType type)
{
    position_ = position-1;
    data_ = data;
    type_ = type;

    ++position;

    statement_.intoType_ = eVector;
    statement_.intos_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;
}

void FirebirdVectorIntoTypeBackEnd::preFetch()
{
    // Nothing to do here.
}

namespace // anonymous
{
    template <typename T>
    void setIntoVector(void *p, std::size_t indx, T const &val)
    {
        std::vector<T> *dest =
            static_cast<std::vector<T> *>(p);

        std::vector<T> &v = *dest;
        v[indx] = val;
    }
} // namespace anonymous

// this will exchange data with vector user buffers
void FirebirdVectorIntoTypeBackEnd::exchangeData(std::size_t row)
{
    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    switch (type_)
    {
            // simple cases
        case eXChar:
            setIntoVector(data_, row, getTextParam(var)[0]);
            break;
        case eXShort:
            {
                short tmp = from_isc<short>(var);
                setIntoVector(data_, row, tmp);
            }
            break;
        case eXInteger:
            {
                int tmp = from_isc<int>(var);
                setIntoVector(data_, row, tmp);
            }
            break;
        case eXUnsignedLong:
            {
                unsigned long tmp = from_isc<unsigned long>(var);
                setIntoVector(data_, row, tmp);
            }
            break;
        case eXDouble:
            {
                double tmp = from_isc<double>(var);
                setIntoVector(data_, row, tmp);
            }
            break;

            // cases that require adjustments and buffer management
        case eXStdString:
            setIntoVector(data_, row, getTextParam(var));
            break;
        case eXStdTm:
            {
                std::tm data;
                tmDecode(var->sqltype, buf_, &data);
                setIntoVector(data_, row, data);
            }
            break;

        default:
            throw SOCIError("Into vector element used with non-supported type.");
    } // switch

}

void FirebirdVectorIntoTypeBackEnd::postFetch(
    bool gotData, eIndicator * ind)
{
    // Here we have to set indicators only. Data was exchanged with user
    // buffers during fetch()
    if (gotData)
    {
        std::size_t rows = statement_.inds_[0].size();

        for (std::size_t i = 0; i<rows; ++i)
        {
            if (statement_.inds_[position_][i] == eNull && !ind)
            {
                throw SOCIError("Null value fetched and no indicator defined.");
            }
            else if (ind != NULL)
            {
                ind[i] = statement_.inds_[position_][i];
            }
        }
    }
}

void FirebirdVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    switch (type_)
    {
        case eXChar:
            resizeVector<char> (data_, sz);
            break;
        case eXShort:
            resizeVector<short> (data_, sz);
            break;
        case eXInteger:
            resizeVector<int> (data_, sz);
            break;
        case eXUnsignedLong:
            resizeVector<unsigned long>(data_, sz);
            break;
        case eXDouble:
            resizeVector<double> (data_, sz);
            break;
        case eXStdString:
            resizeVector<std::string> (data_, sz);
            break;
        case eXStdTm:
            resizeVector<std::tm> (data_, sz);
            break;

        default:
            throw SOCIError("Into vector element used with non-supported type.");
    }
}

std::size_t FirebirdVectorIntoTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
            // simple cases
        case eXChar:
            sz = getVectorSize<char> (data_);
            break;
        case eXShort:
            sz = getVectorSize<short> (data_);
            break;
        case eXInteger:
            sz = getVectorSize<int> (data_);
            break;
        case eXUnsignedLong:
            sz = getVectorSize<unsigned long>(data_);
            break;
        case eXDouble:
            sz = getVectorSize<double> (data_);
            break;
        case eXStdString:
            sz = getVectorSize<std::string> (data_);
            break;
        case eXStdTm:
            sz = getVectorSize<std::tm> (data_);
            break;

        default:
            throw SOCIError("Into vector element used with non-supported type.");
    }

    return sz;
}

void FirebirdVectorIntoTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete[] buf_;
        buf_ = NULL;
    }
}
