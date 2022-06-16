//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include "common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstring>
#include <ctime>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_sqlite3();

// ROWID test
// In sqlite3 the row id can be called ROWID, _ROWID_ or oid
TEST_CASE("SQLite rowid", "[sqlite][rowid][oid]")
{
    soci::session sql(backEnd, connectString);

    try { sql << "drop table test1"; }
    catch (soci_error const &) {} // ignore if error

    sql <<
    "create table test1 ("
    "    id integer,"
    "    name varchar(100)"
    ")";

    sql << "insert into test1(id, name) values(7, \'John\')";

    rowid rid(sql);
    sql << "select oid from test1 where id = 7", into(rid);

    int id;
    std::string name;

    sql << "select id, name from test1 where oid = :rid",
    into(id), into(name), use(rid);

    CHECK(id == 7);
    CHECK(name == "John");

    sql << "drop table test1";
}

class SetupForeignKeys
{
public:
    SetupForeignKeys(soci::session& sql)
        : m_sql(sql)
    {
        m_sql <<
        "create table parent ("
        "    id integer primary key"
        ")";

        m_sql <<
        "create table child ("
        "    id integer primary key,"
        "    parent integer,"
        "    foreign key(parent) references parent(id)"
        ")";

        m_sql << "insert into parent(id) values(1)";
        m_sql << "insert into child(id, parent) values(100, 1)";
    }

    ~SetupForeignKeys()
    {
        m_sql << "drop table child";
        m_sql << "drop table parent";
    }

private:
    SetupForeignKeys(const SetupForeignKeys&);
    SetupForeignKeys& operator=(const SetupForeignKeys&);

    soci::session& m_sql;
};

TEST_CASE("SQLite foreign keys are disabled by default", "[sqlite][foreignkeys]")
{
    soci::session sql(backEnd, connectString);

    SetupForeignKeys setupForeignKeys(sql);

    sql << "delete from parent where id = 1";

    int parent = 0;
    sql << "select parent from child where id = 100 ", into(parent);

    CHECK(parent == 1);
}

TEST_CASE("SQLite foreign keys are enabled by foreign_keys option", "[sqlite][foreignkeys]")
{
    soci::session sql(backEnd, "dbname=:memory: foreign_keys=on");

    SetupForeignKeys setupForeignKeys(sql);

    CHECK_THROWS_WITH(sql << "delete from parent where id = 1",
                      "sqlite3_statement_backend::loadOne: FOREIGN KEY "
                      "constraint failed while executing "
                      "\"delete from parent where id = 1\".");
}

class SetupAutoIncrementTable
{
public:
    SetupAutoIncrementTable(soci::session& sql)
        : m_sql(sql)
    {
        m_sql <<
        "create table t("
        "    id integer primary key autoincrement,"
        "    name text"
        ")";
    }

    ~SetupAutoIncrementTable()
    {
        m_sql << "drop table t";
    }

private:
    SetupAutoIncrementTable(const SetupAutoIncrementTable&);
    SetupAutoIncrementTable& operator=(const SetupAutoIncrementTable&);

    soci::session& m_sql;
};

TEST_CASE("SQLite get_last_insert_id works with AUTOINCREMENT",
          "[sqlite][rowid]")
{
    soci::session sql(backEnd, connectString);
    SetupAutoIncrementTable createTable(sql);

    sql << "insert into t(name) values('x')";
    sql << "insert into t(name) values('y')";

    long long val;
    sql.get_last_insert_id("t", val);
    CHECK(val == 2);
}

TEST_CASE("SQLite get_last_insert_id with AUTOINCREMENT does not reuse IDs when rows deleted",
          "[sqlite][rowid]")
{
    soci::session sql(backEnd, connectString);
    SetupAutoIncrementTable createTable(sql);

    sql << "insert into t(name) values('x')";
    sql << "insert into t(name) values('y')";

    sql << "delete from t where id = 2";

    long long val;
    sql.get_last_insert_id("t", val);
    CHECK(val == 2);
}

class SetupNoAutoIncrementTable
{
public:
    SetupNoAutoIncrementTable(soci::session& sql)
        : m_sql(sql)
    {
        m_sql <<
        "create table t("
        "    id integer primary key,"
        "    name text"
        ")";
    }

    ~SetupNoAutoIncrementTable()
    {
        m_sql << "drop table t";
    }

private:
    SetupNoAutoIncrementTable(const SetupNoAutoIncrementTable&);
    SetupNoAutoIncrementTable& operator=(const SetupNoAutoIncrementTable&);

    soci::session& m_sql;
};

TEST_CASE("SQLite get_last_insert_id without AUTOINCREMENT reuses IDs when rows deleted",
          "[sqlite][rowid]")
{
    soci::session sql(backEnd, connectString);
    SetupNoAutoIncrementTable createTable(sql);

    sql << "insert into t(name) values('x')";
    sql << "insert into t(name) values('y')";

    sql << "delete from t where id = 2";

    long long val;
    sql.get_last_insert_id("t", val);
    CHECK(val == 1);
}

TEST_CASE("SQLite get_last_insert_id throws if table not found",
          "[sqlite][rowid]")
{
    soci::session sql(backEnd, connectString);

    long long val;
    CHECK_THROWS(sql.get_last_insert_id("notexisting", val));
}

class SetupTableWithDoubleQuoteInName
{
public:
    SetupTableWithDoubleQuoteInName(soci::session& sql)
        : m_sql(sql)
    {
        m_sql <<
        "create table \"t\"\"fff\"("
        "    id integer primary key,"
        "    name text"
        ")";
    }

    ~SetupTableWithDoubleQuoteInName()
    {
        m_sql << "drop table \"t\"\"fff\"";
    }

private:
    SetupTableWithDoubleQuoteInName(const SetupTableWithDoubleQuoteInName&);
    SetupTableWithDoubleQuoteInName& operator=(const SetupTableWithDoubleQuoteInName&);

    soci::session& m_sql;
};

TEST_CASE("SQLite get_last_insert_id escapes table name",
          "[sqlite][rowid]")
{
    soci::session sql(backEnd, connectString);
    SetupTableWithDoubleQuoteInName table(sql);

    long long val;
    sql.get_last_insert_id("t\"fff", val);
    CHECK(val == 0);
}

// BLOB test
struct blob_table_creator : public table_creator_base
{
    blob_table_creator(soci::session & sql)
        : table_creator_base(sql)
    {
        sql <<
            "create table soci_test ("
            "    id integer,"
            "    img blob"
            ")";
    }
};

TEST_CASE("SQLite blob", "[sqlite][blob]")
{
    soci::session sql(backEnd, connectString);

    blob_table_creator tableCreator(sql);

    char buf[] = "abcdefghijklmnopqrstuvwxyz";

    sql << "insert into soci_test(id, img) values(7, '')";

    {
        blob b(sql);

        sql << "select img from soci_test where id = 7", into(b);
        CHECK(b.get_len() == 0);

        b.write(0, buf, sizeof(buf));
        CHECK(b.get_len() == sizeof(buf));
        sql << "update soci_test set img=? where id = 7", use(b);

        b.append(buf, sizeof(buf));
        CHECK(b.get_len() == 2 * sizeof(buf));
        sql << "insert into soci_test(id, img) values(8, ?)", use(b);
    }
    {
        blob b(sql);
        sql << "select img from soci_test where id = 8", into(b);
        CHECK(b.get_len() == 2 * sizeof(buf));
        char buf2[100];
        b.read(0, buf2, 10);
        CHECK(std::strncmp(buf2, "abcdefghij", 10) == 0);

        sql << "select img from soci_test where id = 7", into(b);
        CHECK(b.get_len() == sizeof(buf));

    }
}

// This test was put in to fix a problem that occurs when there are both
// into and use elements in the same query and one of them (into) binds
// to a vector object.

struct test3_table_creator : table_creator_base
{
    test3_table_creator(soci::session & sql) : table_creator_base(sql)
    {
        sql << "create table soci_test( id integer, name varchar, subname varchar);";
    }
};

TEST_CASE("SQLite use and vector into", "[sqlite][use][into][vector]")
{
    soci::session sql(backEnd, connectString);

    test3_table_creator tableCreator(sql);

    sql << "insert into soci_test(id,name,subname) values( 1,'john','smith')";
    sql << "insert into soci_test(id,name,subname) values( 2,'george','vals')";
    sql << "insert into soci_test(id,name,subname) values( 3,'ann','smith')";
    sql << "insert into soci_test(id,name,subname) values( 4,'john','grey')";
    sql << "insert into soci_test(id,name,subname) values( 5,'anthony','wall')";

    {
        std::vector<int> v(10);

        statement s(sql.prepare << "Select id from soci_test where name = :name");

        std::string name = "john";

        s.exchange(use(name, "name"));
        s.exchange(into(v));

        s.define_and_bind();
        s.execute(true);

        CHECK(v.size() == 2);
    }
}


// Test case from Amnon David 11/1/2007
// I've noticed that table schemas in SQLite3 can sometimes have typeless
// columns. One (and only?) example is the sqlite_sequence that sqlite
// creates for autoincrement . Attempting to traverse this table caused
// SOCI to crash. I've made the following code change in statement.cpp to
// create a workaround:

struct test4_table_creator : table_creator_base
{
    test4_table_creator(soci::session & sql) : table_creator_base(sql)
    {
        sql << "create table soci_test (col INTEGER PRIMARY KEY AUTOINCREMENT, name char)";
    }
};

TEST_CASE("SQLite select from sequence", "[sqlite][sequence]")
{
    // we need to have an table that uses autoincrement to test this.
    soci::session sql(backEnd, connectString);

    test4_table_creator tableCreator(sql);

    sql << "insert into soci_test(name) values('john')";
    sql << "insert into soci_test(name) values('james')";

    {
        int key;
        std::string name;
        sql << "select * from soci_test", into(key), into(name);
        CHECK(name == "john");

        rowset<row> rs = (sql.prepare << "select * from sqlite_sequence");
        rowset<row>::const_iterator it = rs.begin();
        row const& r1 = (*it);
        CHECK(r1.get<std::string>(0) == "soci_test");
        CHECK(r1.get<std::string>(1) == "2");
    }
}

struct longlong_table_creator : table_creator_base
{
    longlong_table_creator(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(val number(20))";
    }
};

// long long test
TEST_CASE("SQLite long long", "[sqlite][longlong]")
{
    soci::session sql(backEnd, connectString);

    longlong_table_creator tableCreator(sql);

    long long v1 = 1000000000000LL;
    sql << "insert into soci_test(val) values(:val)", use(v1);

    long long v2 = 0LL;
    sql << "select val from soci_test", into(v2);

    CHECK(v2 == v1);
}

TEST_CASE("SQLite vector long long", "[sqlite][vector][longlong]")
{
    soci::session sql(backEnd, connectString);

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

    REQUIRE(v2.size() == 5);
    CHECK(v2[0] == 1000000000004LL);
    CHECK(v2[1] == 1000000000003LL);
    CHECK(v2[2] == 1000000000002LL);
    CHECK(v2[3] == 1000000000001LL);
    CHECK(v2[4] == 1000000000000LL);
}

struct type_inference_table_creator : table_creator_base
{
    type_inference_table_creator(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(cvc varchar (10), cdec decimal (20), "
               "cll bigint, cull unsigned bigint, clls big int, culls unsigned big int)";
    }
};

// test for correct type inference form sqlite column type
TEST_CASE("SQLite type inference", "[sqlite][sequence]")
{
    soci::session sql(backEnd, connectString);

    type_inference_table_creator tableCreator(sql);

    std::string cvc = "john";
    double cdec = 12345.0;  // integers can be stored precisely in IEEE 754
    long long cll = 1000000000003LL;
    unsigned long long cull = 1000000000004ULL;

    sql << "insert into soci_test(cvc, cdec, cll, cull, clls, culls) values(:cvc, :cdec, :cll, :cull, :clls, :culls)",
        use(cvc), use(cdec), use(cll), use(cull), use(cll), use(cull);

    {
        rowset<row> rs = (sql.prepare << "select * from soci_test");
        rowset<row>::const_iterator it = rs.begin();
        row const& r1 = (*it);
        CHECK(r1.get<std::string>(0) == cvc);
        CHECK(r1.get<double>(1) == Approx(cdec));
        CHECK(r1.get<long long>(2) == cll);
        CHECK(r1.get<unsigned long long>(3) == cull);
        CHECK(r1.get<long long>(4) == cll);
        CHECK(r1.get<unsigned long long>(5) == cull);
    }
}

TEST_CASE("SQLite DDL wrappers", "[sqlite][ddl]")
{
    soci::session sql(backEnd, connectString);

    int i = -1;
    sql << "select length(" + sql.empty_blob() + ")", into(i);
    CHECK(i == 0);
    sql << "select " + sql.nvl() + "(1, 2)", into(i);
    CHECK(i == 1);
    sql << "select " + sql.nvl() + "(NULL, 2)", into(i);
    CHECK(i == 2);
}

struct table_creator_for_get_last_insert_id : table_creator_base
{
    table_creator_for_get_last_insert_id(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer primary key autoincrement)";
        sql << "insert into soci_test (id) values (41)";
        sql << "delete from soci_test where id = 41";
    }
};

TEST_CASE("SQLite last insert id", "[sqlite][last-insert-id]")
{
    soci::session sql(backEnd, connectString);
    table_creator_for_get_last_insert_id tableCreator(sql);
    sql << "insert into soci_test default values";
    long long id;
    bool result = sql.get_last_insert_id("soci_test", id);
    CHECK(result == true);
    CHECK(id == 42);
}

struct table_creator_for_std_tm_bind : table_creator_base
{
    table_creator_for_std_tm_bind(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(date datetime)";
        sql << "insert into soci_test (date) values ('2017-04-04 00:00:00')";
        sql << "insert into soci_test (date) values ('2017-04-04 12:00:00')";
        sql << "insert into soci_test (date) values ('2017-04-05 00:00:00')";
    }
};

TEST_CASE("SQLite std::tm bind", "[sqlite][std-tm-bind]")
{
    soci::session sql(backEnd, connectString);
    table_creator_for_std_tm_bind tableCreator(sql);

    std::time_t datetimeEpoch = 1491307200; // 2017-04-04 12:00:00

    std::tm datetime = *std::gmtime(&datetimeEpoch);
    soci::rowset<std::tm> rs = (sql.prepare << "select date from soci_test where date=:dt", soci::use(datetime));

    std::vector<std::tm> result;
    std::copy(rs.begin(), rs.end(), std::back_inserter(result));
    REQUIRE(result.size() == 1);
    result.front().tm_isdst = 0;
    CHECK(std::mktime(&result.front()) == std::mktime(&datetime));
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ul numeric(20), d float, "
                 "num76 numeric(7,6), "
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

// Originally, submitted to SQLite3 backend and later moved to common test.
// Test commit b394d039530f124802d06c3b1a969c3117683152
// Author: Mika Fischer <mika.fischer@zoopnet.de>
// Date:   Thu Nov 17 13:28:07 2011 +0100
// Implement get_affected_rows for SQLite3 backend
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

struct table_creator_from_str : table_creator_base
{
    table_creator_from_str(soci::session & sql, std::string const& sqlStr)
        : table_creator_base(sql)
    {
        sql << sqlStr;
    }
};

class test_context : public test_context_base
{
public:
    test_context(backend_factory const &backend,
                std::string const &connstr)
        : test_context_base(backend, connstr) {}

    table_creator_base* table_creator_1(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_one(s);
    }

    table_creator_base* table_creator_2(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_two(s);
    }

    table_creator_base* table_creator_3(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_three(s);
    }

    table_creator_base* table_creator_4(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_for_get_affected_rows(s);
    }

    table_creator_base* table_creator_get_last_insert_id(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_from_str(s,
            "create table soci_test (id integer primary key, val integer)");
    }

    table_creator_base* table_creator_xml(soci::session& s) const SOCI_OVERRIDE
    {
        return new table_creator_from_str(s,
            "create table soci_test (id integer, x text)");
    }

    std::string to_date_time(std::string const &datdt_string) const SOCI_OVERRIDE
    {
        return "datetime(\'" + datdt_string + "\')";
    }

    bool has_fp_bug() const SOCI_OVERRIDE
    {
        /*
            SQLite seems to be buggy when using text conversion, e.g.:

                 % echo 'create table t(f real); \
                         insert into t(f) values(1.79999999999999982); \
                         select * from t;' | sqlite3
                 1.8

            And there doesn't seem to be any way to avoid this rounding, so we
            have no hope of getting back exactly what we write into it unless,
            perhaps, we start using sqlite3_bind_double() in the backend code.
         */

        return true;
    }

    bool enable_std_char_padding(soci::session&) const SOCI_OVERRIDE
    {
        // SQLite does not support right padded char type.
        return false;
    }

    std::string sql_length(std::string const& s) const SOCI_OVERRIDE
    {
        return "length(" + s + ")";
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
        // If no file name is specfied then work in-memory
        connectString = ":memory:";
    }

    test_context tc(backEnd, connectString);

    return Catch::Session().run(argc, argv);
}
