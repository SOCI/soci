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

void FirebirdStandardIntoTypeBackEnd::defineByPos(
    int & position, void * data, eExchangeType type)
{
    position_ = position-1;
    data_ = data;
    type_ = type;

    ++position;

    statement_.intoType_ = eStandard;
    statement_.intos_.push_back(static_cast<void*>(this));

    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    buf_ = allocBuffer(var);
    var->sqldata = buf_;
    var->sqlind = &indISCHolder_;
}

void FirebirdStandardIntoTypeBackEnd::preFetch()
{
    // nothing to do
}

void FirebirdStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }

    if (gotData)
    {
        if (statement_.inds_[position_][0] == eNull && ind == NULL)
        {
            throw SOCIError("Null value fetched and no indicator defined.");
        }
        else if (ind != NULL)
        {
            *ind = statement_.inds_[position_][0];
        }
    }
    else
    {
        if (ind == NULL)
        {
            throw SOCIError("No data fetched and no indicator defined.");
        }

        *ind = eNoData;
    }
}


void FirebirdStandardIntoTypeBackEnd::exchangeData()
{
    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    switch (type_)
    {
            // simple cases
        case eXChar:
            *reinterpret_cast<char*>(data_) = getTextParam(var)[0];
            break;
        case eXShort:
            {
                short t = from_isc<short>(var);
                *reinterpret_cast<short*>(data_) = t;
            }
            break;
        case eXInteger:
            {
                int t = from_isc<int>(var);
                *reinterpret_cast<int*>(data_) = t;
            }
            break;
        case eXUnsignedLong:
            {
                unsigned long t = from_isc<unsigned long>(var);
                *reinterpret_cast<unsigned long*>(data_) = t;
            }
            break;
        case eXDouble:
            {
                double t = from_isc<double>(var);
                *reinterpret_cast<double*>(data_) = t;
            }
            break;

            // cases that require adjustments and buffer management
        case eXCString:
            {
                details::CStringDescriptor *tmp =
                    static_cast<details::CStringDescriptor*>(data_);

                std::string stmp = getTextParam(var);
                std::strncpy(tmp->str_, stmp.c_str(), tmp->bufSize_ - 1);
                tmp->str_[tmp->bufSize_ - 1] = '\0';

                if (stmp.size() >= tmp->bufSize_)
                {
                    statement_.inds_[position_][0] = eTruncated;
                }
            }
            break;
        case eXStdString:
            *(reinterpret_cast<std::string*>(data_)) = getTextParam(var);
            break;
        case eXStdTm:
            tmDecode(var->sqltype,
                     buf_, static_cast<std::tm*>(data_));
            break;

            // cases that require special handling
        case eXBLOB:
            {
                BLOB *tmp = reinterpret_cast<BLOB*>(data_);

                FirebirdBLOBBackEnd *blob =
                    dynamic_cast<FirebirdBLOBBackEnd *>(tmp->getBackEnd());

                if (blob==0)
                {
                    throw SOCIError("Can't get Firebid BLOB BackEnd");
                }

                blob->assign(*reinterpret_cast<ISC_QUAD*>(buf_));
            }
            break;
        default:
            throw SOCIError("Into element used with non-supported type.");
    } // switch
}

void FirebirdStandardIntoTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete[] buf_;
        buf_ = NULL;
    }
}
