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
        case x_short:
            return exchange_vector_type_cast<x_short>(data).size();
        case x_integer:
            return exchange_vector_type_cast<x_integer>(data).size();
        case x_long_long:
            return exchange_vector_type_cast<x_long_long>(data).size();
        case x_unsigned_long_long:
            return exchange_vector_type_cast<x_unsigned_long_long>(data).size();
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
        case x_short:
            exchange_vector_type_cast<x_short>(data).resize(newSize);
            return;
        case x_integer:
            exchange_vector_type_cast<x_integer>(data).resize(newSize);
            return;
        case x_long_long:
            exchange_vector_type_cast<x_long_long>(data).resize(newSize);
            return;
        case x_unsigned_long_long:
            exchange_vector_type_cast<x_unsigned_long_long>(data).resize(newSize);
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
        case x_short:
        case x_integer:
        case x_long_long:
        case x_unsigned_long_long:
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
