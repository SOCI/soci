//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "common-tests.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

// MS SQL-specific tests
TEST_CASE("MS SQL long string", "[odbc][mssql][long]")
{
    soci::session sql(backEnd, connectString);

    struct long_text_table_creator : public table_creator_base
    {
        explicit long_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            // Notice that 4000 is the maximal length of an nvarchar() column,
            // at least when using FreeTDS ODBC driver.
            sql << "create table soci_test ("
                        "long_text nvarchar(max) null, "
                        "fixed_text nvarchar(4000) null"
                    ")";
        }
    } long_text_table_creator(sql);

    // Build a string at least 8000 characters long to test that it survives
    // the round trip unscathed.
    std::ostringstream os;
    for ( int n = 0; n < 1000; ++n )
    {
        os << "Line #" << n << "\n";
    }

    std::string const str_in = os.str();
    CHECK_NOTHROW((
        sql << "insert into soci_test(long_text) values(:str)", use(str_in)
    ));

    std::string str_out;
    sql << "select long_text from soci_test", into(str_out);

    // Don't just compare the strings because the error message in case they
    // differ is completely unreadable due to their size, so give a better
    // error in the common failure case.
    if (str_out.length() != str_in.length())
    {
        FAIL("Read back string of length " << str_out.length() <<
             " instead of expected " << str_in.length());
    }
    else
    {
        CHECK(str_out == str_in);
    }

    // The long string should be truncated when inserting it into a fixed size
    // column.
    CHECK_THROWS_AS(
        (sql << "insert into soci_test(fixed_text) values(:str)", use(str_in)),
        soci_error
    );
}

TEST_CASE("MS SQL wide string", "[odbc][mssql][widestring]")
{
    soci::session sql(backEnd, connectString);

    struct wide_text_table_creator : public table_creator_base
    {
        explicit wide_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_text nvarchar(40) null"
                ")";
        }
    } wide_text_table_creator(sql);

    std::wstring const str_in = L"Hello, SOCI!";

    sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

    std::wstring str_out;
    sql << "select wide_text from soci_test", into(str_out);

    CHECK(str_out == str_in);

}

TEST_CASE("MS SQL wide string vector", "[odbc][mssql][vector][widestring]")
{
    soci::session sql(backEnd, connectString);

    struct wide_text_table_creator : public table_creator_base
    {
        explicit wide_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_text nvarchar(40) null"
                ")";
        }
    } wide_text_table_creator(sql);

    std::vector<std::wstring> const str_in = {
        L"Hello, SOCI!",
        L"Hello, World!",
        L"Hello, Universe!",
        L"Hello, Galaxy!"
    };

    sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

    std::vector<std::wstring> str_out(4);

    sql << "select wide_text from soci_test", into(str_out);

    CHECK(str_out.size() == str_in.size());
    for (std::size_t i = 0; i != str_in.size(); ++i)
    {
        CHECK(str_out[i] == str_in[i]);
    }
}

TEST_CASE("MS SQL wide char", "[odbc][mssql][wchar]")
{
    soci::session sql(backEnd, connectString);

    struct wide_char_table_creator : public table_creator_base
    {
        explicit wide_char_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_char nchar(2) null"
                ")";
        }
    } wide_char_table_creator(sql);

    wchar_t const ch_in = L'X';

    sql << "insert into soci_test(wide_char) values(:str)", use(ch_in);

    wchar_t ch_out;
    sql << "select wide_char from soci_test", into(ch_out);

    CHECK(ch_out == ch_in);
}

TEST_CASE("MS SQL wchar vector", "[odbc][mssql][vector][wchar]")
{
    soci::session sql(backEnd, connectString);

    struct wide_char_table_creator : public table_creator_base
    {
        explicit wide_char_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                "wide_char nchar(2) null"
                ")";
        }
    } wide_char_table_creator(sql);

    std::vector<wchar_t> const ch_in = {
        L'A',
        L'B',
        L'C',
        L'D'
    };

    sql << "insert into soci_test(wide_char) values(:str)", use(ch_in);

    std::vector<wchar_t> ch_out(4);

    sql << "select wide_char from soci_test", into(ch_out);

    CHECK(ch_out.size() == ch_in.size());
    for (std::size_t i = 0; i != ch_in.size(); ++i)
    {
        CHECK(ch_out[i] == ch_in[i]);
    }
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ll bigint, ul numeric(20), "
                 "d float, num76 numeric(7,6), "
                 "tm datetime, i1 integer, i2 integer, i3 integer, "
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

struct table_creator_for_clob : table_creator_base
{
    table_creator_for_clob(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, s text)";
    }
};

struct table_creator_for_xml : table_creator_base
{
    table_creator_for_xml(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, x xml)";
    }
};

struct table_creator_for_get_last_insert_id : table_creator_base
{
    table_creator_for_get_last_insert_id(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test (id integer identity(1, 1), val integer)";
    }
};

//
// Support for SOCI Common Tests
//

class test_context : public test_context_base
{
public:
    test_context(backend_factory const &backend,
                std::string const &connstr)
        : test_context_base(backend, connstr) {}

    table_creator_base* table_creator_1(soci::session& s) const override
    {
        return new table_creator_one(s);
    }

    table_creator_base* table_creator_2(soci::session& s) const override
    {
        return new table_creator_two(s);
    }

    table_creator_base* table_creator_3(soci::session& s) const override
    {
        return new table_creator_three(s);
    }

    table_creator_base * table_creator_4(soci::session& s) const override
    {
        return new table_creator_for_get_affected_rows(s);
    }

    tests::table_creator_base* table_creator_clob(soci::session& s) const override
    {
        return new table_creator_for_clob(s);
    }

    tests::table_creator_base* table_creator_xml(soci::session& s) const override
    {
        return new table_creator_for_xml(s);
    }

    tests::table_creator_base* table_creator_get_last_insert_id(soci::session& s) const override
    {
        return new table_creator_for_get_last_insert_id(s);
    }

    bool has_real_xml_support() const override
    {
        return true;
    }

    std::string to_date_time(std::string const &datdt_string) const override
    {
        return "convert(datetime, \'" + datdt_string + "\', 120)";
    }

    bool has_multiple_select_bug() const override
    {
        // MS SQL does support MARS (multiple active result sets) since 2005
        // version, but this support needs to be explicitly enabled and is not
        // implemented in FreeTDS ODBC driver used under Unix currently, so err
        // on the side of caution and suppose that it's not supported.
        return true;
    }

    std::string sql_length(std::string const& s) const override
    {
        return "len(" + s + ")";
    }
};

int main(int argc, char** argv)
{
#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc >= 2 && argv[1][0] != '-')
    {
        connectString = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }
    else
    {
        connectString = "FILEDSN=./test-mssql.dsn";
    }

    test_context tc(backEnd, connectString);

    return Catch::Session().run(argc, argv);
}
