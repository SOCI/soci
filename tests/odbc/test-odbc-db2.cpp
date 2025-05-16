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
        sql << "CREATE TABLE SOCI_TEST(ID INTEGER, VAL SMALLINT, C CHAR, STR VARCHAR(20), SH SMALLINT, LL BIGINT, UL NUMERIC(20), "
            "D DOUBLE, NUM76 NUMERIC(7,6), "
            "TM TIMESTAMP(9), I1 INTEGER, I2 INTEGER, I3 INTEGER, NAME VARCHAR(20))";
    }
};

struct table_creator_two : public table_creator_base
{
    table_creator_two(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "CREATE TABLE SOCI_TEST(NUM_FLOAT DOUBLE, NUM_INT INTEGER, NAME VARCHAR(20), SOMETIME TIMESTAMP, CHR CHAR)";
    }
};

struct table_creator_three : public table_creator_base
{
    table_creator_three(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "CREATE TABLE SOCI_TEST(NAME VARCHAR(100) NOT NULL, PHONE VARCHAR(15))";
    }
};

struct table_creator_for_get_affected_rows : table_creator_base
{
    table_creator_for_get_affected_rows(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "CREATE TABLE SOCI_TEST(VAL INTEGER)";
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
        return "DSN=<db>;Uid=<user>;Pwd=<password>";
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

    std::string to_date_time(std::string const &datdt_string) const
    {
        return "\'" + datdt_string + "\'";
    }

    virtual std::string sql_length(std::string const& s) const
    {
        return "length(" + s + ")";
    }
};

struct table_creator_bigint : table_creator_base
{
    table_creator_bigint(soci::session & sql)
        : table_creator_base(sql)
    {
        sql << "CREATE TABLE SOCI_TEST (VAL BIGINT)";
    }
};

TEST_CASE("ODBC/DB2 long long", "[odbc][db2][longlong]")
{
    const int num_recs = 100;
    soci::session sql(backEnd, connectString);
    table_creator_bigint table(sql);

    {
        long long n;
        statement st = (sql.prepare <<
            "INSERT INTO SOCI_TEST (VAL) VALUES (:val)", use(n));
        for (int i = 0; i < num_recs; i++)
        {
            n = 1000000000LL + i;
            st.execute();
        }
    }
    {
        long long n2;
        statement st = (sql.prepare <<
            "SELECT VAL FROM SOCI_TEST ORDER BY VAL", into(n2));
        st.execute();
        for (int i = 0; i < num_recs; i++)
        {
            st.fetch();
            CHECK(n2 == 1000000000LL + i);
        }
    }
}

TEST_CASE("ODBC/DB2 unsigned long long", "[odbc][db2][unsigned][longlong]")
{
    const int num_recs = 100;
    soci::session sql(backEnd, connectString);
    table_creator_bigint table(sql);

    {
        unsigned long long n;
        statement st = (sql.prepare <<
            "INSERT INTO SOCI_TEST (VAL) VALUES (:val)", use(n));
        for (int i = 0; i < num_recs; i++)
        {
            n = 1000000000LL + i;
            st.execute();
        }
    }
    {
        unsigned long long n2;
        statement st = (sql.prepare <<
            "SELECT VAL FROM SOCI_TEST ORDER BY VAL", into(n2));
        st.execute();
        for (int i = 0; i < num_recs; i++)
        {
            st.fetch();
            CHECK(n2 == 1000000000LL + i);
        }
    }
}

TEST_CASE("ODBC/DB2 vector long long", "[odbc][db2][vector][longlong]")
{
    const std::size_t num_recs = 100;
    soci::session sql(backEnd, connectString);
    table_creator_bigint table(sql);

    {
        std::vector<long long> v(num_recs);
        for (std::size_t i = 0; i < num_recs; i++)
        {
            v[i] = 1000000000LL + i;
        }

        sql << "INSERT INTO SOCI_TEST (VAL) VALUES (:bi)", use(v);
    }
    {
        std::size_t recs = 0;

        std::vector<long long> v(num_recs / 2 + 1);
        statement st = (sql.prepare <<
            "SELECT VAL FROM SOCI_TEST ORDER BY VAL", into(v));
        st.execute();
        while (true)
        {
            if (!st.fetch())
            {
                break;
            }

            const std::size_t vsize = v.size();
            for (std::size_t i = 0; i < vsize; i++)
            {
                CHECK(v[i] == 1000000000LL +
                    static_cast<long long>(recs));
                recs++;
            }
        }
        CHECK(recs == num_recs);
    }
}

TEST_CASE("ODBC/DB2 vector unsigned long long", "[odbc][db2][vector][unsigned][longlong]")
{
    const std::size_t num_recs = 100;
    soci::session sql(backEnd, connectString);
    table_creator_bigint table(sql);

    {
        std::vector<unsigned long long> v(num_recs);
        for (std::size_t i = 0; i < num_recs; i++)
        {
            v[i] = 1000000000LL + i;
        }

        sql << "INSERT INTO SOCI_TEST (VAL) VALUES (:bi)", use(v);
    }
    {
        std::size_t recs = 0;

        std::vector<unsigned long long> v(num_recs / 2 + 1);
        statement st = (sql.prepare <<
            "SELECT VAL FROM SOCI_TEST ORDER BY VAL", into(v));
        st.execute();
        while (true)
        {
            if (!st.fetch())
            {
                break;
            }

            const std::size_t vsize = v.size();
            for (std::size_t i = 0; i < vsize; i++)
            {
                CHECK(v[i] == 1000000000LL +
                    static_cast<unsigned long long>(recs));
                recs++;
            }
        }
        CHECK(recs == num_recs);
    }

    std::cout << "test odbc_db2_unsigned_long_long_vector passed" << std::endl;
}

test_context tc_odbc_db2;
