//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "test-context.h"
#include <string>
#include <vector>
#include <ctime>
#include <cmath>

#include <catch.hpp>

#include <fmt/format.h>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

// MS SQL-specific tests

// SQL Server ends the transaction on its own in some cases, e.g. when it
// chooses the connection as a deadlock victim or, as done here, when an error
// occurs with XACT_ABORT on: ending the transaction must still work and leave
// the connection usable for the next ones.
TEST_CASE("MS SQL transaction ended by the server", "[odbc][mssql][transaction]")
{
    soci::session sql(backEnd, connectString);

    struct id_table_creator : public table_creator_base
    {
        explicit id_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test (id int primary key)";
        }
    } id_table_creator(sql);

    auto const get_trancount = [&sql]()
    {
        int trancount = -1;
        sql << "select @@trancount", into(trancount);
        return trancount;
    };

    {
        soci::transaction tr(sql);
        sql << "insert into soci_test values (1)";
        // Each statement is executed in a batch of its own, which would
        // restore the XACT_ABORT setting at its end if it were set separately.
        CHECK_THROWS_AS((sql << "set xact_abort on; select 1/0"), soci_error);

        // Whether ending the transaction fails depends on the driver: FreeTDS
        // reports that there is no transaction any more, while Microsoft ODBC
        // Driver opens a new one as soon as the server ends the previous one.
        // Either way, the connection must remain usable, including for rolling
        // back, as soci::transaction dtor does after a failed commit.
        auto const check_ended_by_server = [](soci_error const& e)
        {
            CHECK_THAT(e.what(), Catch::Contains("already ended by the server"));
        };

        SECTION("rollback")
        {
            try
            {
                tr.rollback();
            }
            catch (soci_error const& e)
            {
                check_ended_by_server(e);
            }
        }

        SECTION("commit")
        {
            try
            {
                tr.commit();
            }
            catch (soci_error const& e)
            {
                check_ended_by_server(e);
            }
        }
    }

    {
        soci::transaction tr(sql);
        sql << "insert into soci_test values (2)";
        CHECK(get_trancount() == 1);
        tr.rollback();
    }
    CHECK(get_trancount() == 0);

    {
        soci::transaction tr(sql);
        sql << "insert into soci_test values (3)";
        tr.commit();
    }

    std::vector<int> ids(10);
    sql << "select id from soci_test", into(ids);
    CHECK(ids == std::vector<int>{3});
}

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
    std::string str_in;
    str_in.reserve(16000);
    for ( int n = 0; n < 1000; ++n )
    {
        str_in += fmt::format("Line #{}\n", n);
    }

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

    // Also check reading it into a vector and as wide strings.
    std::vector<std::string> vec_out(1);
    sql << "select long_text from soci_test", into(vec_out);
    REQUIRE(vec_out.size() == 1);
    CHECK(vec_out[0].length() == str_in.length());
    CHECK(vec_out[0] == str_in);

    std::wstring const wstr_in(str_in.begin(), str_in.end());

    std::wstring wstr_out;
    sql << "select long_text from soci_test", into(wstr_out);
    CHECK(wstr_out.length() == wstr_in.length());
    CHECK(wstr_out == wstr_in);

    std::vector<std::wstring> wvec_out(1);
    sql << "select long_text from soci_test", into(wvec_out);
    REQUIRE(wvec_out.size() == 1);
    CHECK(wvec_out[0].length() == wstr_in.length());
    CHECK(wvec_out[0] == wstr_in);

    // The long string should be truncated when inserting it into a fixed size
    // column.
    CHECK_THROWS_AS(
        (sql << "insert into soci_test(fixed_text) values(:str)", use(str_in)),
        soci_error
    );
}

struct wide_text_table_creator : public table_creator_base
{
  explicit wide_text_table_creator(soci::session &sql)
      : table_creator_base(sql)
  {
      sql << "create table soci_test ("
                  "wide_text nvarchar(17) null"
              ")";
  }
};

TEST_CASE("MS SQL wide string", "[odbc][mssql][wstring]")
{
  soci::session sql(backEnd, connectString);

  wide_text_table_creator create_wide_text_table(sql);

  std::wstring const str_in = L"Привет, SOCI!";

  sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

  std::wstring str_out;
  sql << "select wide_text from soci_test", into(str_out);

  CHECK(str_out == str_in);

  soci::rowset<soci::row> rs = (sql.prepare << "select wide_text from soci_test");
  auto it = rs.begin();
  REQUIRE( it != rs.end() );

  CHECK( it->get<std::wstring>(0) == str_in );

  ++it;
  CHECK( it == rs.end() );
}

TEST_CASE("MS SQL wide string vector", "[odbc][mssql][vector][wstring]")
{
  soci::session sql(backEnd, connectString);

  wide_text_table_creator create_wide_text_table(sql);

  std::vector<std::wstring> const str_in = {
      L"Привет, SOCI!",
      L"Привет, World!",
      L"Привет, Universe!",
      L"Привет, Galaxy!"};

  sql << "insert into soci_test(wide_text) values(:str)", use(str_in);

  std::vector<std::wstring> str_out(4);

  sql << "select wide_text from soci_test", into(str_out);

  CHECK(str_out.size() == str_in.size());
  for (std::size_t i = 0; i != str_in.size(); ++i)
  {
    CHECK(str_out[i] == str_in[i]);
  }
}

// Non-ASCII characters can take more bytes in the client encoding than the
// column size, check that they are not truncated when fetching them.
TEST_CASE("MS SQL non-ASCII short strings", "[odbc][mssql][unicode]")
{
    soci::session sql(backEnd, connectString);

    struct short_text_table_creator : table_creator_base
    {
        explicit short_text_table_creator(soci::session& sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test ("
                        "id int, "
                        "v varchar(10) collate Latin1_General_CI_AS null, "
                        "nv nvarchar(10) null"
                    ")";
        }
    } short_text_table_creator(sql);

    // Use SQL functions to create the values to avoid depending on the
    // encoding used for the query or parameters: the values use E-acute
    // (taking 2 bytes in UTF-8 but just 1 in CP1252 used for VARCHAR column)
    // and Euro sign (3 bytes in UTF-8 and 2 in UTF-16).
    sql << "insert into soci_test(id, v, nv) values("
            "1, replicate(char(233), 10), replicate(nchar(8364), 10))";
    sql << "insert into soci_test(id, v, nv) values(2, 'abc', N'abc')";
    sql << "insert into soci_test(id, v, nv) values(3, NULL, NULL)";

    // The encoding of the narrow strings depends on the driver and its
    // configuration (and also the platform), and we can only check them if
    // it's UTF-8, so check for this by retrieving a known non-ASCII string.
    //
    // Note that we use a long column for this, to ensure that this check
    // doesn't depend on the correct handling of the short ones.
    std::string euro;
    sql << "select cast(nchar(8364) as nvarchar(max))", into(euro);
    bool const isUTF8 = euro == "\xe2\x82\xac";

    // Hardcoded UTF-8 representations of E-acute (U+00E9) and Euro (U+20AC).
    std::string expectedV, expectedNV;
    for (int n = 0; n < 10; ++n)
    {
        expectedV += "\xc3\xa9";
        expectedNV += "\xe2\x82\xac";
    }

    std::wstring const wexpectedV(10, L'é');
    std::wstring const wexpectedNV(10, L'€');

    SECTION("scalar wide")
    {
        std::wstring wv, wnv;
        sql << "select v, nv from soci_test where id = 1", into(wv), into(wnv);
        CHECK(wv == wexpectedV);
        CHECK(wnv == wexpectedNV);
    }

    SECTION("vector wide")
    {
        std::vector<indicator> indV(3), indNV(3);
        std::vector<std::wstring> wv(3), wnv(3);
        sql << "select v, nv from soci_test order by id",
               into(wv, indV), into(wnv, indNV);
        REQUIRE(wv.size() == 3);
        REQUIRE(wnv.size() == 3);

        CHECK(wv[0] == wexpectedV);
        CHECK(wnv[0] == wexpectedNV);
        CHECK(wv[1] == L"abc");
        CHECK(wnv[1] == L"abc");
        CHECK(indV[2] == i_null);
        CHECK(indNV[2] == i_null);
    }

    // Note that we must not return from inside a section, as this would
    // prevent Catch from discovering the sections following it, so put the
    // warning into its own section and return only after it.
    if (!isUTF8)
    {
        SECTION("narrow")
        {
            WARN("Skipping narrow strings tests as client encoding is not UTF-8");
        }
        return;
    }

    SECTION("scalar")
    {
        std::string v, nv;
        sql << "select v, nv from soci_test where id = 1", into(v), into(nv);
        CHECK(v == expectedV);
        CHECK(nv == expectedNV);
    }

    SECTION("scalar after long column")
    {
        // The columns following a long one are retrieved using SQLGetData()
        // instead of being bound, so check that this works too.
        std::string l, v, nv;
        sql << "select cast(nv as nvarchar(max)), v, nv "
               "from soci_test where id = 1",
               into(l), into(v), into(nv);
        CHECK(l == expectedNV);
        CHECK(v == expectedV);
        CHECK(nv == expectedNV);
    }

    SECTION("vector")
    {
        std::vector<std::string> v(3), nv(3);
        std::vector<indicator> indV(3), indNV(3);
        sql << "select v, nv from soci_test order by id",
               into(v, indV), into(nv, indNV);
        REQUIRE(v.size() == 3);
        REQUIRE(nv.size() == 3);

        CHECK(v[0] == expectedV);
        CHECK(nv[0] == expectedNV);
        CHECK(v[1] == "abc");
        CHECK(nv[1] == "abc");
        CHECK(indV[2] == i_null);
        CHECK(indNV[2] == i_null);
    }
}

TEST_CASE("MS SQL table records count", "[odbc][mssql][count]")
{
    soci::session sql(backEnd, connectString);

    // Execute the provided SQL query to count records in tables
    std::string sql_query = R"(
        SET NOCOUNT ON;
        DECLARE db_cursor CURSOR FOR
        SELECT name FROM sys.databases
        WHERE state_desc = 'ONLINE'
        AND name IN ('master');
        DECLARE @DatabaseName NVARCHAR(128);
        DECLARE @outset TABLE(
            INSTANCENAME varchar(50),
            DATABASENAME varchar(100),
            TABLENAME varchar(100),
            NUMBEROFRECORDS_I bigint
        );
        OPEN db_cursor;
        FETCH NEXT FROM db_cursor INTO @DatabaseName;
        WHILE @@FETCH_STATUS = 0
        BEGIN
            DECLARE @command nvarchar(1000) = 'USE ' + QUOTENAME(@DatabaseName) +
            '; SELECT @@SERVERNAME, DB_NAME(), T.NAME, P.[ROWS] FROM sys.tables T ' +
            'INNER JOIN sys.indexes I ON T.OBJECT_ID = I.OBJECT_ID ' +
            'INNER JOIN sys.partitions P ON I.OBJECT_ID = P.OBJECT_ID AND I.INDEX_ID = P.INDEX_ID ' +
            'INNER JOIN sys.allocation_units A ON P.PARTITION_ID = A.CONTAINER_ID ' +
            'WHERE T.NAME NOT LIKE ''DT%'' AND I.OBJECT_ID > 255 AND I.INDEX_ID <= 1 ' +
            'GROUP BY T.NAME, I.OBJECT_ID, I.INDEX_ID, I.NAME, P.[ROWS] ' +
            'ORDER BY OBJECT_NAME(I.OBJECT_ID)';
            INSERT INTO @outset EXEC (@command)
            FETCH NEXT FROM db_cursor INTO @DatabaseName
        END
        CLOSE db_cursor
        DEALLOCATE db_cursor
        SELECT INSTANCENAME, DATABASENAME, TABLENAME, NUMBEROFRECORDS_I
        FROM @outset;
    )";

    soci::rowset<soci::row> rs = (sql.prepare << sql_query);

    // Check that we can access the results.
    bool hasTables = false;
    for (soci::row const& row : rs)
    {
        std::string instance_name = row.get<std::string>(0);
        std::string database_name = row.get<std::string>(1);
        std::string table_name = row.get<std::string>(2);
        long long number_of_records = row.get<long long>(3);

        // Use the variables above to avoid warnings about unused variables and
        // check the only one of them that we can be sure about because we have
        // "name IN ('master')" in the SQL query above.
        INFO("Table " << instance_name << "." << table_name <<
             " has " << number_of_records << " records");
        CHECK( database_name == "master" );
        hasTables = true;
    }

    CHECK(hasTables);
}

struct table_creator_for_sequence : table_creator_base
{
    table_creator_for_sequence(soci::session & sql)
        : table_creator_base(sql, "seqtest")
    {
        sql << "create sequence seqtest start with 101 increment by 1";
    }
};

TEST_CASE("next sequence value", "[odbc][mssql][get_next_sequence_value()]")
{
    soci::session sql(backEnd, connectString);
    table_creator_for_sequence tableCreator(sql);

    long long val = -1;
    CHECK(sql.get_next_sequence_value("seqtest", val));
    CHECK(val == 101);

    CHECK(sql.get_next_sequence_value("seqtest", val));
    CHECK(val == 102);
}

TEST_CASE("MS SQL connect timeout", "[odbc][mssql][connect][timeout]")
{
    connection_parameters params(backEnd, connectString);

    auto const getLoginTimeout = [](soci::session& sql)
    {
        auto const backend = dynamic_cast<odbc_session_backend*>(sql.get_backend());
        REQUIRE(backend);

        SQLUINTEGER timeout = 0;
        SQLRETURN const rc = SQLGetConnectAttr(backend->hdbc_,
                                               SQL_ATTR_LOGIN_TIMEOUT,
                                               &timeout, SQL_IS_UINTEGER,
                                               nullptr);
        if (is_odbc_error(rc))
        {
            throw odbc_soci_error(SQL_HANDLE_DBC, backend->hdbc_,
                                  "getting login timeout");
        }

        return timeout;
    };

    // Use unusual values below to make sure they're not the driver default.

    SECTION("Valid timeout")
    {
        params.set_option(option_connect_timeout, "17");

        soci::session sql(params);
        CHECK(getLoginTimeout(sql) == 17);

        // Check that the option is preserved when reconnecting, which uses a
        // new connection handle.
        sql.close();
        sql.reconnect();
        CHECK(getLoginTimeout(sql) == 17);
    }

    SECTION("Invalid timeout")
    {
        for (auto const& value : {"", "-1", "1s", "abc", "4294967296", "99999999999999999999"})
        {
            INFO("Timeout value: \"" << value << "\"");

            params.set_option(option_connect_timeout, value);
            CHECK_THROWS_WITH(soci::session(params),
                              Catch::Contains("Invalid connection timeout"));
        }
    }

    SECTION("Timeout in connection string")
    {
        soci::session sql(backEnd, connectString + ";odbc.connect_timeout=19");
        CHECK(getLoginTimeout(sql) == 19);

        sql.close();
        sql.reconnect();
        CHECK(getLoginTimeout(sql) == 19);
    }

    SECTION("Timeout in connection string takes precedence over option")
    {
        params.set_connect_string(connectString + ";odbc.connect_timeout=19");
        params.set_option(option_connect_timeout, "17");

        soci::session sql(params);
        CHECK(getLoginTimeout(sql) == 19);
    }

    SECTION("Invalid timeout in connection string")
    {
        CHECK_THROWS_WITH(soci::session(backEnd,
                                        connectString + ";odbc.connect_timeout=abc"),
                          Catch::Contains("Invalid connection timeout"));
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

class test_context : public test_context_common
{
public:
    test_context() = default;

    std::string get_example_connection_string() const override
    {
        return "FILEDSN=./test-mssql.dsn";
    }

    std::string get_backend_name() const override
    {
        return "odbc";
    }

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

    // Note that the functions below require VIEW SERVER STATE and ALTER ANY
    // CONNECTION permissions.
    std::string get_session_id_query() const override
    {
        return "select cast(@@spid as varchar(10))";
    }

    std::string get_kill_session_statement(std::string const& id) const override
    {
        return "kill " + id;
    }

    std::string get_session_alive_query(std::string const& id) const override
    {
        return "select count(*) from sys.dm_exec_sessions "
               "where session_id = " + id;
    }

    std::string get_connect_string_disabling_autoreconnect() const override
    {
        // Recent Microsoft drivers reopen a lost connection on their own by
        // default, which would hide its loss.
        return get_connect_string() + ";ConnectRetryCount=0";
    }
};

test_context tc_odbc_mssql;
