//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

#include "mysql/test-mysql.h"

#include <catch.hpp>

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

class test_context_odbc : public test_context
{
public:
    using test_context::test_context;

    std::string get_example_connection_string() const override
    {
        return "FILEDSN=./test-mysql.dsn";
    }

    std::string get_backend_name() const override
    {
        return "odbc";
    }

    bool truncates_uint64_to_int64() const override
    {
        // The ODBC driver of MySQL truncates values bigger then INT64_MAX.
        // There are open bugs related to this issue:
        // - https://bugs.mysql.com/bug.php?id=95978
        // - https://bugs.mysql.com/bug.php?id=61114
        // Driver version 8.0.31 seems to have fixed this (https://github.com/mysql/mysql-connector-odbc/commit/e78da1344247752f76a082de51cfd36d5d2dd98f),
        // but we use an older version in the AppVeyor builds.
        return true;
    }

    table_creator_base* table_creator_blob(soci::session&) const override
    {
      // Blobs are not supported
      return nullptr;
    }
};

test_context_odbc tc_odbc_mysql;
