//
// Copyright (C) 2004-2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ERROR_H_INCLUDED
#define SOCI_ERROR_H_INCLUDED

#include "soci-config.h"
// std
#include <stdexcept>
#include <string>

namespace soci
{
    struct SOCI_DECL mn_odbc_error_info
    {
        void setDefault() { native_error_code_ = 0; odbc_error_message_.clear(); odbc_error_code_.clear(); odbc_func_returnval_ = 0; odbc_func_name_.clear(); }
        int         native_error_code_;
        std::string odbc_error_message_;
        std::string odbc_error_code_;
        int         odbc_func_returnval_;
        std::string odbc_func_name_;
    };

    class SOCI_DECL soci_error : public std::runtime_error
    {
    public:
        explicit soci_error(std::string const & msg);
    };

} // namespace soci

#endif // SOCI_ERROR_H_INCLUDED
