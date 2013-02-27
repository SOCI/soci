//
// Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_QUERY_TRANSFORMATION_H_INCLUDED
#define SOCI_QUERY_TRANSFORMATION_H_INCLUDED

#include "soci-config.h"
#include <functional>
#include <string>

namespace soci
{

namespace details
{

class query_transformation_function : public std::unary_function<std::string const&, std::string>
{
public:
    virtual result_type operator()(argument_type a) const = 0;
};

template <typename T>
class query_transformation : public query_transformation_function
{
public:
    query_transformation(T callback)
        : callback_(callback)
    {}

    result_type operator()(argument_type query) const
    {
        return callback_(query);
    }

private:
    T callback_;
};

} // namespace details

} // namespace soci

#endif // SOCI_QUERY_TRANSFORMATION_H_INCLUDED
