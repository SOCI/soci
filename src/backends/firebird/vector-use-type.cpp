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

void FirebirdVectorUseTypeBackEnd::bindByPos(int & position,
        void * data, eExchangeType type)
{
    if (statement_.boundByName_)
    {
        throw SOCIError(
         "Binding for use elements must be either by position or by name.");
    }

	position_ = position-1;
    data_ = data;
    type_ = type;

    ++position;

    statement_.useType_ = eVector;
    statement_.uses_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqlda2p_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;

	statement_.boundByPos_ = true;
}

void FirebirdVectorUseTypeBackEnd::bindByName(
    std::string const & name, void * data, eExchangeType type)
{
    if (statement_.boundByPos_)
    {
        throw SOCIError(
         "Binding for use elements must be either by position or by name.");
    }

	std::map <std::string, int> :: iterator idx =
        statement_.names_.find(name);

    if (idx == statement_.names_.end())
    {
        throw SOCIError("Missing use element for bind by name (" + name + ")");
    }

    position_ = idx->second;
    data_ = data;
    type_ = type;

    statement_.useType_ = eVector;
    statement_.uses_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqlda2p_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;

	statement_.boundByName_ = true;
}

void FirebirdVectorUseTypeBackEnd::preUse(eIndicator const * ind)
{
    inds_ = ind;
}

namespace
{
    template <typename T>
    T* getUseVectorValue(void *v, std::size_t index)
    {
        std::vector<T> *src =
            static_cast<std::vector<T> *>(v);

        std::vector<T> &v_ = *src;
        return &(v_[index]);
    }
}

void FirebirdVectorUseTypeBackEnd::exchangeData(std::size_t row)
{
    // first prepare indicators
    if (inds_ != NULL)
    {
        switch (inds_[row])
        {
            case eNull:
                indISCHolder_ = -1;
                break;
            case eOK:
                indISCHolder_ = 0;
                break;
            default:
                throw SOCIError("Use element used with non-supported indicator type.");
        }
    }

    XSQLVAR * var = statement_.sqlda2p_->sqlvar+position_;

    // then set parameters for query execution
    switch (type_)
    {
            // simple cases
        case eXChar:
            setTextParam(getUseVectorValue<char>(data_, row), 1, buf_, var);
            break;
        case eXShort:
            to_isc<short>(
                static_cast<void*>(getUseVectorValue<short>(data_, row)),
                var);
            break;
        case eXInteger:
            to_isc<int>(
                static_cast<void*>(getUseVectorValue<int>(data_, row)),
                var);
            break;
        case eXUnsignedLong:
            to_isc<unsigned long>(
                static_cast<void*>(getUseVectorValue<unsigned long>(data_, row)),
                var);
            break;
        case eXDouble:
            to_isc<double>(
                static_cast<void*>(getUseVectorValue<double>(data_, row)),
                var);
            break;

            // cases that require adjustments and buffer management
        case eXStdString:
            {
                std::string *tmp = getUseVectorValue<std::string>(data_, row);
                setTextParam(tmp->c_str(), tmp->size(), buf_, var);
            }
            break;
        case eXStdTm:
            tmEncode(var->sqltype,
                     getUseVectorValue<std::tm>(data_, row), buf_);
            break;
//  Not supported
//  case eXCString:
//  case eXBLOB:
        default:
            throw SOCIError("Use element used with non-supported type.");
    } // switch
}

std::size_t FirebirdVectorUseTypeBackEnd::size()
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
            throw SOCIError("Use vector element used with non-supported type.");
    }

    return sz;
}

void FirebirdVectorUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete[] buf_;
        buf_ = NULL;
    }
}
