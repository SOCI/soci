//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "common.h"
#include <soci.h>

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Firebird;

void FirebirdStandardUseTypeBackEnd::bindByPos(
    int & position, void * data, eExchangeType type)
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

    statement_.useType_ = eStandard;
    statement_.uses_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqlda2p_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;

	statement_.boundByPos_ = true;
}

void FirebirdStandardUseTypeBackEnd::bindByName(
    std::string const & name, void * data,
    eExchangeType type)
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

    statement_.useType_ = eStandard;
    statement_.uses_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqlda2p_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;

	statement_.boundByName_ = true;
}

void FirebirdStandardUseTypeBackEnd::preUse(eIndicator const * ind)
{
    if (ind)
    {
        switch (*ind)
        {
            case eNull:
                indISCHolder_ = -1;
                break;
            case eOK:
                indISCHolder_ =  0;
                break;
            default:
                throw SOCIError("Unsupported indicator value.");
        }
    }
}

void FirebirdStandardUseTypeBackEnd::exchangeData()
{
    XSQLVAR *var = statement_.sqlda2p_->sqlvar+position_;

    switch (type_)
    {
        case eXChar:
            setTextParam(static_cast<char*>(data_), 1, buf_, var);
            break;
        case eXShort:
            to_isc<short>(data_, var);
            break;
        case eXInteger:
            to_isc<int>(data_, var);
            break;
        case eXUnsignedLong:
            to_isc<unsigned long>(data_, var);
            break;
        case eXDouble:
            to_isc<double>(data_, var);
            break;

            // cases that require adjustments and buffer management
        case eXCString:
            {
                details::CStringDescriptor *tmp
                = static_cast<CStringDescriptor *>(data_);

                // remove trailing nulls
                while (tmp->str_[tmp->bufSize_-1] == '\0')
                {
                    --tmp->bufSize_;
                }

                setTextParam(tmp->str_, tmp->bufSize_, buf_, var);
            }
            break;
        case eXStdString:
            {
                std::string *tmp = static_cast<std::string*>(data_);
                setTextParam(tmp->c_str(), tmp->size(), buf_, var);
            }
            break;
        case eXStdTm:
            tmEncode(var->sqltype,
                     static_cast<std::tm*>(data_), buf_);
            break;

            // cases that require special handling
        case eXBLOB:
            {
                BLOB *tmp = static_cast<BLOB*>(data_);

                FirebirdBLOBBackEnd *blob =
                    dynamic_cast<FirebirdBLOBBackEnd *>(tmp->getBackEnd());

                if (blob==0)
                {
                    throw SOCIError("Can't get Firebid BLOB BackEnd");
                }

                blob->save();
                memcpy(buf_, &blob->bid_, var->sqllen);
            }
            break;
        default:
            throw SOCIError("Use element used with non-supported type.");
    } // switch
}

void FirebirdStandardUseTypeBackEnd::postUse(
    bool /* gotData */, eIndicator * /* ind */)
{
    // ...
}

void FirebirdStandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete[] buf_;
        buf_ = NULL;
    }
}
