#ifndef SOCI_TESTS_MYSQL_H_INCLUDED
#define SOCI_TESTS_MYSQL_H_INCLUDED

#include "common-tests.h"

using namespace soci;
using namespace soci::tests;

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "num76 numeric(7,6), "
                 "tm datetime, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20)) engine=InnoDB";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(session & sql)
        : table_creator_base(sql)
    {
        sql  << "create table soci_test(num_float float8, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct table_creator_three : public table_creator_base
{
    table_creator_three(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

struct table_creator_for_get_affected_rows : table_creator_base
{
    table_creator_for_get_affected_rows(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val integer)";
    }
};

//
// Support for SOCI Common Tests
//

class test_context : public test_context_base
{
public:
    test_context(backend_factory const &backEnd,
                std::string const &connectString)
        : test_context_base(backEnd, connectString) {}

    table_creator_base* table_creator_1(session& s) const
    {
        return new table_creator_one(s);
    }

    table_creator_base* table_creator_2(session& s) const
    {
        return new table_creator_two(s);
    }

    table_creator_base* table_creator_3(session& s) const
    {
        return new table_creator_three(s);
    }

    table_creator_base* table_creator_4(session& s) const
    {
        return new table_creator_for_get_affected_rows(s);
    }

    std::string to_date_time(std::string const &datdt_string) const
    {
        return "\'" + datdt_string + "\'";
    }

    virtual bool has_fp_bug() const
    {
        // MySQL fails in the common test3() with "1.8000000000000000 !=
        // 1.7999999999999998", so don't use exact doubles comparisons for it.
        return true;
    }

    virtual bool has_transactions_support(session& sql) const
    {
        sql << "drop table if exists soci_test";
        sql << "create table soci_test (id int) engine=InnoDB";
        row r;
        sql << "show table status like \'soci_test\'", into(r);
        bool retv = (r.get<std::string>(1) == "InnoDB");
        sql << "drop table soci_test";
        return retv;
    }
};

#endif // SOCI_TESTS_MYSQL_H_INCLUDED
