//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-mysql.h"
#include "test/common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <ctime>
#include <ciso646>
#include <cstdlib>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = mysql;


// procedure call test
void test1()
{
    {
        session sql(backEnd, connectString);

        mysql_session_backend *sessionBackEnd
            = static_cast<mysql_session_backend *>(sql.get_backend());
        std::string version = mysql_get_server_info(sessionBackEnd->conn_);
        int v;
        std::istringstream iss(version);
        if ((iss >> v) and v < 5)
        {
            std::cout << "skipping test 1 (MySQL server version ";
            std::cout << version << " does not support stored procedures)\n";
            return;
        }

        try { sql << "drop function myecho"; }
        catch (soci_error const &) {}

        sql <<
            "create function myecho(msg text) "
            "returns text deterministic "
            "  return msg; ";

        std::string in("my message");
        std::string out;

        statement st = (sql.prepare <<
            "select myecho(:input)",
            into(out),
            use(in, "input"));

        st.execute(1);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;

            procedure proc = (sql.prepare <<
                "myecho(:input)",
                into(out), use(in, "input"));

            proc.execute(1);
            assert(out == in);
        }

        sql << "drop function myecho";
    }

    std::cout << "test 1 passed" << std::endl;
}

// MySQL error reporting test.
void test2()
{
    {
        try
        {
            session sql(backEnd, "host=test.soci.invalid");
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.errNum_ == CR_UNKNOWN_HOST);
        }
    }

    {
        session sql(backEnd, connectString);
        sql << "create table soci_test (id integer)";
        try
        {
            int n;
            sql << "select id from soci_test_nosuchtable", into(n);
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.errNum_ == ER_NO_SUCH_TABLE);
        }
        try
        {
            sql << "insert into soci_test (invalid) values (256)";
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.errNum_ == ER_BAD_FIELD_ERROR);
        }
        // A bulk operation.
        try
        {
            std::vector<int> v(3, 5);
            sql << "insert into soci_test_nosuchtable values (:n)", use(v);
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.errNum_ == ER_NO_SUCH_TABLE);
        }
        sql << "drop table soci_test";
    }

    std::cout << "test 2 passed" << std::endl;
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(session& session)
        : table_creator_base(session)
    {
        session << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "tm datetime, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20)) type=InnoDB";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(session& session)
        : table_creator_base(session)
    {
        session  << "create table soci_test(num_float float8, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct table_creator_three : public table_creator_base
{
    table_creator_three(session& session)
        : table_creator_base(session)
    {
        session << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
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

    std::string to_date_time(std::string const &dateString) const
    {
        return "\'" + dateString + "\'";
    }

};

bool are_transactions_supported()
{
    session sql(backEnd, connectString);
    sql << "drop table if exists soci_test";
    sql << "create table soci_test (id int) type=InnoDB";
    row r;
    sql << "show table status like \'soci_test\'", into(r);
    bool retv = (r.get<std::string>(1) == "InnoDB");
    sql << "drop table soci_test";
    return retv;
}

int main(int argc, char** argv)
{
    if (argc == 2)
    {
        connectString = argv[1];
    }
    else
    {
        std::cout << "usage: " << argv[0]
            << " connectstring\n"
            << "example: " << argv[0]
            << " \"dbname=test user=root password=\'Ala ma kota\'\"\n";
        exit(1);
    }

    try
    {
        test_context tc(backEnd, connectString);
        common_tests tests(tc);
        bool checkTransactions = are_transactions_supported();
        tests.run(checkTransactions);

        std::cout << "\nSOCI MySQL Tests:\n\n";

        test1();
        test2();

        std::cout << "\nOK, all tests passed.\n\n";
        return EXIT_SUCCESS;
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
