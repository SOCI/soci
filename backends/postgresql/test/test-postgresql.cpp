//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-postgresql.h"
#include "common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cstdlib>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_postgresql();

// Postgres-specific tests

struct oid_table_creator : public table_creator_base
{
    oid_table_creator(session& sql)
    : table_creator_base(sql)
    {
        sql << "create table soci_test ("
                " id integer,"
                " name varchar(100)"
                ") with oids";
    }
};

// ROWID test
// Note: in PostgreSQL, there is no ROWID, there is OID.
// It is still provided as a separate type for "portability",
// whatever that means.
void test1()
{
    {
        session sql(backEnd, connectString);

        oid_table_creator tableCreator(sql);

        sql << "insert into soci_test(id, name) values(7, \'John\')";

        rowid rid(sql);
        sql << "select oid from soci_test where id = 7", into(rid);

        int id;
        std::string name;

#ifndef SOCI_POSTGRESQL_NOPARAMS

        sql << "select id, name from soci_test where oid = :rid",
            into(id), into(name), use(rid);

#else
        // Older PostgreSQL does not support use elements.

        postgresql_rowid_backend *rbe
            = static_cast<postgresql_rowid_backend *>(rid.get_backend());

        unsigned long oid = rbe->value_;

        sql << "select id, name from soci_test where oid = " << oid,
            into(id), into(name);

#endif // SOCI_POSTGRESQL_NOPARAMS

        assert(id == 7);
        assert(name == "John");
    }

    std::cout << "test 1 passed" << std::endl;
}

// function call test
class function_creator : function_creator_base
{
public:

    function_creator(session & sql)
    : function_creator_base(sql)
    {
        // before a language can be used it must be defined
        // if it has already been defined then an error will occur
        try { sql << "create language plpgsql"; }
        catch (soci_error const &) {} // ignore if error

#ifndef SOCI_POSTGRESQL_NOPARAMS

        sql  <<
            "create or replace function soci_test(msg varchar) "
            "returns varchar as $$ "
            "begin "
            "  return msg; "
            "end $$ language plpgsql";
#else

       sql <<
            "create or replace function soci_test(varchar) "
            "returns varchar as \' "
            "begin "
            "  return $1; "
            "end \' language plpgsql";
#endif
    }

protected:

    std::string drop_statement()
    {
        return "drop function soci_test(varchar)";
    }
};

void test2()
{
    {
        session sql(backEnd, connectString);

        function_creator functionCreator(sql);

        std::string in("my message");
        std::string out;

#ifndef SOCI_POSTGRESQL_NOPARAMS

        statement st = (sql.prepare <<
            "select soci_test(:input)",
            into(out),
            use(in, "input"));

#else
        // Older PostgreSQL does not support use elements.

        statement st = (sql.prepare <<
            "select soci_test(\'" << in << "\')",
            into(out));

#endif // SOCI_POSTGRESQL_NOPARAMS

        st.execute(true);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;

#ifndef SOCI_POSTGRESQL_NOPARAMS

            procedure proc = (sql.prepare <<
                "soci_test(:input)",
                into(out), use(in, "input"));

#else
        // Older PostgreSQL does not support use elements.

            procedure proc = (sql.prepare <<
                "soci_test(\'" << in << "\')", into(out));

#endif // SOCI_POSTGRESQL_NOPARAMS

            proc.execute(true);
            assert(out == in);
        }
    }

    std::cout << "test 2 passed" << std::endl;
}

// BLOB test
struct blob_table_creator : public table_creator_base
{
    blob_table_creator(session & sql)
    : table_creator_base(sql)
    {
        sql <<
             "create table soci_test ("
             "    id integer,"
             "    img oid"
             ")";
    }
};

void test3()
{
    {
        session sql(backEnd, connectString);

        blob_table_creator tableCreator(sql);

        char buf[] = "abcdefghijklmnopqrstuvwxyz";

        sql << "insert into soci_test(id, img) values(7, lo_creat(-1))";

        // in PostgreSQL, BLOB operations must be within transaction block
        transaction tr(sql);

        {
            blob b(sql);

            sql << "select img from soci_test where id = 7", into(b);
            assert(b.get_len() == 0);

            b.write(0, buf, sizeof(buf));
            assert(b.get_len() == sizeof(buf));

            b.append(buf, sizeof(buf));
            assert(b.get_len() == 2 * sizeof(buf));
        }
        {
            blob b(sql);
            sql << "select img from soci_test where id = 7", into(b);
            assert(b.get_len() == 2 * sizeof(buf));
            char buf2[100];
            b.read(0, buf2, 10);
            assert(std::strncmp(buf2, "abcdefghij", 10) == 0);
        }

        unsigned long oid;
        sql << "select img from soci_test where id = 7", into(oid);
        sql << "select lo_unlink(" << oid << ")";
    }

    std::cout << "test 3 passed" << std::endl;
}

struct longlong_table_creator : table_creator_base
{
    longlong_table_creator(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val int8)";
    }
};

// long long test
void test4()
{
    {
        session sql(backEnd, connectString);

        longlong_table_creator tableCreator(sql);

        long long v1 = 1000000000000LL;
        assert(v1 / 1000000 == 1000000);

        sql << "insert into soci_test(val) values(:val)", use(v1);

        long long v2 = 0LL;
        sql << "select val from soci_test", into(v2);

        assert(v2 == v1);
    }

    // vector<long long>
    {
        session sql(backEnd, connectString);

        longlong_table_creator tableCreator(sql);

        std::vector<long long> v1;
        v1.push_back(1000000000000LL);
        v1.push_back(1000000000001LL);
        v1.push_back(1000000000002LL);
        v1.push_back(1000000000003LL);
        v1.push_back(1000000000004LL);

        sql << "insert into soci_test(val) values(:val)", use(v1);

        std::vector<long long> v2(10);
        sql << "select val from soci_test order by val desc", into(v2);

        assert(v2.size() == 5);
        assert(v2[0] == 1000000000004LL);
        assert(v2[1] == 1000000000003LL);
        assert(v2[2] == 1000000000002LL);
        assert(v2[3] == 1000000000001LL);
        assert(v2[4] == 1000000000000LL);
    }

    std::cout << "test 4 passed" << std::endl;
}

// unsigned long long test
void test4ul()
{
    {
        session sql(backEnd, connectString);

        longlong_table_creator tableCreator(sql);

        unsigned long long v1 = 1000000000000ULL;
        assert(v1 / 1000000 == 1000000);

        sql << "insert into soci_test(val) values(:val)", use(v1);

        unsigned long long v2 = 0ULL;
        sql << "select val from soci_test", into(v2);

        assert(v2 == v1);
    }
}

struct boolean_table_creator : table_creator_base
{
    boolean_table_creator(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val boolean)";
    }
};

void test5()
{
    {
        session sql(backEnd, connectString);

        boolean_table_creator tableCreator(sql);

        int i1 = 0;

        sql << "insert into soci_test(val) values(:val)", use(i1);

        int i2 = 7;
        sql << "select val from soci_test", into(i2);

        assert(i2 == i1);

        sql << "update soci_test set val = true";
        sql << "select val from soci_test", into(i2);
        assert(i2 == 1);
    }

    std::cout << "test 5 passed" << std::endl;
}

// dynamic backend test
void test6()
{
    try
    {
        session sql("nosuchbackend://" + connectString);
        assert(false);
    }
    catch (soci_error const & e)
    {
        assert(e.what() == std::string("Failed to open: libsoci_nosuchbackend.so"));
    }

    {
        dynamic_backends::register_backend("pgsql", backEnd);

        std::vector<std::string> backends = dynamic_backends::list_all();
        assert(backends.size() == 1);
        assert(backends[0] == "pgsql");

        {
            session sql("pgsql://" + connectString);
        }

        dynamic_backends::unload("pgsql");

        backends = dynamic_backends::list_all();
        assert(backends.empty());
    }

    {
        session sql("postgresql://" + connectString);
    }

    std::cout << "test 6 passed" << std::endl;
}

void test7()
{
    {
        session sql(backEnd, connectString);

        int i;
        sql << "select 123", into(i);
        assert(i == 123);

        try
        {
            sql << "select 'ABC'", into (i);
            assert(false);
        }
        catch (soci_error const & e)
        {
            assert(e.what() == std::string("Cannot convert data."));
        }
    }

    std::cout << "test 7 passed" << std::endl;
}

void test8()
{
    {
        session sql(backEnd, connectString);

        assert(sql.get_backend_name() == "postgresql");
    }

    std::cout << "test 8 passed" << std::endl;
}

// test for double-colon cast in SQL expressions
void test9()
{
    {
        session sql(backEnd, connectString);

        int a = 123;
        int b = 0;
        sql << "select :a::integer", use(a), into(b);
        assert(b == a);
    }

    std::cout << "test 9 passed" << std::endl;
}

// test for date, time and timestamp parsing
void test10()
{
    {
        session sql(backEnd, connectString);

        std::string someDate = "2009-06-17 22:51:03.123";
        std::tm t1, t2, t3;

        sql << "select :sd::date, :sd::time, :sd::timestamp",
            use(someDate, "sd"), into(t1), into(t2), into(t3);

        // t1 should contain only the date part
        assert(t1.tm_year == 2009 - 1900);
        assert(t1.tm_mon == 6 - 1);
        assert(t1.tm_mday == 17);
        assert(t1.tm_hour == 0);
        assert(t1.tm_min == 0);
        assert(t1.tm_sec == 0);

        // t2 should contain only the time of day part
        assert(t2.tm_year == 0);
        assert(t2.tm_mon == 0);
        assert(t2.tm_mday == 1);
        assert(t2.tm_hour == 22);
        assert(t2.tm_min == 51);
        assert(t2.tm_sec == 3);

        // t3 should contain all information
        assert(t3.tm_year == 2009 - 1900);
        assert(t3.tm_mon == 6 - 1);
        assert(t3.tm_mday == 17);
        assert(t3.tm_hour == 22);
        assert(t3.tm_min == 51);
        assert(t3.tm_sec == 3);
    }

    std::cout << "test 10 passed" << std::endl;
}

// test for number of affected rows

struct table_creator_for_test11 : table_creator_base
{
    table_creator_for_test11(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val integer)";
    }
};

void test11()
{
    {
        session sql(backEnd, connectString);

        table_creator_for_test11 tableCreator(sql);

        for (int i = 0; i != 10; i++)
        {
            sql << "insert into soci_test(val) values(:val)", use(i);
        }

        statement st1 = (sql.prepare <<
            "update soci_test set val = val + 1");
        st1.execute(false);

        assert(st1.get_affected_rows() == 10);

        statement st2 = (sql.prepare <<
            "delete from soci_test where val <= 5");
        st2.execute(false);

        assert(st2.get_affected_rows() == 5);
    }

    std::cout << "test 11 passed" << std::endl;
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "tm timestamp, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20))";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(session & sql)
        : table_creator_base(sql)
    {
        sql  << "create table soci_test(num_float float8, num_int integer,"
                     " name varchar(20), sometime timestamp, chr char)";
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

//
// Support for soci Common Tests
//

class test_context : public test_context_base
{
public:
    test_context(backend_factory const &backEnd, std::string const &connectString)
        : test_context_base(backEnd, connectString)
    {}

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

    std::string to_date_time(std::string const &datdt_string) const
    {
        return "timestamptz(\'" + datdt_string + "\')";
    }
};


int main(int argc, char** argv)
{

#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run asser()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc == 2)
    {
        connectString = argv[1];
    }
    else
    {
        std::cout << "usage: " << argv[0]
            << " connectstring\n"
            << "example: " << argv[0]
            << " \'connect_string_for_PostgreSQL\'\n";
        return EXIT_FAILURE;
    }

    try
    {
        test_context tc(backEnd, connectString);
        common_tests tests(tc);
        tests.run();

        std::cout << "\nSOCI Postgres Tests:\n\n";
        test1();
        test2();
        test3();
        test4();
        test4ul();
        test5();

//         test6();
        std::cout << "test 6 skipped (dynamic backend)\n";

        test7();
        test8();
        test9();
        test10();
        test11();

        std::cout << "\nOK, all tests passed.\n\n";
        return EXIT_SUCCESS;
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
    return EXIT_FAILURE;
}
