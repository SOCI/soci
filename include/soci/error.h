//
// Copyright (C) 2004-2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ERROR_H_INCLUDED
#define SOCI_ERROR_H_INCLUDED

#include "soci/soci-config.h"
// std
#include <stdexcept>
#include <string>

namespace soci
{

class SOCI_DECL soci_error : public std::runtime_error
{
public:
    explicit soci_error(std::string const & msg);
};

class SOCI_DECL connection_cancelled : public soci_error
{
public:
    explicit connection_cancelled(std::string const & msg);
};

class SOCI_DECL sql_error : public soci_error
{
public:
    explicit sql_error(std::string const & msg);
    ~sql_error();

    virtual int native_code() const = 0;
    virtual std::string sql_state() const = 0;
};

} // namespace soci

#endif // SOCI_ERROR_H_INCLUDED
