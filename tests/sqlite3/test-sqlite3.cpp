//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include "test-context.h"

#include <catch.hpp>

#include <cstdint>
#include <cstdio>
#include <forward_list>
#include <limits>
#include <thread>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_sqlite3();

TEST_CASE("SQLite connection string", "[sqlite][connstring]")
{
    CHECK_THROWS_WITH(soci::session(backEnd, ""),
                      Catch::Contains("Database name must be specified"));
    CHECK_THROWS_WITH(soci::session(backEnd, "readonly=1"),
                      Catch::Contains("Database name must be specified"));

    CHECK_THROWS_WITH(soci::session(backEnd, "readonly=\""),
                      Catch::Contains("Expected closing quote '\"'"));
    CHECK_THROWS_WITH(soci::session(backEnd, "readonly=maybe"),
                      Catch::Contains("Invalid value"));

    CHECK_THROWS_WITH(soci::session(backEnd, "db=no-such-file nocreate=1"),
                      Catch::Contains("Cannot establish connection"));

    CHECK_NOTHROW(soci::session(backEnd, "dbname=:memory: nocreate"));
    CHECK_NOTHROW(soci::session(backEnd, "dbname=:memory: foreign_keys=on"));

    // Also check an alternative way of specifying the connection parameters.
    connection_parameters params(backEnd, "dbname=still-no-such-file");
    params.set_option("foreign_keys", "1");
    params.set_option("nocreate", "1");
    CHECK_THROWS_WITH(soci::session(params),
                      Catch::Contains("Cannot establish connection"));

    // Finally allow testing arbitrary connection strings by specifying them in
    // the environment variables.
    if (auto const connstr = std::getenv("SOCI_TEST_CONNSTR_GOOD"))
    {
        CHECK_NOTHROW(soci::session(backEnd, connstr));
    }

    if (auto const connstr = std::getenv("SOCI_TEST_CONNSTR_BAD"))
    {
        CHECK_THROWS_AS(soci::session(backEnd, connstr), soci_error);
    }

}

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
    explicit SetupForeignKeys(soci::session& sql)
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

TEST_CASE("SQLite foreign keys", "[sqlite][foreignkeys]")
{
    soci::session sql(backEnd, connectString);

    SetupForeignKeys setupForeignKeys(sql);

    SECTION("Off")
    {
        sql << "pragma foreign_keys = off";

        sql << "delete from parent where id = 1";

        int parent = 0;
        sql << "select parent from child where id = 100 ", into(parent);
        CHECK(parent == 1);
    }

    SECTION("On")
    {
        sql << "pragma foreign_keys = on";

        try
        {
            sql << "delete from parent where id = 1";

            FAIL("Expected exception not thrown");
        }
        catch (sqlite3_soci_error const& e)
        {
            CHECK_THAT(e.what(), Catch::Contains(
                          "FOREIGN KEY constraint failed while executing "
                          "\"delete from parent where id = 1\"."));

            CHECK( e.get_error_category() == soci_error::constraint_violation );
            CHECK( e.result() == 19 /* SQLITE_CONSTRAINT */ );
            CHECK( e.extended_result() == 787 /* SQLITE_CONSTRAINT_FOREIGNKEY */ );
        }
    }
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

// Test the DDL and metadata functionality
TEST_CASE("SQLite DDL with metadata", "[sqlite][ddl]")
{
    if (sqlite3_session_backend::libversion_number() < 3036000) {
        if (sqlite3_session_backend::libversion_number() < 3014000) {
            WARN("SQLite requires at least version 3.14.0 for column description, detected " << sqlite3_session_backend::libversion());
        }
        WARN("SQLite requires at least version 3.36.0 for drop column, detected " << sqlite3_session_backend::libversion());
        return;
    }
    soci::session sql(backEnd, connectString);

    // note: prepare_column_descriptions expects l-value
    std::string ddl_t1 = "DDL_T1";
    std::string ddl_t2 = "DDL_T2";
    std::string ddl_t3 = "DDL_T3";

    // single-expression variant:
    sql.create_table(ddl_t1).column("I", soci::dt_integer).column("J", soci::dt_integer);

    // check whether this table was created:

    bool ddl_t1_found = false;
    bool ddl_t2_found = false;
    bool ddl_t3_found = false;
    std::string table_name;
    soci::statement st = (sql.prepare_table_names(), into(table_name));
    st.execute();
    while (st.fetch())
    {
        if (table_name == ddl_t1) { ddl_t1_found = true; }
        if (table_name == ddl_t2) { ddl_t2_found = true; }
        if (table_name == ddl_t3) { ddl_t3_found = true; }
    }

    CHECK(ddl_t1_found);
    CHECK(ddl_t2_found == false);
    CHECK(ddl_t3_found == false);

    // check whether ddl_t1 has the right structure:

    bool i_found = false;
    bool j_found = false;
    bool other_found = false;
    soci::column_info ci;
    soci::statement st1 = (sql.prepare_column_descriptions(ddl_t1), into(ci));
    st1.execute();
    while (st1.fetch())
    {
        if (ci.name == "I")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable);
            i_found = true;
        }
        else if (ci.name == "J")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable);
            j_found = true;
        }
        else
        {
            other_found = true;
        }
    }

    CHECK(i_found);
    CHECK(j_found);
    CHECK(other_found == false);

    // two more tables:

    // separately defined columns:
    // (note: statement is executed when ddl object goes out of scope)
    {
        soci::ddl_type ddl = sql.create_table(ddl_t2);
        ddl.column("I", soci::dt_integer);
        ddl.column("J", soci::dt_integer);
        ddl.column("K", soci::dt_integer)("not null");
        ddl.primary_key("t2_pk", "J");
    }

    sql.add_column(ddl_t1, "K", soci::dt_integer);
    sql.add_column(ddl_t1, "BIG", soci::dt_string, 0); // "unlimited" length -> CLOB
    sql.drop_column(ddl_t1, "I");

    // or with constraint as in t2:
    sql.add_column(ddl_t2, "M", soci::dt_integer)("not null");

    // third table with a foreign key to the second one
    {
        soci::ddl_type ddl = sql.create_table(ddl_t3);
        ddl.column("X", soci::dt_integer);
        ddl.column("Y", soci::dt_integer);
        ddl.foreign_key("t3_fk", "X", ddl_t2, "J");
    }

    // check if all tables were created:

    ddl_t1_found = false;
    ddl_t2_found = false;
    ddl_t3_found = false;
    soci::statement st2 = (sql.prepare_table_names(), into(table_name));
    st2.execute();
    while (st2.fetch())
    {
        if (table_name == ddl_t1) { ddl_t1_found = true; }
        if (table_name == ddl_t2) { ddl_t2_found = true; }
        if (table_name == ddl_t3) { ddl_t3_found = true; }
    }

    CHECK(ddl_t1_found);
    CHECK(ddl_t2_found);
    CHECK(ddl_t3_found);

    // check if ddl_t1 has the right structure (it was altered):

    i_found = false;
    j_found = false;
    bool k_found = false;
    bool big_found = false;
    other_found = false;
    soci::statement st3 = (sql.prepare_column_descriptions(ddl_t1), into(ci));
    st3.execute();
    while (st3.fetch())
    {
        if (ci.name == "J")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable);
            j_found = true;
        }
        else if (ci.name == "K")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable);
            k_found = true;
        }
        else if (ci.name == "BIG")
        {
            CHECK(ci.type == soci::dt_string);
            CHECK(ci.precision == 0); // "unlimited" for strings
            big_found = true;
        }
        else
        {
            other_found = true;
        }
    }

    CHECK(i_found == false);
    CHECK(j_found);
    CHECK(k_found);
    CHECK(big_found);
    CHECK(other_found == false);

    // check if ddl_t2 has the right structure:

    i_found = false;
    j_found = false;
    k_found = false;
    bool m_found = false;
    other_found = false;
    soci::statement st4 = (sql.prepare_column_descriptions(ddl_t2), into(ci));
    st4.execute();
    while (st4.fetch())
    {
        if (ci.name == "I")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable);
            i_found = true;
        }
        else if (ci.name == "J")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable == true); // primary key -> SQLite default behavior
            j_found = true;
        }
        else if (ci.name == "K")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable == false);
            k_found = true;
        }
        else if (ci.name == "M")
        {
            CHECK(ci.type == soci::dt_integer);
            CHECK(ci.nullable == false);
            m_found = true;
        }
        else
        {
            other_found = true;
        }
    }

    CHECK(i_found);
    CHECK(j_found);
    CHECK(k_found);
    CHECK(m_found);
    CHECK(other_found == false);

    sql.drop_table(ddl_t1);
    sql.drop_table(ddl_t3); // note: this must be dropped before ddl_t2
    sql.drop_table(ddl_t2);

    // check if all tables were dropped:

    ddl_t1_found = false;
    ddl_t2_found = false;
    ddl_t3_found = false;
    st2 = (sql.prepare_table_names(), into(table_name));
    st2.execute();
    while (st2.fetch())
    {
        if (table_name == ddl_t1) { ddl_t1_found = true; }
        if (table_name == ddl_t2) { ddl_t2_found = true; }
        if (table_name == ddl_t3) { ddl_t3_found = true; }
    }

    CHECK(ddl_t1_found == false);
    CHECK(ddl_t2_found == false);
    CHECK(ddl_t3_found == false);
}


// Helpers for the DDL roundtrip test below.
namespace soci
{

// Helper used by test_roundtrip() below which collects all round trip test
// data and allows to define a type conversion for it.
template<typename T>
struct Roundtrip
{
    typedef T val_type;
    Roundtrip(soci::db_type type, T val)
        : inType(type), inVal(val) {}

    soci::db_type inType;
    T inVal;

    soci::db_type outType;
    T outVal;
};

// Test a rountrip insertion data to the current database for the arithmetic type T
// This test specifically use the dynamic bindings and the DDL creation statements.
template<typename T>
struct type_conversion<Roundtrip<T>>
{
    static_assert(std::is_arithmetic<T>::value, "Roundtrip currently supported only for numeric types");
    typedef soci::values base_type;
    static void from_base(soci::values const &v, soci::indicator, Roundtrip<T> &t)
    {
        t.outType = v.get_properties(0).get_db_type();
        switch (t.outType)
        {
            case soci::db_int8:   t.outVal = static_cast<T>(v.get<std::int8_t>(0));   break;
            case soci::db_uint8:  t.outVal = static_cast<T>(v.get<std::uint8_t>(0));  break;
            case soci::db_int16:  t.outVal = static_cast<T>(v.get<std::int16_t>(0));  break;
            case soci::db_uint16: t.outVal = static_cast<T>(v.get<std::uint16_t>(0)); break;
            case soci::db_int32:  t.outVal = static_cast<T>(v.get<std::int32_t>(0));  break;
            case soci::db_uint32: t.outVal = static_cast<T>(v.get<std::uint32_t>(0)); break;
            case soci::db_int64:  t.outVal = static_cast<T>(v.get<std::int64_t>(0));  break;
            case soci::db_uint64: t.outVal = static_cast<T>(v.get<std::uint64_t>(0)); break;
            case soci::db_double: t.outVal = static_cast<T>(v.get<double>(0));        break;
            default: FAIL_CHECK("Unsupported type mapped to db_type"); break;
        }
    }
    static void to_base(Roundtrip<T> const &t, soci::values &v, soci::indicator&)
    {
        v.set("VAL", t.inVal);
    }
};

template<typename T>
void check(soci::Roundtrip<T> const &val)
{
    CHECK(val.inType == val.outType);
    CHECK(val.inVal == val.outVal);
}

template<>
void check(soci::Roundtrip<double> const &val)
{
    CHECK(val.inType == val.outType);
    CHECK(std::fpclassify(val.inVal) == std::fpclassify(val.outVal));
    if (std::isnormal(val.inVal) && std::isnormal(val.outVal))
        CHECK_THAT(val.inVal, Catch::Matchers::WithinRel(val.outVal));
}

template<typename T>
void test_roundtrip(soci::session &sql, soci::db_type inputType, T inputVal)
{
    try
    {
        Roundtrip<T> tester(inputType, inputVal);

        const std::string table = "TEST_ROUNDTRIP";
        sql.create_table(table).column("VAL", tester.inType);
        struct table_dropper
        {
            table_dropper(soci::session& sql, std::string const& table)
                : sql_(sql), table_(table) {}
            ~table_dropper() { sql_ << "DROP TABLE " << table_; }

            soci::session& sql_;
            const std::string table_;
        } dropper(sql, table);

        sql << "INSERT INTO " << table << "(VAL) VALUES (:VAL)", soci::use(const_cast<const Roundtrip<T>&>(tester));
        soci::statement stmt = (sql.prepare << "SELECT * FROM " << table);
        stmt.exchange(soci::into(tester));
        stmt.define_and_bind();
        stmt.execute();
        stmt.fetch();
        check(tester);
    }
    catch (const std::exception& e)
    {
        FAIL_CHECK(e.what());
    }
}

} // namespace soci

TEST_CASE("SQLite DDL roundrip", "[sqlite][ddl][roundtrip]")
{
    soci::session sql(backEnd, connectString);
    test_roundtrip(sql, soci::db_double, std::numeric_limits<double>::max());
    test_roundtrip(sql, soci::db_int8,   std::numeric_limits<std::int8_t>::max());
    test_roundtrip(sql, soci::db_int16,  std::numeric_limits<std::int16_t>::max());
    test_roundtrip(sql, soci::db_int32,  std::numeric_limits<std::int32_t>::max());
    test_roundtrip(sql, soci::db_int64,  std::numeric_limits<std::int64_t>::max());
    test_roundtrip(sql, soci::db_uint8,  std::numeric_limits<std::uint8_t>::max());
    test_roundtrip(sql, soci::db_uint16, std::numeric_limits<std::uint16_t>::max());
    test_roundtrip(sql, soci::db_uint32, std::numeric_limits<std::uint32_t>::max());
    test_roundtrip(sql, soci::db_uint64, std::numeric_limits<std::uint64_t>::max());
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

// This is a regression test case for https://github.com/SOCI/soci/issues/1190
// The core issue here is that SQLite lacks strict type checking, which allows to
// store arbitrarily large integers in a column, regardless of whether the column's
// type was declared as int, bigint, smallint, etc.
// This naturally confuses SOCI as it expects type safety. The issue only appears with
// dynamic (row-based) APIs as in this case, SOCI needs to supply an internal temporary
// into which to select the queried data.
// By trusting the column type, SOCI could end up choosing an integer type that is too
// small for the queried value, leading to a silent integer overflow and hence unexpected
// query results.
// This test case effectively ensures that this no longer happens.
//
// Note that this test is SQLite-specific because with the other backends we'd
// fail to insert the value in the first place.
TEST_CASE("SQLite row int64", "[sqlite][row][int64]")
{
    soci::session sql(backEnd, connectString);
    struct integer_table_creator : table_creator_base
    {
        integer_table_creator(soci::session &sql)
            : table_creator_base(sql)
        {
            sql << "create table soci_test(id integer primary key, val integer)";
        }
    } creator(sql);

    int id = 1;
    std::int64_t val = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());
    sql << "insert into soci_test(id, val) values (:id, :val)", use(id), use(val);

    // As long as we don't overflow the 32bit integer, we can select with an int32_t
    row r;
    sql << "SELECT val FROM soci_test WHERE id = :id", use(id), into(r);
    CHECK(r.get_properties("val").get_db_type() == db_int32);
    CHECK(r.size() == 1);
    CHECK(r.get<std::int32_t>("val") == val);

    val += 1;
    id += 1;
    sql << "insert into soci_test(id, val) values (:id, :val)", use(id), use(val);

    sql << "SELECT val FROM soci_test WHERE id = :id", use(id), into(r);
    CHECK(r.size() == 1);
    CHECK(r.get_properties("val").get_db_type() == db_int32);
    // This query would overflow the 32bit int -> an exception is thrown
    REQUIRE_THROWS(r.get<std::int32_t>("val"));
    // Selecting as int64_t instead works
    CHECK(r.get<std::int64_t>("val") == val);
}

// The setting "synchronous" cannot be set when the database is locked in
// environments with parallelisms for example. A timeout solves this issue.
// This test checks whether the timeout was applied before setting "synchronous".
TEST_CASE("SQLite synchronous option works from multiple threads",
          "[sqlite][pragma]")
{
    {
        soci::session sql(backEnd, "db=test.db");
        sql << R"(PRAGMA journal_mode="WAL")";
    }

    struct FileRemover
    {
        // Argument must be a literal string as we just keep the pointer.
        explicit FileRemover(char const* fileName) : fileName_(fileName) {}
        ~FileRemover() { std::remove(fileName_); }

        char const* const fileName_;
    };

    FileRemover fileRemoverDB("test.db");
    FileRemover fileRemoverSHM("test.db-shm");
    FileRemover fileRemoverWAL("test.db-wal");

    std::forward_list<std::thread> threads;
    for (int i = 0; i < 32; ++i)
    {
        threads.emplace_front([]() -> void
        {
            REQUIRE_NOTHROW(
                soci::session(backEnd, "db=test.db synchronous=extra timeout=2")
            );
        });
    }

    for (auto& thr : threads)
    {
        thr.join();
    }
}

// DDL Creation objects for common tests
struct table_creator_one : public table_creator_base
{
    table_creator_one(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ll bigint, ul unsigned bigint, "
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
        // The CHECK clause is needed to make SQLite refuse inserting "a" into
        // this column: the test using this table relies on this to fail and
        // this condition ensures it does.
        //
        // Note that more straightforward checks, like typeof(val) = 'integer',
        // don't work with old SQLite version, such as 3.12 used on AppVeyor.
        sql << R"(create table soci_test(val integer check (val < 100)))";
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

struct table_creator_for_blob : public tests::table_creator_base
{
    table_creator_for_blob(soci::session & sql)
		: tests::table_creator_base(sql)
    {
        sql << "create table soci_test(id integer, b blob)";
    }
};

class test_context : public test_context_common
{
public:
    test_context() = default;

    bool initialize_connect_string(std::string argFromCommandLine) override
    {
        // Unlike most other backends, we have a reasonable default value for
        // the connection string, so initialize it with it to use in-memory
        // database if nothing is specified on the command line.
        if (argFromCommandLine.empty())
        {
            // Enable FKs by default to make SQLite behaviour more compatible
            // with the other backends.
            argFromCommandLine = "db=:memory: foreign_keys=on";
        }

        return test_context_base::initialize_connect_string(argFromCommandLine);
    }

    std::string get_backend_name() const override
    {
        return "sqlite3";
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

    table_creator_base* table_creator_4(soci::session& s) const override
    {
        return new table_creator_for_get_affected_rows(s);
    }

    table_creator_base* table_creator_get_last_insert_id(soci::session& s) const override
    {
        return new table_creator_from_str(s,
            "create table soci_test (id integer primary key, val integer)");
    }

    table_creator_base* table_creator_blob(soci::session& s) const override
    {
      return new table_creator_for_blob(s);
    }

    table_creator_base* table_creator_xml(soci::session& s) const override
    {
        return new table_creator_from_str(s,
            "create table soci_test (id integer, x text)");
    }

    std::string to_date_time(std::string const &datdt_string) const override
    {
        return "datetime(\'" + datdt_string + "\')";
    }

    bool has_fp_bug() const override
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

    bool has_uint64_storage_bug() const override
    {
        // SQLite processes integers as 8-byte signed values. Values bigger
        // than INT64_MAX therefore overflow and are stored as negative values.
        return true;
    }

    bool enable_std_char_padding(soci::session&) const override
    {
        // SQLite does not support right padded char type.
        return false;
    }

    std::string sql_length(std::string const& s) const override
    {
        return "length(" + s + ")";
    }
};

test_context tc_sqlite3;
