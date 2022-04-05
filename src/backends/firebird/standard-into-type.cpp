//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci/firebird/soci-firebird.h"
#include "soci-exchange-cast.h"
#include "soci-compiler.h"
#include "firebird/common.h"
#include "soci/soci.h"

#include <sstream>

using namespace soci;
using namespace soci::details;
using namespace soci::details::firebird;

void firebird_standard_into_type_backend::define_by_pos(
    int & position, void * data, exchange_type type)
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

void firebird_standard_into_type_backend::pre_fetch()
{
    // nothing to do
}

void firebird_standard_into_type_backend::post_fetch(
    bool gotData, bool calledFromFetch, indicator * ind)
{
    if (calledFromFetch && (gotData == false))
    {
        // this is a normal end-of-rowset condition,
        // no need to set anything (fetch() will return false)
        return;
    }

    if (gotData)
    {
        if (i_null == statement_.inds_[position_][0] && NULL == ind)
        {
            throw soci_error("Null value fetched and no indicator defined.");
        }
        else if (NULL != ind)
        {
            *ind = statement_.inds_[position_][0];
        }
    }
}


void firebird_standard_into_type_backend::exchangeData()
{
    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    switch (type_)
    {
            // simple cases
        case x_char:
            exchange_type_cast<x_char>(data_) = getTextParam(var)[0];
            break;
        case x_int8:
            exchange_type_cast<x_int8>(data_) = from_isc<int8_t>(var);
            break;
        case x_uint8:
            exchange_type_cast<x_uint8>(data_) = from_isc<uint8_t>(var);
            break;
        case x_int16:
            exchange_type_cast<x_int16>(data_) = from_isc<int16_t>(var);
            break;
        case x_uint16:
            exchange_type_cast<x_uint16>(data_) = from_isc<uint16_t>(var);
            break;
        case x_int32:
            exchange_type_cast<x_int32>(data_) = from_isc<int32_t>(var);
            break;
        case x_uint32:
            exchange_type_cast<x_uint32>(data_) = from_isc<uint32_t>(var);
            break;
        case x_int64:
            exchange_type_cast<x_int64>(data_) = from_isc<int64_t>(var);
            break;
        case x_uint64:
            exchange_type_cast<x_uint64>(data_) = from_isc<uint64_t>(var);
            break;
        case x_double:
            exchange_type_cast<x_double>(data_) = from_isc<double>(var);
            break;

            // cases that require adjustments and buffer management
        case x_stdstring:
            exchange_type_cast<x_stdstring>(data_) = getTextParam(var);
            break;
        case x_stdtm:
            {
                std::tm& t = exchange_type_cast<x_stdtm>(data_);
                tmDecode(var->sqltype, buf_, &t);

                // isc_decode_timestamp() used by tmDecode() incorrectly sets
                // tm_isdst to 0 in the struct that it creates, see
                // http://tracker.firebirdsql.org/browse/CORE-3877, work around it
                // by pretending the DST is actually unknown.
                t.tm_isdst = -1;
            }
            break;

            // cases that require special handling
        case x_blob:
            {
                blob *tmp = reinterpret_cast<blob*>(data_);

                firebird_blob_backend *blob =
                    dynamic_cast<firebird_blob_backend*>(tmp->get_backend());

                if (0 == blob)
                {
                    throw soci_error("Can't get Firebid BLOB BackEnd");
                }

                SOCI_GCC_WARNING_SUPPRESS(cast-align)

                blob->assign(*reinterpret_cast<ISC_QUAD*>(buf_));

                SOCI_GCC_WARNING_RESTORE(cast-align)
            }
            break;

        case x_longstring:
            {
                std::string &tmp = exchange_type_cast<x_longstring>(data_).value;
                copy_from_blob(statement_, buf_, tmp);
            }
            break;

        case x_xmltype:
            {
                std::string &tmp = exchange_type_cast<x_xmltype>(data_).value;
                copy_from_blob(statement_, buf_, tmp);
            }
            break;

        default:
            throw soci_error("Into element used with non-supported type.");
    } // switch
}

void firebird_standard_into_type_backend::clean_up()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
    std::vector<void*>::iterator it =
        std::find(statement_.intos_.begin(), statement_.intos_.end(), this);
    if (it != statement_.intos_.end())
        statement_.intos_.erase(it);
}
