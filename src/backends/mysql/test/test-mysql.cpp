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
#include <mysqld_error.h>
#include <errmsg.h>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_mysql();


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
            assert(e.err_num_ == CR_UNKNOWN_HOST ||
                   e.err_num_ == CR_CONN_HOST_ERROR);
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
            assert(e.err_num_ == ER_NO_SUCH_TABLE);
        }
        try
        {
            sql << "insert into soci_test (invalid) values (256)";
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.err_num_ == ER_BAD_FIELD_ERROR);
        }
        // A bulk operation.
        try
        {
            std::vector<int> v(3, 5);
            sql << "insert into soci_test_nosuchtable values (:n)", use(v);
        }
        catch (mysql_soci_error const &e)
        {
            assert(e.err_num_ == ER_NO_SUCH_TABLE);
        }
        sql << "drop table soci_test";
    }

    std::cout << "test 2 passed" << std::endl;
}

struct longlong_table_creator : table_creator_base
{
    longlong_table_creator(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val bigint)";
    }
};

// long long test
void test3()
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

    std::cout << "test 3 passed" << std::endl;
}

template <typename T>
void test_num(const char* s, bool valid, T value)
{
    try
    {
        session sql(backEnd, connectString);
        T val;
        sql << "select \'" << s << "\'", into(val);
        if (valid)
        {
            double v1 = static_cast<double>(value);
            double v2 = static_cast<double>(val);
            double d = std::fabs(v1 - v2);
            double epsilon = 0.001;
            assert(d < epsilon ||
                   d < epsilon * (std::fabs(v1) + std::fabs(v2)));
        }
        else
        {
            std::cout << "string \"" << s << "\" parsed as " << val
                      << " but should have failed.\n";
            assert(false);
        }
    }
    catch (soci_error const& e)
    {
        if (valid)
        {
            std::cout << "couldn't parse number: \"" << s << "\"\n";
            assert(false);
        }
        else
        {
            assert(std::string(e.what()) == "Cannot convert data.");
        }
    }
}

// Number conversion test.
void test4()
{
    test_num<double>("", false, 0);
    test_num<double>("foo", false, 0);
    test_num<double>("1", true, 1);
    test_num<double>("12", true, 12);
    test_num<double>("123", true, 123);
    test_num<double>("12345", true, 12345);
    test_num<double>("12341234123412341234123412341234123412341234123412341",
        true, 1.23412e+52);
    test_num<double>("99999999999999999999999912222222222222222222222222223"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333"
        "9999999999999999999999991222222222222222222222222222333333333333",
        false, 0);
    test_num<double>("1e3", true, 1000);
    test_num<double>("1.2", true, 1.2);
    test_num<double>("1.2345e2", true, 123.45);
    test_num<double>("1 ", false, 0);
    test_num<double>("     123", true, 123);
    test_num<double>("1,2", false, 0);
    test_num<double>("123abc", false, 0);
    test_num<double>("-0", true, 0);

    test_num<short>("123", true, 123);
    test_num<short>("100000", false, 0);

    test_num<int>("123", true, 123);
    test_num<int>("2147483647", true, 2147483647);
    test_num<int>("2147483647a", false, 0);
    test_num<int>("2147483648", false, 0);
    // -2147483648 causes a warning because it is interpreted as
    // 2147483648 (which doesn't fit in an integer) to which a negation
    // is applied.
    test_num<int>("-2147483648", true, -2147483647 - 1);
    test_num<int>("-2147483649", false, 0);
    test_num<int>("-0", true, 0);
    test_num<int>("1.1", false, 0);

    test_num<long long>("123", true, 123);
    test_num<long long>("9223372036854775807", true, 9223372036854775807LL);
    test_num<long long>("9223372036854775808", false, 0);
    
    std::cout << "test 4 passed" << std::endl;
}

void test5()
{
    session sql(backEnd, connectString);
    std::tm t;
    sql << "select maketime(19, 54, 52)", into(t);
    assert(t.tm_year == 100);
    assert(t.tm_mon == 0);
    assert(t.tm_mday == 1);
    assert(t.tm_hour == 19);
    assert(t.tm_min == 54);
    assert(t.tm_sec == 52);

    std::cout << "test 5 passed" << std::endl;
}

// TEXT and BLOB types support test.
void test6()
{
    session sql(backEnd, connectString);
    std::string a("asdfg\0hjkl", 10);
    std::string b("lkjhg\0fd\0\0sa\0", 13);
    std::string c("\\0aa\\0bb\\0cc\\0", 10);
    // The maximum length for TEXT and BLOB is 65536.
    std::string x(60000, 'X');
    std::string y(60000, 'Y');
    // The default max_allowed_packet value for a MySQL server is 1M,
    // so let's limit ourselves to 800k, even though the maximum length
    // for LONGBLOB is 4G.
    std::string z(800000, 'Z');

    sql << "create table soci_test (id int, text_value text, "
        "blob_value blob, longblob_value longblob)";
    sql << "insert into soci_test values (1, \'foo\', \'bar\', \'baz\')";
    sql << "insert into soci_test "
        << "values (2, \'qwerty\\0uiop\', \'zxcv\\0bnm\', "
        << "\'qwerty\\0uiop\\0zxcvbnm\\0\')";
    sql << "insert into soci_test values (3, :a, :b, :c)",
           use(a), use(b), use(c);
    sql << "insert into soci_test values (4, :x, :y, :z)",
           use(x), use(y), use(z);

    std::vector<std::string> text_vec(100);
    std::vector<std::string> blob_vec(100);
    std::vector<std::string> longblob_vec(100);
    sql << "select text_value, blob_value, longblob_value "
        << "from soci_test order by id",
           into(text_vec), into(blob_vec), into(longblob_vec);
    assert(text_vec.size() == 4);
    assert(blob_vec.size() == 4);
    assert(longblob_vec.size() == 4);
    assert(text_vec[0] == "foo");
    assert(blob_vec[0] == "bar");
    assert(longblob_vec[0] == "baz");
    assert(text_vec[1] == std::string("qwerty\0uiop", 11));
    assert(blob_vec[1] == std::string("zxcv\0bnm", 8));
    assert(longblob_vec[1] == std::string("qwerty\0uiop\0zxcvbnm\0", 20));
    assert(text_vec[2] == a);
    assert(blob_vec[2] == b);
    assert(longblob_vec[2] == c);
    assert(text_vec[3] == x);
    assert(blob_vec[3] == y);
    assert(longblob_vec[3] == z);

    std::string text, blob, longblob;
    sql << "select text_value, blob_value, longblob_value "
        << "from soci_test where id = 1",
           into(text), into(blob), into(longblob);
    assert(text == "foo");
    assert(blob == "bar");
    assert(longblob == "baz");
    sql << "select text_value, blob_value, longblob_value "
        << "from soci_test where id = 2",
           into(text), into(blob), into(longblob);
    assert(text == std::string("qwerty\0uiop", 11));
    assert(blob == std::string("zxcv\0bnm", 8));
    assert(longblob == std::string("qwerty\0uiop\0zxcvbnm\0", 20));
    sql << "select text_value, blob_value, longblob_value "
        << "from soci_test where id = 3",
           into(text), into(blob), into(longblob);
    assert(text == a);
    assert(blob == b);
    assert(longblob == c);
    sql << "select text_value, blob_value, longblob_value "
        << "from soci_test where id = 4",
           into(text), into(blob), into(longblob);
    assert(text == x);
    assert(blob == y);
    assert(longblob == z);

    rowset<row> rs =
        (sql.prepare << "select text_value, blob_value, longblob_value "
                        "from soci_test order by id");
    rowset<row>::const_iterator r = rs.begin();
    assert(r->get_properties(0).get_data_type() == dt_string);
    assert(r->get<std::string>(0) == "foo");
    assert(r->get_properties(1).get_data_type() == dt_string);
    assert(r->get<std::string>(1) == "bar");
    assert(r->get_properties(2).get_data_type() == dt_string);
    assert(r->get<std::string>(2) == "baz");
    ++r;
    assert(r->get_properties(0).get_data_type() == dt_string);
    assert(r->get<std::string>(0) == std::string("qwerty\0uiop", 11));
    assert(r->get_properties(1).get_data_type() == dt_string);
    assert(r->get<std::string>(1) == std::string("zxcv\0bnm", 8));
    assert(r->get_properties(2).get_data_type() == dt_string);
    assert(r->get<std::string>(2) ==
           std::string("qwerty\0uiop\0zxcvbnm\0", 20));
    ++r;
    assert(r->get_properties(0).get_data_type() == dt_string);
    assert(r->get<std::string>(0) == a);
    assert(r->get_properties(1).get_data_type() == dt_string);
    assert(r->get<std::string>(1) == b);
    assert(r->get_properties(2).get_data_type() == dt_string);
    assert(r->get<std::string>(2) == c);
    ++r;
    assert(r->get_properties(0).get_data_type() == dt_string);
    assert(r->get<std::string>(0) == x);
    assert(r->get_properties(1).get_data_type() == dt_string);
    assert(r->get<std::string>(1) == y);
    assert(r->get_properties(2).get_data_type() == dt_string);
    assert(r->get<std::string>(2) == z);
    ++r;
    assert(r == rs.end());

    sql << "drop table soci_test";

    std::cout << "test 6 passed" << std::endl;
}

// test for number of affected rows

struct integer_value_table_creator : table_creator_base
{
    integer_value_table_creator(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val integer)";
    }
};

void test7()
{
    {
        session sql(backEnd, connectString);

        integer_value_table_creator tableCreator(sql);

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

    std::cout << "test 7 passed" << std::endl;
}


// The prepared statements should survive session::reconnect().
void test8()
{
  {
    session sql(backEnd, connectString);

    integer_value_table_creator tableCreator(sql);

    int i;
    statement st = (sql.prepare
        << "insert into soci_test(val) values(:val)", use(i));
    i = 5;
    st.execute(true);

    sql.reconnect();

    i = 6;
    st.execute(true);

    sql.close();
    sql.reconnect();

    i = 7;
    st.execute(true);

    std::vector<int> v(5);
    sql << "select val from soci_test order by val", into(v);
    assert(v.size() == 3);
    assert(v[0] == 5);
    assert(v[1] == 6);
    assert(v[2] == 7);
  }

  std::cout << "test 8 passed" << std::endl;
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "tm datetime, i1 integer, i2 integer, i3 integer, "
                 "name varchar(20)) type=InnoDB";
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

    std::string to_date_time(std::string const &datdt_string) const
    {
        return "\'" + datdt_string + "\'";
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
        std::exit(1);
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
        test3();
        test4();
        test5();
        test6();
        test7();
        test8();

        std::cout << "\nOK, all tests passed.\n\n";
        return EXIT_SUCCESS;
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }

    return EXIT_FAILURE;
}
