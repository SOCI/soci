//
// Copyright (C) 2021 Sinitsyn Ilya
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_VECTOR_HELPERS_H_INCLUDED
#define SOCI_VECTOR_HELPERS_H_INCLUDED

#include "soci-exchange-cast.h"

namespace soci
{

namespace details
{

// Helper functions to work with vectors.

template <exchange_type e>
std::vector<typename exchange_type_traits<e>::value_type>& exchange_vector_type_cast(void *data)
{
    return *static_cast<std::vector<typename exchange_type_traits<e>::value_type>*>(data);
}

// Get the size of the vector.
inline std::size_t get_vector_size(exchange_type e, void *data)
{
    switch (e)
    {
        case x_char:
            return exchange_vector_type_cast<x_char>(data).size();
        case x_stdstring:
            return exchange_vector_type_cast<x_stdstring>(data).size();
        case x_int8:
            return exchange_vector_type_cast<x_int8>(data).size();
        case x_uint8:
            return exchange_vector_type_cast<x_uint8>(data).size();
        case x_int16:
            return exchange_vector_type_cast<x_int16>(data).size();
        case x_uint16:
            return exchange_vector_type_cast<x_uint16>(data).size();
        case x_int32:
            return exchange_vector_type_cast<x_int32>(data).size();
        case x_uint32:
            return exchange_vector_type_cast<x_uint32>(data).size();
        case x_int64:
            return exchange_vector_type_cast<x_int64>(data).size();
        case x_uint64:
            return exchange_vector_type_cast<x_uint64>(data).size();
        case x_double:
            return exchange_vector_type_cast<x_double>(data).size();
        case x_stdtm:
            return exchange_vector_type_cast<x_stdtm>(data).size();
        case x_xmltype:
            return exchange_vector_type_cast<x_xmltype>(data).size();
        case x_longstring:
            return exchange_vector_type_cast<x_longstring>(data).size();
        case x_statement:
        case x_rowid:
        case x_blob:
            break;
    }
    throw soci_error("Failed to get the size of the vector of non-supported type.");
}

// Set the size of the vector.
inline void resize_vector(exchange_type e, void *data, std::size_t newSize)
{
    switch (e)
    {
        case x_char:
            exchange_vector_type_cast<x_char>(data).resize(newSize);
            return;
        case x_stdstring:
            exchange_vector_type_cast<x_stdstring>(data).resize(newSize);
            return;
        case x_int8:
            exchange_vector_type_cast<x_int8>(data).resize(newSize);
            return;
        case x_uint8:
            exchange_vector_type_cast<x_uint8>(data).resize(newSize);
            return;
        case x_int16:
            exchange_vector_type_cast<x_int16>(data).resize(newSize);
            return;
        case x_uint16:
            exchange_vector_type_cast<x_uint16>(data).resize(newSize);
            return;
        case x_int32:
            exchange_vector_type_cast<x_int32>(data).resize(newSize);
            return;
        case x_uint32:
            exchange_vector_type_cast<x_uint32>(data).resize(newSize);
            return;
        case x_int64:
            exchange_vector_type_cast<x_int64>(data).resize(newSize);
            return;
        case x_uint64:
            exchange_vector_type_cast<x_uint64>(data).resize(newSize);
            return;
        case x_double:
            exchange_vector_type_cast<x_double>(data).resize(newSize);
            return;
        case x_stdtm:
            exchange_vector_type_cast<x_stdtm>(data).resize(newSize);
            return;
        case x_xmltype:
            exchange_vector_type_cast<x_xmltype>(data).resize(newSize);
            return;
        case x_longstring:
            exchange_vector_type_cast<x_longstring>(data).resize(newSize);
            return;
        case x_statement:
        case x_rowid:
        case x_blob:
            break;
    }
    throw soci_error("Failed to get the size of the vector of non-supported type.");
}

// Get the string at the given index of the vector.
inline std::string& vector_string_value(exchange_type e, void *data, std::size_t ind)
{
    switch (e)
    {
        case x_stdstring:
            return exchange_vector_type_cast<x_stdstring>(data).at(ind);
        case x_xmltype:
            return exchange_vector_type_cast<x_xmltype>(data).at(ind).value;
        case x_longstring:
            return exchange_vector_type_cast<x_longstring>(data).at(ind).value;
        case x_char:
        case x_int8:
        case x_uint8:
        case x_int16:
        case x_uint16:
        case x_int32:
        case x_uint32:
        case x_int64:
        case x_uint64:
        case x_double:
        case x_stdtm:
        case x_statement:
        case x_rowid:
        case x_blob:
            break;
    }
    throw soci_error("Can't get the string value from the vector of values with non-supported type.");
}

} // namespace details

} // namespace soci

#endif // SOCI_VECTOR_HELPERS_H_INCLUDED
