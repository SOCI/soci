//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/firebird/soci-firebird.h"
#include "firebird/common.h"
#include "soci-vector-helpers.h"

using namespace soci;
using namespace soci::details;
using namespace soci::details::firebird;

void firebird_vector_into_type_backend::define_by_pos(
    int & position, void * data, exchange_type type)
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

void firebird_vector_into_type_backend::pre_fetch()
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
void firebird_vector_into_type_backend::exchangeData(std::size_t row)
{
    XSQLVAR *var = statement_.sqldap_->sqlvar+position_;

    switch (type_)
    {
        // simple cases
    case x_char:
        setIntoVector(data_, row, getTextParam(var)[0]);
        break;
    case x_int8:
        {
            int8_t tmp = from_isc<int8_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_uint8:
        {
            uint8_t tmp = from_isc<uint8_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_int16:
        {
            int16_t tmp = from_isc<int16_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_uint16:
        {
            uint16_t tmp = from_isc<uint16_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_int32:
        {
            int32_t tmp = from_isc<int32_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_uint32:
        {
            uint32_t tmp = from_isc<uint32_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_int64:
        {
            int64_t tmp = from_isc<int64_t>(var);
            setIntoVector(data_, row, tmp);
        }
        break;
    case x_uint64:
        {
            uint64_t tmp = from_isc<uint64_t>(var);
            setIntoVector(data_, row, tmp);
        }
    break;
    case x_double:
        {
            double tmp = from_isc<double>(var);
            setIntoVector(data_, row, tmp);
        }
        break;

        // cases that require adjustments and buffer management
    case x_stdstring:
        setIntoVector(data_, row, getTextParam(var));
        break;
    case x_stdtm:
        {
            std::tm data = std::tm();
            tmDecode(var->sqltype, buf_, &data);
            setIntoVector(data_, row, data);
        }
        break;

    case x_longstring:
        {
            std::string &tmp = exchange_vector_type_cast<x_longstring>(data_)[row].value;
            copy_from_blob(statement_, buf_, tmp);
        }
        break;

    case x_xmltype:
        {
            std::string &tmp = exchange_vector_type_cast<x_xmltype>(data_)[row].value;
            copy_from_blob(statement_, buf_, tmp);
        }
        break;

    default:
        throw soci_error("Into vector element used with non-supported type.");
    } // switch

}

void firebird_vector_into_type_backend::post_fetch(
    bool gotData, indicator * ind)
{
    // Here we have to set indicators only. Data was exchanged with user
    // buffers during fetch()
    if (gotData)
    {
        std::size_t rows = statement_.rowsFetched_;

        for (std::size_t i = 0; i<rows; ++i)
        {
            if (statement_.inds_[position_][i] == i_null && (ind == NULL))
            {
                throw soci_error("Null value fetched and no indicator defined.");
            }
            else if (ind != NULL)
            {
                ind[i] = statement_.inds_[position_][i];
            }
        }
    }
}

void firebird_vector_into_type_backend::resize(std::size_t sz)
{
    resize_vector(type_, data_, sz);
}

std::size_t firebird_vector_into_type_backend::size() const
{
    return get_vector_size(type_, data_);
}

void firebird_vector_into_type_backend::clean_up()
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
