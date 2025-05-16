//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "test-context.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

#include <catch.hpp>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh integer, ll number, ul number, "
                 "d float, num76 numeric(7,6), "
                 "tm timestamp, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20))";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(soci::session & sql)
        : table_creator_base(sql)
    {
        sql  << "create table soci_test(num_float float, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct table_creator_three : public table_creator_base
{
    table_creator_three(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

struct table_creator_for_get_affected_rows : table_creator_base
{
    table_creator_for_get_affected_rows(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val integer)";
    }
};

//
// Support for SOCI Common Tests
//

class test_context : public test_context_common
{
public:

    test_context() = default;

    std::string get_example_connection_string() const override
    {
        return "FILEDSN=./test-access.dsn";
    }

    std::string get_backend_name() const override
    {
        return "odbc";
    }

    table_creator_base * table_creator_1(soci::session& s) const
    {
        return new table_creator_one(s);
    }

    table_creator_base * table_creator_2(soci::session& s) const
    {
        return new table_creator_two(s);
    }

    table_creator_base * table_creator_3(soci::session& s) const
    {
        return new table_creator_three(s);
    }

    table_creator_base * table_creator_4(soci::session& s) const
    {
        return new table_creator_for_get_affected_rows(s);
    }

    std::string fromDual(std::string const &sql) const
    {
        return sql;
    }

    std::string toDate(std::string const &datdt_string) const
    {
        return "#" + datdt_string + "#";
    }

    std::string to_date_time(std::string const &datdt_string) const
    {
        return "#" + datdt_string + "#";
    }

    virtual std::string sql_length(std::string const& s) const
    {
        return "len(" + s + ")";
    }
};

test_context tc_odbc_access;
