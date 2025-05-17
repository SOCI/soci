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

struct wide_text_table_creator : public table_creator_base
{
  explicit wide_text_table_creator(soci::session &sql)
      : table_creator_base(sql)
  {
      sql << "create table soci_test ("
                  "wide_text nvarchar(40) null"
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
    for (auto it = rs.begin(); it != rs.end(); ++it)
    {
        soci::row const& row = *it;
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
        return;
    }

    FAIL("No tables found in the master database");
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
};

test_context tc_odbc_mssql;
