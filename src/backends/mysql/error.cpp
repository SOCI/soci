//
// Copyright (C) 2025 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"

using namespace soci;

mysql_soci_error::mysql_soci_error(std::string const & msg, int errNum)
    : soci_error(msg), err_num_(errNum)
{
}

soci_error::error_category mysql_soci_error::get_error_category() const
{
    if (err_num_ == CR_CONNECTION_ERROR ||
        err_num_ == CR_CONN_HOST_ERROR ||
        err_num_ == CR_SERVER_GONE_ERROR ||
        err_num_ == CR_SERVER_LOST ||
        err_num_ == 1927) // Lost connection to backend server
    {
        return connection_error;
    }

    return unknown;
}
