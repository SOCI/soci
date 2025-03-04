//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#ifdef SOCI_HAVE_CXX17
#include "soci/std-optional.h"
#endif

#include "soci-compiler.h"

#include <catch.hpp>

#if defined(_MSC_VER) && (_MSC_VER < 1500)
#undef SECTION
#define SECTION(name) INTERNAL_CATCH_SECTION(name, "dummy-for-vc8")
#endif

#include <algorithm>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <typeinfo>
#include <type_traits>

#include "soci-mktime.h"

#include "test-assert.h"
#include "test-myint.h"
#include "test-padded.h"

namespace soci
{

namespace tests
{

// Define the test cases in their own namespace to avoid clashes with the test
// cases defined in individual backend tests: as only line number is used for
// building the name of the "anonymous" function by the TEST_CASE macro, we
// could have a conflict between a test defined here and in some backend if
// they happened to start on the same line.
namespace test_cases
{

inline bool operator== ( const std::tm& a, const std::tm& b )
{
    return a.tm_sec == b.tm_sec && a.tm_min == b.tm_min && a.tm_hour == b.tm_hour && a.tm_mday == b.tm_mday && a.tm_mon == b.tm_mon &&
           a.tm_year == b.tm_year && a.tm_wday == b.tm_wday && a.tm_yday == b.tm_yday && a.tm_isdst == b.tm_isdst;
}

TEST_CASE("timegm implementation", "[core][timegm]")
{
    std::tm t1;
    t1.tm_year = 105;
    t1.tm_mon = 13;     // + 1 year
    t1.tm_mday = 15;
    t1.tm_hour = 28;    // + 1 day
    t1.tm_min = 14;
    t1.tm_sec = 17;

    std::tm t2 = t1;

    const auto timegm_result = timegm (&t1);
    const auto timegm_soci_result = details::timegm_impl_soci (&t2);
    CHECK ( timegm_result == timegm_soci_result );
    CHECK ( t1.tm_year == 106 );
    CHECK ( t1.tm_mon == 1 );
    CHECK ( t1.tm_mday == 16 );
    CHECK ( t1.tm_hour == 4 );
    CHECK ( ( t1 == t2 ) );
}

TEST_CASE_METHOD(common_tests, "Exception on not connected", "[core][exception]")
{
    soci::session sql; // no connection

    // ensure connection is checked, no crash occurs
    CHECK_THROWS_AS(sql.begin(), soci_error);
    CHECK_THROWS_AS(sql.commit(), soci_error);
    CHECK_THROWS_AS(sql.rollback(), soci_error);
    CHECK_THROWS_AS(sql.get_backend_name(), soci_error);
    CHECK_THROWS_AS(sql.make_statement_backend(), soci_error);
    CHECK_THROWS_AS(sql.make_rowid_backend(), soci_error);
    CHECK_THROWS_AS(sql.make_blob_backend(), soci_error);

    std::string s;
    long long l;
    CHECK_THROWS_AS(sql.get_next_sequence_value(s, l), soci_error);
    CHECK_THROWS_AS(sql.get_last_insert_id(s, l), soci_error);
}

TEST_CASE_METHOD(common_tests, "Basic functionality", "[core][basics]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    CHECK_THROWS_AS(sql << "drop table soci_test_nosuchtable", soci_error);

    sql << "insert into soci_test (id) values (" << 123 << ")";
    int id;
    sql << "select id from soci_test", into(id);
    CHECK(id == 123);

    sql << "insert into soci_test (id) values (" << 234 << ")";
    sql << "insert into soci_test (id) values (" << 345 << ")";
    // Test prepare, execute, fetch correctness
    statement st = (sql.prepare << "select id from soci_test", into(id));
    st.execute();
    int count = 0;
    while(st.fetch())
        count++;
    CHECK(count == 3 );
    bool fetchEnd = st.fetch(); // All the data has been read here so additional fetch must return false
    CHECK(fetchEnd == false);
}

// "into" tests, type conversions, etc.
TEST_CASE_METHOD(common_tests, "Use and into", "[core][into]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    SECTION("Round trip works for char")
    {
        char c('a');
        sql << "insert into soci_test(c) values(:c)", use(c);
        sql << "select c from soci_test", into(c);
        CHECK(c == 'a');
    }

    SECTION("Round trip works for string")
    {
        std::string helloSOCI("Hello, SOCI!");
        sql << "insert into soci_test(str) values(:s)", use(helloSOCI);
        std::string str;
        sql << "select str from soci_test", into(str);
        CHECK(str == "Hello, SOCI!");
    }

    SECTION("Round trip works for short")
    {
        short three(3);
        sql << "insert into soci_test(sh) values(:id)", use(three);
        short sh(0);
        sql << "select sh from soci_test", into(sh);
        CHECK(sh == 3);
    }

    SECTION("Round trip works for int")
    {
        int five(5);
        sql << "insert into soci_test(id) values(:id)", use(five);
        int i(0);
        sql << "select id from soci_test", into(i);
        CHECK(i == 5);
    }

    SECTION("Round trip works for unsigned long")
    {
        unsigned long seven(7);
        sql << "insert into soci_test(ul) values(:ul)", use(seven);
        unsigned long ul(0);
        sql << "select ul from soci_test", into(ul);
        CHECK(ul == 7);
    }

    SECTION("Round trip works for double")
    {
        double pi(3.14159265);
        sql << "insert into soci_test(d) values(:d)", use(pi);
        double d(0.0);
        sql << "select d from soci_test", into(d);
        ASSERT_EQUAL(d, pi);
    }

    SECTION("Round trip works for date without time")
    {
        std::tm nov15 = std::tm();
        nov15.tm_year = 105;
        nov15.tm_mon = 10;
        nov15.tm_mday = 15;
        nov15.tm_hour = 0;
        nov15.tm_min = 0;
        nov15.tm_sec = 0;

        sql << "insert into soci_test(tm) values(:tm)", use(nov15);

        std::tm t = std::tm();
        sql << "select tm from soci_test", into(t);
        CHECK(t.tm_year == 105);
        CHECK(t.tm_mon  == 10);
        CHECK(t.tm_mday == 15);
        CHECK(t.tm_hour == 0);
        CHECK(t.tm_min  == 0);
        CHECK(t.tm_sec  == 0);
    }

    SECTION("Round trip works for date with time")
    {
        std::tm nov15 = std::tm();
        nov15.tm_year = 105;
        nov15.tm_mon = 10;
        nov15.tm_mday = 15;
        nov15.tm_hour = 22;
        nov15.tm_min = 14;
        nov15.tm_sec = 17;

        sql << "insert into soci_test(tm) values(:tm)", use(nov15);

        std::tm t = std::tm();
        sql << "select tm from soci_test", into(t);
        CHECK(t.tm_year == 105);
        CHECK(t.tm_mon  == 10);
        CHECK(t.tm_mday == 15);
        CHECK(t.tm_hour == 22);
        CHECK(t.tm_min  == 14);
        CHECK(t.tm_sec  == 17);
    }

    SECTION("Indicator is filled correctly in the simplest case")
    {
        int id(1);
        std::string str("Hello");
        sql << "insert into soci_test(id, str) values(:id, :str)",
            use(id), use(str);

        int i;
        indicator ind;
        sql << "select id from soci_test", into(i, ind);
        CHECK(ind == i_ok);
    }

    SECTION("Indicators work correctly more generally")
    {
        sql << "insert into soci_test(id,tm) values(NULL,NULL)";
        int i;
        indicator ind;
        sql << "select id from soci_test", into(i, ind);
        CHECK(ind == i_null);

        // additional test for NULL with std::tm
        std::tm t = std::tm();
        sql << "select tm from soci_test", into(t, ind);
        CHECK(ind == i_null);

        // indicator should be initialized even when nothing is found
        ind = i_ok;
        sql << "select id from soci_test where str='NO SUCH ROW'",
                into(i, ind);
        CHECK(ind == i_null);

        try
        {
            // expect error
            sql << "select id from soci_test", into(i);
            FAIL("expected exception not thrown");
        }
        catch (soci_error const &e)
        {
            CHECK(e.get_error_message() ==
                "Null value fetched and no indicator defined.");
            CHECK_THAT(e.what(),
                Catch::Contains("for the parameter number 1"));
        }

        sql << "select id from soci_test where id = 1000", into(i, ind);
        CHECK(sql.got_data() == false);

        // no data expected
        sql << "select id from soci_test where id = 1000", into(i);
        CHECK(sql.got_data() == false);

        // no data expected, test correct behaviour with use
        int id = 1000;
        sql << "select id from soci_test where id = :id", use(id), into(i);
        CHECK(sql.got_data() == false);
    }
}

// repeated fetch and bulk fetch
TEST_CASE_METHOD(common_tests, "Repeated and bulk fetch", "[core][bulk]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    SECTION("char")
    {
        char c;
        for (c = 'a'; c <= 'z'; ++c)
        {
            sql << "insert into soci_test(c) values(\'" << c << "\')";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 'z' - 'a' + 1);

        {
            char c2 = 'a';

            statement st = (sql.prepare <<
                "select c from soci_test order by c", into(c));

            st.execute();
            while (st.fetch())
            {
                CHECK(c == c2);
                ++c2;
            }
            CHECK(c2 == 'a' + count);
        }
        {
            char c2 = 'a';

            std::vector<char> vec(10);
            statement st = (sql.prepare <<
                "select c from soci_test order by c", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t i = 0; i != vec.size(); ++i)
                {
                    CHECK(c2 == vec[i]);
                    ++c2;
                }

                vec.resize(10);
            }
            CHECK(c2 == 'a' + count);
        }

        {
            // verify an exception is thrown when empty vector is used
            std::vector<char> vec;
            try
            {
                sql << "select c from soci_test", into(vec);
                FAIL("expected exception not thrown");
            }
            catch (soci_error const &e)
            {
                 CHECK(e.get_error_message() ==
                     "Vectors of size 0 are not allowed.");
            }
        }

    }

    // repeated fetch and bulk fetch of std::string
    SECTION("std::string")
    {
        int const rowsToTest = 10;
        for (int i = 0; i != rowsToTest; ++i)
        {
            std::ostringstream ss;
            ss << "Hello_" << i;

            sql << "insert into soci_test(str) values(\'"
                << ss.str() << "\')";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == rowsToTest);

        {
            int i = 0;
            std::string s;
            statement st = (sql.prepare <<
                "select str from soci_test order by str", into(s));

            st.execute();
            while (st.fetch())
            {
                std::ostringstream ss;
                ss << "Hello_" << i;
                CHECK(s == ss.str());
                ++i;
            }
            CHECK(i == rowsToTest);
        }
        {
            int i = 0;

            std::vector<std::string> vec(4);
            statement st = (sql.prepare <<
                "select str from soci_test order by str", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t j = 0; j != vec.size(); ++j)
                {
                    std::ostringstream ss;
                    ss << "Hello_" << i;
                    CHECK(ss.str() == vec[j]);
                    ++i;
                }

                vec.resize(4);
            }
            CHECK(i == rowsToTest);
        }
    }

    SECTION("short")
    {
        short const rowsToTest = 100;
        short sh;
        for (sh = 0; sh != rowsToTest; ++sh)
        {
            sql << "insert into soci_test(sh) values(" << sh << ")";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == rowsToTest);

        {
            short sh2 = 0;

            statement st = (sql.prepare <<
                "select sh from soci_test order by sh", into(sh));

            st.execute();
            while (st.fetch())
            {
                CHECK(sh == sh2);
                ++sh2;
            }
            CHECK(sh2 == rowsToTest);
        }
        {
            short sh2 = 0;

            std::vector<short> vec(8);
            statement st = (sql.prepare <<
                "select sh from soci_test order by sh", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t i = 0; i != vec.size(); ++i)
                {
                    CHECK(sh2 == vec[i]);
                    ++sh2;
                }

                vec.resize(8);
            }
            CHECK(sh2 == rowsToTest);
        }
    }

    SECTION("int")
    {
        int const rowsToTest = 100;
        int i;
        for (i = 0; i != rowsToTest; ++i)
        {
            sql << "insert into soci_test(id) values(" << i << ")";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == rowsToTest);

        {
            int i2 = 0;

            statement st = (sql.prepare <<
                "select id from soci_test order by id", into(i));

            st.execute();
            while (st.fetch())
            {
                CHECK(i == i2);
                ++i2;
            }
            CHECK(i2 == rowsToTest);
        }
        {
            // additional test with the use element

            int i2 = 0;
            int cond = 0; // this condition is always true

            statement st = (sql.prepare <<
                "select id from soci_test where id >= :cond order by id",
                use(cond), into(i));

            st.execute();
            while (st.fetch())
            {
                CHECK(i == i2);
                ++i2;
            }
            CHECK(i2 == rowsToTest);
        }
        {
            int i2 = 0;

            std::vector<int> vec(8);
            statement st = (sql.prepare <<
                "select id from soci_test order by id", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t n = 0; n != vec.size(); ++n)
                {
                    CHECK(i2 == vec[n]);
                    ++i2;
                }

                vec.resize(8);
            }
            CHECK(i2 == rowsToTest);
        }
    }

    SECTION("unsigned int")
    {
        unsigned int const rowsToTest = 100;
        unsigned int ul;
        for (ul = 0; ul != rowsToTest; ++ul)
        {
            sql << "insert into soci_test(ul) values(" << ul << ")";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == static_cast<int>(rowsToTest));

        {
            unsigned int ul2 = 0;

            statement st = (sql.prepare <<
                "select ul from soci_test order by ul", into(ul));

            st.execute();
            while (st.fetch())
            {
                CHECK(ul == ul2);
                ++ul2;
            }
            CHECK(ul2 == rowsToTest);
        }
        {
            unsigned int ul2 = 0;

            std::vector<unsigned int> vec(8);
            statement st = (sql.prepare <<
                "select ul from soci_test order by ul", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t i = 0; i != vec.size(); ++i)
                {
                    CHECK(ul2 == vec[i]);
                    ++ul2;
                }

                vec.resize(8);
            }
            CHECK(ul2 == rowsToTest);
        }
    }

    SECTION("unsigned long long")
    {
        unsigned int const rowsToTest = 100;
        unsigned long long ul;
        for (ul = 0; ul != rowsToTest; ++ul)
        {
            sql << "insert into soci_test(ul) values(" << ul << ")";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == static_cast<int>(rowsToTest));

        {
            unsigned long long ul2 = 0;

            statement st = (sql.prepare <<
                "select ul from soci_test order by ul", into(ul));

            st.execute();
            while (st.fetch())
            {
                CHECK(ul == ul2);
                ++ul2;
            }
            CHECK(ul2 == rowsToTest);
        }
        {
            unsigned long long ul2 = 0;

            std::vector<unsigned long long> vec(8);
            statement st = (sql.prepare <<
                "select ul from soci_test order by ul", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t i = 0; i != vec.size(); ++i)
                {
                    CHECK(ul2 == vec[i]);
                    ++ul2;
                }

                vec.resize(8);
            }
            CHECK(ul2 == rowsToTest);
        }
    }

    SECTION("double")
    {
        int const rowsToTest = 100;
        double d = 0.0;

        statement sti = (sql.prepare <<
            "insert into soci_test(d) values(:d)", use(d));
        for (int i = 0; i != rowsToTest; ++i)
        {
            sti.execute(true);
            d += 0.6;
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == rowsToTest);

        {
            double d2 = 0.0;
            int i = 0;

            statement st = (sql.prepare <<
                "select d from soci_test order by d", into(d));

            st.execute();
            while (st.fetch())
            {
                ASSERT_EQUAL(d, d2);
                d2 += 0.6;
                ++i;
            }
            CHECK(i == rowsToTest);
        }
        {
            double d2 = 0.0;
            int i = 0;

            std::vector<double> vec(8);
            statement st = (sql.prepare <<
                "select d from soci_test order by d", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t j = 0; j != vec.size(); ++j)
                {
                    ASSERT_EQUAL(d2, vec[j]);
                    d2 += 0.6;
                    ++i;
                }

                vec.resize(8);
            }
            CHECK(i == rowsToTest);
        }
    }

    SECTION("std::tm")
    {
        int const rowsToTest = 8;
        for (int i = 0; i != rowsToTest; ++i)
        {
            std::ostringstream ss;
            ss << 2000 + i << "-0" << 1 + i << '-' << 20 - i << ' '
                << 15 + i << ':' << 50 - i << ':' << 40 + i;

            sql << "insert into soci_test(id, tm) values(" << i
            << ", " << tc_.to_date_time(ss.str()) << ")";
        }

        int count;
        sql << "select count(*) from soci_test", into(count);
        CHECK(count == rowsToTest);

        {
            std::tm t = std::tm();
            int i = 0;

            statement st = (sql.prepare <<
                "select tm from soci_test order by id", into(t));

            st.execute();
            while (st.fetch())
            {
                CHECK(t.tm_year == 2000 - 1900 + i);
                CHECK(t.tm_mon == i);
                CHECK(t.tm_mday == 20 - i);
                CHECK(t.tm_hour == 15 + i);
                CHECK(t.tm_min == 50 - i);
                CHECK(t.tm_sec == 40 + i);

                ++i;
            }
            CHECK(i == rowsToTest);
        }
        {
            int i = 0;

            std::vector<std::tm> vec(3);
            statement st = (sql.prepare <<
                "select tm from soci_test order by id", into(vec));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t j = 0; j != vec.size(); ++j)
                {
                    CHECK(vec[j].tm_year == 2000 - 1900 + i);
                    CHECK(vec[j].tm_mon == i);
                    CHECK(vec[j].tm_mday == 20 - i);
                    CHECK(vec[j].tm_hour == 15 + i);
                    CHECK(vec[j].tm_min == 50 - i);
                    CHECK(vec[j].tm_sec == 40 + i);

                    ++i;
                }

                vec.resize(3);
            }
            CHECK(i == rowsToTest);
        }
    }
}

// test for indicators (repeated fetch and bulk)
TEST_CASE_METHOD(common_tests, "Indicators", "[core][indicator]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(id, val) values(1, 10)";
        sql << "insert into soci_test(id, val) values(2, 11)";
        sql << "insert into soci_test(id, val) values(3, NULL)";
        sql << "insert into soci_test(id, val) values(4, NULL)";
        sql << "insert into soci_test(id, val) values(5, 12)";

        {
            int val;
            indicator ind;

            statement st = (sql.prepare <<
                "select val from soci_test order by id", into(val, ind));

            st.execute();
            bool gotData = st.fetch();
            CHECK(gotData);
            CHECK(ind == i_ok);
            CHECK(val == 10);
            gotData = st.fetch();
            CHECK(gotData);
            CHECK(ind == i_ok);
            CHECK(val == 11);
            gotData = st.fetch();
            CHECK(gotData);
            CHECK(ind == i_null);
            gotData = st.fetch();
            CHECK(gotData);
            CHECK(ind == i_null);
            gotData = st.fetch();
            CHECK(gotData);
            CHECK(ind == i_ok);
            CHECK(val == 12);
            gotData = st.fetch();
            CHECK(gotData == false);
        }
        {
            std::vector<int> vals(3);
            std::vector<indicator> inds(3);

            statement st = (sql.prepare <<
                "select val from soci_test order by id", into(vals, inds));

            st.execute();
            bool gotData = st.fetch();
            CHECK(gotData);
            CHECK(vals.size() == 3);
            CHECK(inds.size() == 3);
            CHECK(inds[0] == i_ok);
            CHECK(vals[0] == 10);
            CHECK(inds[1] == i_ok);
            CHECK(vals[1] == 11);
            CHECK(inds[2] == i_null);
            gotData = st.fetch();
            CHECK(gotData);
            CHECK(vals.size() == 2);
            CHECK(inds[0] == i_null);
            CHECK(inds[1] == i_ok);
            CHECK(vals[1] == 12);
            gotData = st.fetch();
            CHECK(gotData == false);
        }

        // additional test for "no data" condition
        {
            std::vector<int> vals(3);
            std::vector<indicator> inds(3);

            statement st = (sql.prepare <<
                "select val from soci_test where 0 = 1", into(vals, inds));

            bool gotData = st.execute(true);
            CHECK(gotData == false);

            // for convenience, vectors should be truncated
            CHECK(vals.empty());
            CHECK(inds.empty());

            // for even more convenience, fetch should not fail
            // but just report end of rowset
            // (and vectors should be truncated)

            vals.resize(1);
            inds.resize(1);

            gotData = st.fetch();
            CHECK(gotData == false);
            CHECK(vals.empty());
            CHECK(inds.empty());
        }

        // additional test for "no data" without prepared statement
        {
            std::vector<int> vals(3);
            std::vector<indicator> inds(3);

            sql << "select val from soci_test where 0 = 1",
                into(vals, inds);

            // vectors should be truncated
            CHECK(vals.empty());
            CHECK(inds.empty());
        }
    }

}

// test for different sizes of data vector and indicators vector
// (library should force ind. vector to have same size as data vector)
TEST_CASE_METHOD(common_tests, "Indicators vector", "[core][indicator][vector]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(id, str, val) values(1, 'ten', 10)";
        sql << "insert into soci_test(id, str, val) values(2, 'elf', 11)";
        sql << "insert into soci_test(id, str, val) values(3, NULL, NULL)";
        sql << "insert into soci_test(id, str, val) values(4, NULL, NULL)";
        sql << "insert into soci_test(id, str, val) values(5, 'xii', 12)";

        {
            std::vector<int> vals(4);
            std::vector<indicator> inds;

            statement st = (sql.prepare <<
                "select val from soci_test order by id", into(vals, inds));

            st.execute();
            st.fetch();
            CHECK(vals.size() == 4);
            CHECK(inds.size() == 4);
            vals.resize(3);
            st.fetch();
            CHECK(vals.size() == 1);
            CHECK(inds.size() == 1);

            std::vector<std::string> strs(5);
            sql << "select str from soci_test order by id", into(strs, inds);
            REQUIRE(inds.size() == 5);
            CHECK(inds[0] == i_ok);
            CHECK(inds[1] == i_ok);
            CHECK(inds[2] == i_null);
            CHECK(inds[3] == i_null);
            CHECK(inds[4] == i_ok);

            strs.resize(1);
            sql << "select str from soci_test order by id", into(strs, inds);
            CHECK(inds.size() == 1);

            strs.resize(1);
            st = (sql.prepare << "select str from soci_test order by id", into(strs, inds));
            st.execute();
            st.fetch();
            CHECK(inds.size() == 1);
            while (st.fetch());

            std::vector<int> ids(1);
            sql << "select id from soci_test", into(ids);
            CHECK(ids.size() == 1);
        }
    }

}

TEST_CASE_METHOD(common_tests, "Get last insert ID", "[core][get_last_insert_id]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_get_last_insert_id(sql));

    // If the get_last_insert_id() supported by the backend.
    if (!tableCreator.get())
        return;

    long long id;
    REQUIRE(sql.get_last_insert_id("soci_test", id));
    // The initial value should be 1 and we call get_last_insert_id() before
    // the first insert, so the "pre-initial value" is 0.
    CHECK(id == 0);

    sql << "insert into soci_test(val) values(10)";

    REQUIRE(sql.get_last_insert_id("soci_test", id));
    CHECK(id == 1);

    sql << "insert into soci_test(val) values(11)";

    REQUIRE(sql.get_last_insert_id("soci_test", id));
    CHECK(id == 2);
}

// "use" tests, type conversions, etc.
TEST_CASE_METHOD(common_tests, "Use type conversion", "[core][use]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    SECTION("char")
    {
        char c('a');
        sql << "insert into soci_test(c) values(:c)", use(c);

        c = 'b';
        sql << "select c from soci_test", into(c);
        CHECK(c == 'a');

    }

    SECTION("std::string")
    {
        std::string s = "Hello SOCI!";
        sql << "insert into soci_test(str) values(:s)", use(s);

        std::string str;
        sql << "select str from soci_test", into(str);

        CHECK(str == "Hello SOCI!");
    }

    SECTION("int8_t")
    {
        int8_t i = 123;
        sql << "insert into soci_test(id) values(:id)", use(i);

        int8_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == 123);

        sql << "delete from soci_test";

        i = (std::numeric_limits<int8_t>::min)();
        sql << "insert into soci_test(id) values(:id)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int8_t>::min)());

        sql << "delete from soci_test";

        i = (std::numeric_limits<int8_t>::max)();
        sql << "insert into soci_test(id) values(:id)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int8_t>::max)());
    }

    SECTION("uint8_t")
    {
        uint8_t ui = 123;
        sql << "insert into soci_test(id) values(:id)", use(ui);

        uint8_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 123);

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint8_t>::min)();
        sql << "insert into soci_test(id) values(:id)", use(ui);

        ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint8_t>::min)());

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint8_t>::max)();
        sql << "insert into soci_test(id) values(:id)", use(ui);

        ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint8_t>::max)());
    }

    SECTION("short")
    {
        short s = 123;
        sql << "insert into soci_test(id) values(:id)", use(s);

        short s2 = 0;
        sql << "select id from soci_test", into(s2);

        CHECK(s2 == 123);
    }

    SECTION("int16_t")
    {
        int16_t i = 123;
        sql << "insert into soci_test(id) values(:id)", use(i);

        int16_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == 123);

        sql << "delete from soci_test";

        i = (std::numeric_limits<int16_t>::min)();
        sql << "insert into soci_test(id) values(:id)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int16_t>::min)());

        sql << "delete from soci_test";

        i = (std::numeric_limits<int16_t>::max)();
        sql << "insert into soci_test(id) values(:id)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int16_t>::max)());
    }

    SECTION("uint16_t")
    {
        uint16_t ui = 123;
        sql << "insert into soci_test(id) values(:id)", use(ui);

        uint16_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 123);

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint16_t>::min)();
        sql << "insert into soci_test(id) values(:id)", use(ui);

        ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint16_t>::min)());

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint16_t>::max)();
        sql << "insert into soci_test(id) values(:id)", use(ui);

        ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint16_t>::max)());
    }

    SECTION("int")
    {
        int i = -12345678;
        sql << "insert into soci_test(id) values(:i)", use(i);

        int i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == -12345678);
    }

    SECTION("int32_t")
    {
        int32_t i = -12345678;
        sql << "insert into soci_test(id) values(:i)", use(i);

        int32_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == -12345678);

        sql << "delete from soci_test";

        i = (std::numeric_limits<int32_t>::min)();
        sql << "insert into soci_test(id) values(:i)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int32_t>::min)());

        sql << "delete from soci_test";

        i = (std::numeric_limits<int32_t>::max)();
        sql << "insert into soci_test(id) values(:i)", use(i);

        i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int32_t>::max)());
    }

    SECTION("uint32_t")
    {
        uint32_t ui = 12345678;
        sql << "insert into soci_test(id) values(:i)", use(ui);

        uint32_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 12345678);

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint32_t>::min)();
        sql << "insert into soci_test(id) values(:i)", use(ui);

        ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint32_t>::min)());

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint32_t>::max)();
        sql << "insert into soci_test(ul) values(:i)", use(ui);

        ui2 = 0;
        sql << "select ul from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint32_t>::max)());
    }

    SECTION("unsigned long")
    {
        unsigned long ul = 4000000000ul;
        sql << "insert into soci_test(ul) values(:num)", use(ul);

        unsigned long ul2 = 0;
        sql << "select ul from soci_test", into(ul2);

        CHECK(ul2 == 4000000000ul);
    }

    SECTION("int64_t")
    {
        int64_t i = 4000000000ll;
        sql << "insert into soci_test(ll) values(:num)", use(i);

        int64_t i2 = 0;
        sql << "select ll from soci_test", into(i2);

        CHECK(i2 == 4000000000ll);

        sql << "delete from soci_test";

        i = (std::numeric_limits<int64_t>::min)();
        sql << "insert into soci_test(ll) values(:num)", use(i);

        i2 = 0;
        sql << "select ll from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int64_t>::min)());

        sql << "delete from soci_test";

        i = (std::numeric_limits<int64_t>::max)();
        sql << "insert into soci_test(ll) values(:num)", use(i);

        i2 = 0;
        sql << "select ll from soci_test", into(i2);

        CHECK(i2 == (std::numeric_limits<int64_t>::max)());
    }

    SECTION("uint64_t")
    {
        uint64_t ui = 4000000000ull;
        sql << "insert into soci_test(ul) values(:num)", use(ui);

        uint64_t ui2 = 0;
        sql << "select ul from soci_test", into(ui2);

        CHECK(ui2 == 4000000000ull);

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint64_t>::min)();
        sql << "insert into soci_test(ul) values(:num)", use(ui);

        ui2 = 0;
        sql << "select ul from soci_test", into(ui2);

        CHECK(ui2 == (std::numeric_limits<uint64_t>::min)());

        sql << "delete from soci_test";

        ui = (std::numeric_limits<uint64_t>::max)();
        sql << "insert into soci_test(ul) values(:num)", use(ui);

        ui2 = 0;
        sql << "select ul from soci_test", into(ui2);

        if (tc_.truncates_uint64_to_int64())
        {
            CHECK(ui2 == static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()));
        }
        else
        {
            CHECK(ui2 == (std::numeric_limits<uint64_t>::max)());
        }
    }

    SECTION("double")
    {
        double d = 3.14159265;
        sql << "insert into soci_test(d) values(:d)", use(d);

        double d2 = 0;
        sql << "select d from soci_test", into(d2);

        ASSERT_EQUAL(d2, d);
    }

    SECTION("std::tm")
    {
        std::tm t = std::tm();
        t.tm_year = 105;
        t.tm_mon = 10;
        t.tm_mday = 19;
        t.tm_hour = 21;
        t.tm_min = 39;
        t.tm_sec = 57;
        sql << "insert into soci_test(tm) values(:t)", use(t);

        std::tm t2 = std::tm();
        t2.tm_year = 0;
        t2.tm_mon = 0;
        t2.tm_mday = 0;
        t2.tm_hour = 0;
        t2.tm_min = 0;
        t2.tm_sec = 0;

        sql << "select tm from soci_test", into(t2);

        CHECK(t.tm_year == 105);
        CHECK(t.tm_mon  == 10);
        CHECK(t.tm_mday == 19);
        CHECK(t.tm_hour == 21);
        CHECK(t.tm_min  == 39);
        CHECK(t.tm_sec  == 57);
    }

    SECTION("repeated use")
    {
        int i;
        statement st = (sql.prepare
            << "insert into soci_test(id) values(:id)", use(i));

        i = 5;
        st.execute(true);
        i = 6;
        st.execute(true);
        i = 7;
        st.execute(true);

        std::vector<int> v(5);
        sql << "select id from soci_test order by id", into(v);

        CHECK(v.size() == 3);
        CHECK(v[0] == 5);
        CHECK(v[1] == 6);
        CHECK(v[2] == 7);
    }

    // tests for use of const objects

    SECTION("const char")
    {
        char const c('a');
        sql << "insert into soci_test(c) values(:c)", use(c);

        char c2 = 'b';
        sql << "select c from soci_test", into(c2);
        CHECK(c2 == 'a');

    }

    SECTION("const std::string")
    {
        std::string const s = "Hello const SOCI!";
        sql << "insert into soci_test(str) values(:s)", use(s);

        std::string str;
        sql << "select str from soci_test", into(str);

        CHECK(str == "Hello const SOCI!");
    }

    SECTION("const int8_t")
    {
        int8_t const i = 123;
        sql << "insert into soci_test(id) values(:id)", use(i);

        int8_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == 123);
    }

    SECTION("const uint8_t")
    {
        uint8_t const ui = 123;
        sql << "insert into soci_test(id) values(:id)", use(ui);

        uint8_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 123);
    }

    SECTION("const short")
    {
        short const s = 123;
        sql << "insert into soci_test(id) values(:id)", use(s);

        short s2 = 0;
        sql << "select id from soci_test", into(s2);

        CHECK(s2 == 123);
    }

    SECTION("const int16_t")
    {
        int16_t const i = 123;
        sql << "insert into soci_test(id) values(:id)", use(i);

        int16_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == 123);
    }

    SECTION("const uint16_t")
    {
        uint16_t const ui = 123;
        sql << "insert into soci_test(id) values(:id)", use(ui);

        uint16_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 123);
    }

    SECTION("const int")
    {
        int const i = -12345678;
        sql << "insert into soci_test(id) values(:i)", use(i);

        int i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == -12345678);
    }

    SECTION("const int32_t")
    {
        int32_t const i = -12345678;
        sql << "insert into soci_test(id) values(:i)", use(i);

        int32_t i2 = 0;
        sql << "select id from soci_test", into(i2);

        CHECK(i2 == -12345678);
    }

    SECTION("const uint32_t")
    {
        uint32_t const ui = 12345678;
        sql << "insert into soci_test(id) values(:i)", use(ui);

        uint32_t ui2 = 0;
        sql << "select id from soci_test", into(ui2);

        CHECK(ui2 == 12345678);
    }

    SECTION("const unsigned long")
    {
        unsigned long const ul = 4000000000ul;
        sql << "insert into soci_test(ul) values(:num)", use(ul);

        unsigned long ul2 = 0;
        sql << "select ul from soci_test", into(ul2);

        CHECK(ul2 == 4000000000ul);
    }

    SECTION("const int64_t")
    {
        int64_t const i = 4000000000ll;
        sql << "insert into soci_test(ul) values(:num)", use(i);

        int64_t i2 = 0;
        sql << "select ul from soci_test", into(i2);

        CHECK(i2 == 4000000000ll);
    }

    SECTION("const uint64_t")
    {
        uint64_t const ui = 4000000000ull;
        sql << "insert into soci_test(ul) values(:num)", use(ui);

        uint64_t ui2 = 0;
        sql << "select ul from soci_test", into(ui2);

        CHECK(ui2 == 4000000000ull);
    }

    SECTION("const double")
    {
        double const d = 3.14159265;
        sql << "insert into soci_test(d) values(:d)", use(d);

        double d2 = 0;
        sql << "select d from soci_test", into(d2);

        ASSERT_EQUAL(d2, d);
    }

    SECTION("const std::tm")
    {
        std::tm t = std::tm();
        t.tm_year = 105;
        t.tm_mon = 10;
        t.tm_mday = 19;
        t.tm_hour = 21;
        t.tm_min = 39;
        t.tm_sec = 57;
        std::tm const & ct = t;
        sql << "insert into soci_test(tm) values(:t)", use(ct);

        std::tm t2 = std::tm();
        t2.tm_year = 0;
        t2.tm_mon = 0;
        t2.tm_mday = 0;
        t2.tm_hour = 0;
        t2.tm_min = 0;
        t2.tm_sec = 0;

        sql << "select tm from soci_test", into(t2);

        CHECK(t.tm_year == 105);
        CHECK(t.tm_mon  == 10);
        CHECK(t.tm_mday == 19);
        CHECK(t.tm_hour == 21);
        CHECK(t.tm_min  == 39);
        CHECK(t.tm_sec  == 57);
    }
}

// test for multiple use (and into) elements
TEST_CASE_METHOD(common_tests, "Multiple use and into", "[core][use][into]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    {
        int i1 = 5;
        int i2 = 6;
        int i3 = 7;

        sql << "insert into soci_test(i1, i2, i3) values(:i1, :i2, :i3)",
            use(i1), use(i2), use(i3);

        i1 = 0;
        i2 = 0;
        i3 = 0;
        sql << "select i1, i2, i3 from soci_test",
            into(i1), into(i2), into(i3);

        CHECK(i1 == 5);
        CHECK(i2 == 6);
        CHECK(i3 == 7);

        // same for vectors
        sql << "delete from soci_test";

        i1 = 0;
        i2 = 0;
        i3 = 0;

        statement st = (sql.prepare
            << "insert into soci_test(i1, i2, i3) values(:i1, :i2, :i3)",
            use(i1), use(i2), use(i3));

        i1 = 1;
        i2 = 2;
        i3 = 3;
        st.execute(true);
        i1 = 4;
        i2 = 5;
        i3 = 6;
        st.execute(true);
        i1 = 7;
        i2 = 8;
        i3 = 9;
        st.execute(true);

        std::vector<int> v1(5);
        std::vector<int> v2(5);
        std::vector<int> v3(5);

        sql << "select i1, i2, i3 from soci_test order by i1",
            into(v1), into(v2), into(v3);

        CHECK(v1.size() == 3);
        CHECK(v2.size() == 3);
        CHECK(v3.size() == 3);
        CHECK(v1[0] == 1);
        CHECK(v1[1] == 4);
        CHECK(v1[2] == 7);
        CHECK(v2[0] == 2);
        CHECK(v2[1] == 5);
        CHECK(v2[2] == 8);
        CHECK(v3[0] == 3);
        CHECK(v3[1] == 6);
        CHECK(v3[2] == 9);
    }
}

// use vector elements
TEST_CASE_METHOD(common_tests, "Use vector", "[core][use][vector]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    SECTION("char")
    {
        std::vector<char> v;
        v.push_back('a');
        v.push_back('b');
        v.push_back('c');
        v.push_back('d');

        sql << "insert into soci_test(c) values(:c)", use(v);

        std::vector<char> v2(4);

        sql << "select c from soci_test order by c", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == 'a');
        CHECK(v2[1] == 'b');
        CHECK(v2[2] == 'c');
        CHECK(v2[3] == 'd');
    }

    SECTION("std::string")
    {
        std::vector<std::string> v;
        v.push_back("ala");
        v.push_back("ma");
        v.push_back("kota");

        sql << "insert into soci_test(str) values(:s)", use(v);

        std::vector<std::string> v2(4);

        sql << "select str from soci_test order by str", into(v2);
        CHECK(v2.size() == 3);
        CHECK(v2[0] == "ala");
        CHECK(v2[1] == "kota");
        CHECK(v2[2] == "ma");
    }

    SECTION("int8_t")
    {
        std::vector<int8_t> v;
        v.push_back((std::numeric_limits<int8_t>::min)());
        v.push_back(-5);
        v.push_back(123);
        v.push_back((std::numeric_limits<int8_t>::max)());

        sql << "insert into soci_test(sh) values(:sh)", use(v);

        std::vector<int8_t> v2(4);

        sql << "select sh from soci_test", into(v2);
        CHECK(v2.size() == 4);

        // This is a hack: "order by" doesn't work correctly with SQL Server
        // for this type because it's stored as unsigned in the database, so
        // sort the values here instead.
        std::sort(v2.begin(), v2.end());

        CHECK((int)v2[0] == (int)(std::numeric_limits<int8_t>::min)());
        CHECK((int)v2[1] == (int)-5);
        CHECK((int)v2[2] == (int)123);
        CHECK((int)v2[3] == (int)(std::numeric_limits<int8_t>::max)());
    }

    SECTION("uint8_t")
    {
        std::vector<uint8_t> v;
        v.push_back((std::numeric_limits<uint8_t>::min)());
        v.push_back(6);
        v.push_back(123);
        v.push_back((std::numeric_limits<uint8_t>::max)());

        sql << "insert into soci_test(sh) values(:sh)", use(v);

        std::vector<uint8_t> v2(4);

        sql << "select sh from soci_test order by sh", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == (std::numeric_limits<uint8_t>::min)());
        CHECK(v2[1] == 6);
        CHECK(v2[2] == 123);
        CHECK(v2[3] == (std::numeric_limits<uint8_t>::max)());
    }

    SECTION("short")
    {
        std::vector<short> v;
        v.push_back(-5);
        v.push_back(6);
        v.push_back(7);
        v.push_back(123);

        sql << "insert into soci_test(sh) values(:sh)", use(v);

        std::vector<short> v2(4);

        sql << "select sh from soci_test order by sh", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == -5);
        CHECK(v2[1] == 6);
        CHECK(v2[2] == 7);
        CHECK(v2[3] == 123);
    }

    SECTION("int16_t")
    {
        std::vector<int16_t> v;
        v.push_back((std::numeric_limits<int16_t>::min)());
        v.push_back(-5);
        v.push_back(123);
        v.push_back((std::numeric_limits<int16_t>::max)());

        sql << "insert into soci_test(sh) values(:sh)", use(v);

        std::vector<int16_t> v2(4);

        sql << "select sh from soci_test order by sh", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == (std::numeric_limits<int16_t>::min)());
        CHECK(v2[1] == -5);
        CHECK(v2[2] == 123);
        CHECK(v2[3] == (std::numeric_limits<int16_t>::max)());
    }

    SECTION("uint16_t")
    {
        std::vector<uint16_t> v;
        v.push_back((std::numeric_limits<uint16_t>::min)());
        v.push_back(6);
        v.push_back(123);
        v.push_back((std::numeric_limits<uint16_t>::max)());

        sql << "insert into soci_test(val) values(:val)", use(v);

        std::vector<uint16_t> v2(4);

        sql << "select val from soci_test order by val", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == (std::numeric_limits<uint16_t>::min)());
        CHECK(v2[1] == 6);
        CHECK(v2[2] == 123);
        CHECK(v2[3] == (std::numeric_limits<uint16_t>::max)());
    }

    SECTION("int")
    {
        std::vector<int> v;
        v.push_back(-2000000000);
        v.push_back(0);
        v.push_back(1);
        v.push_back(2000000000);

        sql << "insert into soci_test(id) values(:i)", use(v);

        std::vector<int> v2(4);

        sql << "select id from soci_test order by id", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == -2000000000);
        CHECK(v2[1] == 0);
        CHECK(v2[2] == 1);
        CHECK(v2[3] == 2000000000);
    }

    SECTION("int32_t")
    {
        std::vector<int32_t> v;
        v.push_back((std::numeric_limits<int32_t>::min)());
        v.push_back(-2000000000);
        v.push_back(0);
        v.push_back(1);
        v.push_back(2000000000);
        v.push_back((std::numeric_limits<int32_t>::max)());

        sql << "insert into soci_test(id) values(:i)", use(v);

        std::vector<int32_t> v2(6);

        sql << "select id from soci_test order by id", into(v2);
        CHECK(v2.size() == 6);
        CHECK(v2[0] == (std::numeric_limits<int32_t>::min)());
        CHECK(v2[1] == -2000000000);
        CHECK(v2[2] == 0);
        CHECK(v2[3] == 1);
        CHECK(v2[4] == 2000000000);
        CHECK(v2[5] == (std::numeric_limits<int32_t>::max)());
    }

    SECTION("unsigned int")
    {
        std::vector<unsigned int> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(123);
        v.push_back(1000);

        sql << "insert into soci_test(ul) values(:ul)", use(v);

        std::vector<unsigned int> v2(4);

        sql << "select ul from soci_test order by ul", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == 0);
        CHECK(v2[1] == 1);
        CHECK(v2[2] == 123);
        CHECK(v2[3] == 1000);
    }

    SECTION("uint32_t")
    {
        std::vector<uint32_t> v;
        v.push_back((std::numeric_limits<uint32_t>::min)());
        v.push_back(0);
        v.push_back(1);
        v.push_back(123);
        v.push_back(1000);
        v.push_back((std::numeric_limits<uint32_t>::max)());

        sql << "insert into soci_test(ul) values(:ul)", use(v);

        std::vector<uint32_t> v2(6);

        sql << "select ul from soci_test order by ul", into(v2);
        CHECK(v2.size() == 6);
        CHECK(v2[0] == (std::numeric_limits<uint32_t>::min)());
        CHECK(v2[1] == 0);
        CHECK(v2[2] == 1);
        CHECK(v2[3] == 123);
        CHECK(v2[4] == 1000);
        CHECK(v2[5] == (std::numeric_limits<uint32_t>::max)());
    }

    SECTION("unsigned long long")
    {
        std::vector<unsigned long long> v;
        v.push_back(0);
        v.push_back(1);
        v.push_back(123);
        v.push_back(1000);

        sql << "insert into soci_test(ul) values(:ul)", use(v);

        std::vector<unsigned int> v2(4);

        sql << "select ul from soci_test order by ul", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == 0);
        CHECK(v2[1] == 1);
        CHECK(v2[2] == 123);
        CHECK(v2[3] == 1000);
    }

    SECTION("int64_t")
    {
        std::vector<int64_t> v;
        v.push_back((std::numeric_limits<int64_t>::min)());
        v.push_back(0);
        v.push_back(1);
        v.push_back(123);
        v.push_back(1000);
        v.push_back((std::numeric_limits<int64_t>::max)());

        sql << "insert into soci_test(ll) values(:ll)", use(v);

        std::vector<int64_t> v2(6);

        sql << "select ll from soci_test order by ll", into(v2);
        CHECK(v2.size() == 6);
        CHECK(v2[0] == (std::numeric_limits<int64_t>::min)());
        CHECK(v2[1] == 0);
        CHECK(v2[2] == 1);
        CHECK(v2[3] == 123);
        CHECK(v2[4] == 1000);
        CHECK(v2[5] == (std::numeric_limits<int64_t>::max)());
    }

    SECTION("uint64_t")
    {
        std::vector<uint64_t> v;
        v.push_back((std::numeric_limits<uint64_t>::min)());
        v.push_back(0);
        v.push_back(1);
        v.push_back(123);
        v.push_back(1000);
        v.push_back((std::numeric_limits<uint64_t>::max)());

        sql << "insert into soci_test(ul) values(:ul)", use(v);

        std::vector<uint64_t> v2(6);

        sql << "select ul from soci_test order by ul", into(v2);
        CHECK(v2.size() == 6);
        if (tc_.has_uint64_storage_bug())
        {
            CHECK(v2[0] == (std::numeric_limits<uint64_t>::max)());
            CHECK(v2[1] == (std::numeric_limits<uint64_t>::min)());
            CHECK(v2[2] == 0);
            CHECK(v2[3] == 1);
            CHECK(v2[4] == 123);
            CHECK(v2[5] == 1000);
        }
        else
        {
            CHECK(v2[0] == (std::numeric_limits<uint64_t>::min)());
            CHECK(v2[1] == 0);
            CHECK(v2[2] == 1);
            CHECK(v2[3] == 123);
            CHECK(v2[4] == 1000);
            if (tc_.truncates_uint64_to_int64())
            {
                CHECK(v2[5] == static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()));
            }
            else
            {
                CHECK(v2[5] == (std::numeric_limits<uint64_t>::max)());
            }
        }
    }

    SECTION("double")
    {
        std::vector<double> v;
        v.push_back(0);
        v.push_back(-0.0001);
        v.push_back(0.0001);
        v.push_back(3.1415926);

        sql << "insert into soci_test(d) values(:d)", use(v);

        std::vector<double> v2(4);

        sql << "select d from soci_test order by d", into(v2);
        CHECK(v2.size() == 4);
        ASSERT_EQUAL(v2[0],-0.0001);
        ASSERT_EQUAL(v2[1], 0);
        ASSERT_EQUAL(v2[2], 0.0001);
        ASSERT_EQUAL(v2[3], 3.1415926);
    }

    SECTION("std::tm")
    {
        std::vector<std::tm> v;
        std::tm t = std::tm();
        t.tm_year = 105;
        t.tm_mon  = 10;
        t.tm_mday = 26;
        t.tm_hour = 22;
        t.tm_min  = 45;
        t.tm_sec  = 17;

        v.push_back(t);

        t.tm_sec = 37;
        v.push_back(t);

        t.tm_mday = 25;
        v.push_back(t);

        sql << "insert into soci_test(tm) values(:t)", use(v);

        std::vector<std::tm> v2(4);

        sql << "select tm from soci_test order by tm", into(v2);
        CHECK(v2.size() == 3);
        CHECK(v2[0].tm_year == 105);
        CHECK(v2[0].tm_mon  == 10);
        CHECK(v2[0].tm_mday == 25);
        CHECK(v2[0].tm_hour == 22);
        CHECK(v2[0].tm_min  == 45);
        CHECK(v2[0].tm_sec  == 37);
        CHECK(v2[1].tm_year == 105);
        CHECK(v2[1].tm_mon  == 10);
        CHECK(v2[1].tm_mday == 26);
        CHECK(v2[1].tm_hour == 22);
        CHECK(v2[1].tm_min  == 45);
        CHECK(v2[1].tm_sec  == 17);
        CHECK(v2[2].tm_year == 105);
        CHECK(v2[2].tm_mon  == 10);
        CHECK(v2[2].tm_mday == 26);
        CHECK(v2[2].tm_hour == 22);
        CHECK(v2[2].tm_min  == 45);
        CHECK(v2[2].tm_sec  == 37);
    }

    SECTION("const int")
    {
        std::vector<int> v;
        v.push_back(-2000000000);
        v.push_back(0);
        v.push_back(1);
        v.push_back(2000000000);

        std::vector<int> const & cv = v;

        sql << "insert into soci_test(id) values(:i)", use(cv);

        std::vector<int> v2(4);

        sql << "select id from soci_test order by id", into(v2);
        CHECK(v2.size() == 4);
        CHECK(v2[0] == -2000000000);
        CHECK(v2[1] == 0);
        CHECK(v2[2] == 1);
        CHECK(v2[3] == 2000000000);
    }
}

// test for named binding
TEST_CASE_METHOD(common_tests, "Named parameters", "[core][use][named-params]")
{
    soci::session sql(backEndFactory_, connectString_);
    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        int i1 = 7;
        int i2 = 8;

        // verify the exception is thrown if both by position
        // and by name use elements are specified
        try
        {
            sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
                use(i1, "i1"), use(i2);

            FAIL("expected exception not thrown");
        }
        catch (soci_error const& e)
        {
            CHECK(e.get_error_message() ==
                "Binding for use elements must be either by position "
                "or by name.");
        }

        // normal test
        sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
            use(i1, "i1"), use(i2, "i2");

        i1 = 0;
        i2 = 0;
        sql << "select i1, i2 from soci_test", into(i1), into(i2);
        CHECK(i1 == 7);
        CHECK(i2 == 8);

        i2 = 0;
        sql << "select i2 from soci_test where i1 = :i1", into(i2), use(i1);
        CHECK(i2 == 8);

        sql << "delete from soci_test";

        // test vectors

        std::vector<int> v1;
        v1.push_back(1);
        v1.push_back(2);
        v1.push_back(3);

        std::vector<int> v2;
        v2.push_back(4);
        v2.push_back(5);
        v2.push_back(6);

        sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
            use(v1, "i1"), use(v2, "i2");

        sql << "select i2, i1 from soci_test order by i1 desc",
            into(v1), into(v2);
        CHECK(v1.size() == 3);
        CHECK(v2.size() == 3);
        CHECK(v1[0] == 6);
        CHECK(v1[1] == 5);
        CHECK(v1[2] == 4);
        CHECK(v2[0] == 3);
        CHECK(v2[1] == 2);
        CHECK(v2[2] == 1);
    }
}

TEST_CASE_METHOD(common_tests, "Named parameters with similar names", "[core][use][named-params]")
{
    // Verify parsing of parameters with similar names,
    // where one name is part of the other, etc.
    // https://github.com/SOCI/soci/issues/26

    soci::session sql(backEndFactory_, connectString_);
    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));
        std::string passwd("abc");
        std::string passwd_clear("clear");

        SECTION("unnamed")
        {
            sql << "INSERT INTO soci_test(str,name) VALUES(:passwd_clear, :passwd)",
                    soci::use(passwd), soci::use(passwd_clear);
        }

        SECTION("same order")
        {
            sql << "INSERT INTO soci_test(str,name) VALUES(:passwd_clear, :passwd)",
                    soci::use(passwd_clear, "passwd_clear"), soci::use(passwd, "passwd");
        }

        SECTION("reversed order")
        {
            sql << "INSERT INTO soci_test(str,name) VALUES(:passwd_clear, :passwd)",
                    soci::use(passwd, "passwd"), soci::use(passwd_clear, "passwd_clear");
        }

        // TODO: Allow binding the same varibale multiple times
        // SECTION("one for multiple placeholders")
        // {
        //     sql << "INSERT INTO soci_test(str,name) VALUES(:passwd, :passwd)",
        //             soci::use(passwd, "passwd");
        // }
    }
}

// transaction test
TEST_CASE_METHOD(common_tests, "Transactions", "[core][transaction]")
{
    soci::session sql(backEndFactory_, connectString_);

    if (!tc_.has_transactions_support(sql))
    {
        WARN("Transactions not supported by the database, skipping the test.");
        return;
    }

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    int count;
    sql << "select count(*) from soci_test", into(count);
    CHECK(count == 0);

    {
        transaction tr(sql);

        sql << "insert into soci_test (id, name) values(1, 'John')";
        sql << "insert into soci_test (id, name) values(2, 'Anna')";
        sql << "insert into soci_test (id, name) values(3, 'Mike')";

        tr.commit();
    }
    {
        transaction tr(sql);

        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 3);

        sql << "insert into soci_test (id, name) values(4, 'Stan')";

        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 4);

        tr.rollback();

        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 3);
    }
    {
        transaction tr(sql);

        sql << "delete from soci_test";

        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 0);

        tr.rollback();

        sql << "select count(*) from soci_test", into(count);
        CHECK(count == 3);
    }
    {
        // additional test for detection of double commit
        transaction tr(sql);
        tr.commit();
        try
        {
            tr.commit();
            FAIL("expected exception not thrown");
        }
        catch (soci_error const &e)
        {
            CHECK(e.get_error_message() ==
                "The transaction object cannot be handled twice.");
        }
    }
}

std::tm  generate_tm()
{
    std::tm t = std::tm();
    t.tm_year = 105;
    t.tm_mon = 10;
    t.tm_mday = 15;
    t.tm_hour = 22;
    t.tm_min = 14;
    t.tm_sec = 17;
    return t;
}

// test of use elements with indicators
TEST_CASE_METHOD(common_tests, "Use with indicators", "[core][use][indicator]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    indicator ind1 = i_ok;
    indicator ind2 = i_ok;
    indicator ind3 = i_ok;

    int id = 1;
    int val = 10;
    std::tm tm_gen = generate_tm();
    char const* insert = "insert into soci_test(id, val, tm) values(:id, :val, :tm)";
    sql << insert, use(id, ind1), use(val, ind2), use(tm_gen, ind3);

    id = 2;
    val = 11;
    ind2 = i_null;
    std::tm tm = std::tm();
    ind3 = i_null;

    sql << "insert into soci_test(id, val, tm) values(:id, :val, :tm)",
        use(id, ind1), use(val, ind2), use(tm, ind3);

    sql << "select val from soci_test where id = 1", into(val, ind2);
    CHECK(ind2 == i_ok);
    CHECK(val == 10);
    sql << "select val, tm from soci_test where id = 2", into(val, ind2), into(tm, ind3);
    CHECK(ind2 == i_null);
    CHECK(ind3 == i_null);

    std::vector<int> ids;
    ids.push_back(3);
    ids.push_back(4);
    ids.push_back(5);
    std::vector<int> vals;
    vals.push_back(12);
    vals.push_back(13);
    vals.push_back(14);
    std::vector<indicator> inds;
    inds.push_back(i_ok);
    inds.push_back(i_null);
    inds.push_back(i_ok);

    sql << "insert into soci_test(id, val) values(:id, :val)",
        use(ids), use(vals, inds);

    ids.resize(5);
    vals.resize(5);
    sql << "select id, val from soci_test order by id desc",
        into(ids), into(vals, inds);

    CHECK(ids.size() == 5);
    CHECK(ids[0] == 5);
    CHECK(ids[1] == 4);
    CHECK(ids[2] == 3);
    CHECK(ids[3] == 2);
    CHECK(ids[4] == 1);
    CHECK(inds.size() == 5);
    CHECK(inds[0] == i_ok);
    CHECK(inds[1] == i_null);
    CHECK(inds[2] == i_ok);
    CHECK(inds[3] == i_null);
    CHECK(inds[4] == i_ok);
    CHECK(vals.size() == 5);
    CHECK(vals[0] == 14);
    CHECK(vals[2] == 12);
    CHECK(vals[4] == 10);
}

TEST_CASE_METHOD(common_tests, "Numeric round trip", "[core][float]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    double d1 = 0.003958,
           d2;

    sql << "insert into soci_test(num76) values (:d1)", use(d1);
    sql << "select num76 from soci_test", into(d2);

    // The numeric value should make the round trip unchanged, we really want
    // to use exact comparisons here.
    ASSERT_EQUAL_EXACT(d1, d2);

    // test negative doubles too
    sql << "delete from soci_test";
    d1 = -d1;

    sql << "insert into soci_test(num76) values (:d1)", use(d1);
    sql << "select num76 from soci_test", into(d2);

    ASSERT_EQUAL_EXACT(d1, d2);
}

// test for bulk fetch with single use
TEST_CASE_METHOD(common_tests, "Bulk fetch with single use", "[core][bulk]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    sql << "insert into soci_test(name, id) values('john', 1)";
    sql << "insert into soci_test(name, id) values('george', 2)";
    sql << "insert into soci_test(name, id) values('anthony', 1)";
    sql << "insert into soci_test(name, id) values('marc', 3)";
    sql << "insert into soci_test(name, id) values('julian', 1)";

    int code = 1;
    std::vector<std::string> names(10);
    sql << "select name from soci_test where id = :id order by name",
         into(names), use(code);

    CHECK(names.size() == 3);
    CHECK(names[0] == "anthony");
    CHECK(names[1] == "john");
    CHECK(names[2] == "julian");
}

// test for basic logging support
TEST_CASE_METHOD(common_tests, "Basic logging support", "[core][logging]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.set_query_context_logging_mode(log_context::always);

    std::ostringstream log;
    sql.set_log_stream(&log);

    try
    {
        sql << "drop table soci_test1";
    }
    catch (...) {}

    CHECK(sql.get_last_query() == "drop table soci_test1");
    CHECK(sql.get_last_query_context() == "");

    sql.set_log_stream(NULL);

    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        int id = 1;
        std::string name = "b";
        sql << "insert into soci_test (name,id) values (:name,:id)", use(name, "name"), use(id, "id");

        CHECK(sql.get_last_query() == "insert into soci_test (name,id) values (:name,:id)");
        CHECK(sql.get_last_query_context() == R"(:name="b", :id=1)");

        statement stmt = (sql.prepare << "insert into soci_test(name, id) values (:name, :id)");
        {
            id = 5;
            name = "alice";
            stmt.exchange(use(name, "name"));
            stmt.exchange(use(id, "id"));
            stmt.define_and_bind();
            stmt.execute(true);
            stmt.bind_clean_up();
            CHECK(sql.get_last_query() == "insert into soci_test(name, id) values (:name, :id)");
            CHECK(sql.get_last_query_context() == R"(:name="alice", :id=5)");
        }
        {
            id = 42;
            name = "bob";
            stmt.exchange(use(name, "name"));
            stmt.exchange(use(id, "id"));
            stmt.define_and_bind();
            stmt.execute(true);
            stmt.bind_clean_up();
            CHECK(sql.get_last_query() == "insert into soci_test(name, id) values (:name, :id)");
            CHECK(sql.get_last_query_context() == R"(:name="bob", :id=42)");
        }

        // Suppress bogus MSVC warnings about unrecognized escape sequences in
        // the raw strings below.
        SOCI_MSVC_WARNING_SUPPRESS(4129)

        try {
            sql << "insert into soci_test (dummy, id) values (:dummy, :id)", use(name, "dummy"), use(id, "id");
        } catch (const soci_error &e) {
            REQUIRE_THAT(e.what(),
                Catch::Matches(
                    R"re((.|\n)+ while (preparing|executing) "insert into soci_test \(dummy, id\) values \(:dummy, :id\)" with :dummy="bob", :id=42\.)re"
                )
            );
        }
        CHECK(sql.get_last_query() == "insert into soci_test (dummy, id) values (:dummy, :id)");
        CHECK(sql.get_last_query_context() == R"(:dummy="bob", :id=42)");


        // on_error mode
        sql.set_query_context_logging_mode(log_context::on_error);

        sql << "insert into soci_test (name, id) values (:name, :id)", use(name, "name"), use(id, "id");
        CHECK(sql.get_last_query_context() == "");
        try {
            sql << "insert into soci_test (dummy, id) values (:dummy, :id)", use(name, "dummy"), use(id, "id");
        } catch (const soci_error &e) {
            REQUIRE_THAT(e.what(),
                Catch::Matches(
                    R"re((.|\n)+ while (preparing|executing) "insert into soci_test \(dummy, id\) values \(:dummy, :id\)" with :dummy="bob", :id=42\.)re"
                )
            );
        }
        CHECK(sql.get_last_query() == "insert into soci_test (dummy, id) values (:dummy, :id)");
        CHECK(sql.get_last_query_context() == R"(:dummy="bob", :id=42)");

        // never mode
        sql.set_query_context_logging_mode(log_context::never);

        sql << "insert into soci_test (name, id) values (:name, :id)", use(name, "name"), use(id, "id");
        CHECK(sql.get_last_query_context() == "");
        try {
            sql << "insert into soci_test (dummy, id) values (:dummy, :id)", use(name, "dummy"), use(id, "id");
        } catch (const soci_error &e) {
            REQUIRE_THAT(e.what(),
                Catch::Matches(
                    R"re((.|\n)+ while (preparing|executing) "insert into soci_test \(dummy, id\) values \(:dummy, :id\)"\.)re"
                )
            );
        }

        SOCI_MSVC_WARNING_RESTORE(4129)

        CHECK(sql.get_last_query() == "insert into soci_test (dummy, id) values (:dummy, :id)");
        CHECK(sql.get_last_query_context() == "");
    }

    sql.set_log_stream(&log);

    try
    {
        sql << "drop table soci_test3";
    }
    catch (...) {}

    CHECK(sql.get_last_query() == "drop table soci_test3");
    CHECK(log.str() ==
        "drop table soci_test1\n"
        "drop table soci_test3\n");

}

TEST_CASE("soci_error is nothrow", "[core][exception][nothrow]")
{
    CHECK(std::is_nothrow_copy_assignable<soci_error>::value == true);
    CHECK(std::is_nothrow_copy_constructible<soci_error>::value == true);
    CHECK(std::is_nothrow_destructible<soci_error>::value == true);
}

#ifdef SOCI_HAVE_CXX17

// test for handling NULL values with std::optional
// (both into and use)
TEST_CASE_METHOD(common_tests, "NULL with std optional", "[core][null]")
{

    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator0(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(val) values(7)";

        {
            // verify non-null value is fetched correctly
            std::optional<int> opt;
            sql << "select val from soci_test", into(opt);
            CHECK(opt.has_value());
            CHECK(opt.value() == 7);

            // indicators can be used with optional
            // (although that's just a consequence of implementation,
            // not an intended feature - but let's test it anyway)
            indicator ind;
            opt.reset();
            sql << "select val from soci_test", into(opt, ind);
            CHECK(opt.has_value());
            CHECK(opt.value() == 7);
            CHECK(ind == i_ok);

            // verify null value is fetched correctly
            sql << "select i1 from soci_test", into(opt);
            CHECK(opt.has_value() == false);

            // and with indicator
            opt = 5;
            sql << "select i1 from soci_test", into(opt, ind);
            CHECK(opt.has_value() == false);
            CHECK(ind == i_null);

            // verify non-null is inserted correctly
            opt = 3;
            sql << "update soci_test set val = :v", use(opt);
            int j = 0;
            sql << "select val from soci_test", into(j);
            CHECK(j == 3);

            // verify null is inserted correctly
            opt.reset();
            sql << "update soci_test set val = :v", use(opt);
            ind = i_ok;
            sql << "select val from soci_test", into(j, ind);
            CHECK(ind == i_null);
        }

        // vector tests (select)

        {
            sql << "delete from soci_test";

            // simple readout of non-null data

            sql << "insert into soci_test(id, val, str) values(1, 5, \'abc\')";
            sql << "insert into soci_test(id, val, str) values(2, 6, \'def\')";
            sql << "insert into soci_test(id, val, str) values(3, 7, \'ghi\')";
            sql << "insert into soci_test(id, val, str) values(4, 8, null)";
            sql << "insert into soci_test(id, val, str) values(5, 9, \'mno\')";

            std::vector<std::optional<int> > v(10);
            sql << "select val from soci_test order by val", into(v);

            CHECK(v.size() == 5);
            CHECK(v[0].has_value());
            CHECK(v[0].value() == 5);
            CHECK(v[1].has_value());
            CHECK(v[1].value() == 6);
            CHECK(v[2].has_value());
            CHECK(v[2].value() == 7);
            CHECK(v[3].has_value());
            CHECK(v[3].value() == 8);
            CHECK(v[4].has_value());
            CHECK(v[4].value() == 9);

            // readout of nulls

            sql << "update soci_test set val = null where id = 2 or id = 4";

            std::vector<int> ids(5);
            sql << "select id, val from soci_test order by id", into(ids), into(v);

            CHECK(v.size() == 5);
            CHECK(ids.size() == 5);
            CHECK(v[0].has_value());
            CHECK(v[0].value() == 5);
            CHECK(v[1].has_value() == false);
            CHECK(v[2].has_value());
            CHECK(v[2].value() == 7);
            CHECK(v[3].has_value() == false);
            CHECK(v[4].has_value());
            CHECK(v[4].value() == 9);

            // readout with statement preparation

            int id = 1;

            ids.resize(3);
            v.resize(3);
            statement st = (sql.prepare <<
                "select id, val from soci_test order by id", into(ids), into(v));
            st.execute();
            while (st.fetch())
            {
                for (std::size_t i = 0; i != v.size(); ++i)
                {
                    CHECK(id == ids[i]);

                    if (id == 2 || id == 4)
                    {
                        CHECK(v[i].has_value() == false);
                    }
                    else
                    {
                        CHECK(v[i].has_value());
                        CHECK(v[i].value() == id + 4);
                    }

                    ++id;
                }

                ids.resize(3);
                v.resize(3);
            }
            CHECK(id == 6);
        }

        // and why not stress iterators and the dynamic binding, too!

        {
            rowset<row> rs = (sql.prepare << "select id, val, str from soci_test order by id");

            rowset<row>::const_iterator it = rs.begin();
            CHECK(it != rs.end());

            row const& r1 = (*it);

            CHECK(r1.size() == 3);

            // Note: for the reason of differences between number(x,y) type and
            // binary representation of integers, the following commented assertions
            // do not work for Oracle.
            // The problem is that for this single table the data type used in Oracle
            // table creator for the id column is number(10,0),
            // which allows to insert all int values.
            // On the other hand, the column description scheme used in the Oracle
            // backend figures out that the natural type for such a column
            // is eUnsignedInt - this makes the following assertions fail.
            // Other database backends (like PostgreSQL) use other types like int
            // and this not only allows to insert all int values (obviously),
            // but is also recognized as int (obviously).
            // There is a similar problem with stream-like extraction,
            // where internally get<T> is called and the type mismatch is detected
            // for the id column - that's why the code below skips this column
            // and tests the remaining column only.

            //CHECK(r1.get_properties(0).get_data_type() == dt_integer);
            //CHECK(r1.get_properties(0).get_db_type() == db_int32);
            CHECK(r1.get_properties(1).get_data_type() == dt_integer);
            CHECK(r1.get_properties(1).get_db_type() == db_int32);
            CHECK(r1.get_properties(2).get_data_type() == dt_string);
            CHECK(r1.get_properties(2).get_db_type() == db_string);
            //CHECK(r1.get<int>(0) == 1);
            CHECK(r1.get<int>(1) == 5);
            CHECK(r1.get<std::string>(2) == "abc");
            CHECK(r1.get<std::optional<int> >(1).has_value());
            CHECK(r1.get<std::optional<int> >(1).value() == 5);
            CHECK(r1.get<std::optional<std::string> >(2).has_value());
            CHECK(r1.get<std::optional<std::string> >(2).value() == "abc");

            ++it;

            row const& r2 = (*it);

            CHECK(r2.size() == 3);

            // CHECK(r2.get_properties(0).get_data_type() == dt_integer);
            // CHECK(r2.get_properties(0).get_db_type() == db_int32);
            CHECK(r2.get_properties(1).get_data_type() == dt_integer);
            CHECK(r2.get_properties(1).get_db_type() == db_int32);
            CHECK(r2.get_properties(2).get_data_type() == dt_string);
            CHECK(r2.get_properties(2).get_db_type() == db_string);
            //CHECK(r2.get<int>(0) == 2);
            try
            {
                // expect exception here, this is NULL value
                (void)r1.get<int>(1);
                FAIL("expected exception not thrown");
            }
            catch (soci_error const &) {}

            // but we can read it as optional
            CHECK(r2.get<std::optional<int> >(1).has_value() == false);

            // stream-like data extraction

            ++it;
            row const &r3 = (*it);

            std::optional<int> io;
            std::optional<std::string> so;

            r3.skip(); // move to val and str columns
            r3 >> io >> so;

            CHECK(io.has_value());
            CHECK(io.value() == 7);
            CHECK(so.has_value());
            CHECK(so.value() == "ghi");

            ++it;
            row const &r4 = (*it);

            r3.skip(); // move to val and str columns
            r4 >> io >> so;

            CHECK(io.has_value() == false);
            CHECK(so.has_value() == false);
        }

        // inserts of non-null const data
        {
            sql << "delete from soci_test";

            const int id = 10;
            const std::optional<int> val = 11;

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(id, "id"),
                use(val, "val");

            int sum;
            sql << "select sum(val) from soci_test", into(sum);
            CHECK(sum == 11);
        }

        // bulk inserts of non-null data

        {
            sql << "delete from soci_test";

            std::vector<int> ids;
            std::vector<std::optional<int> > v;

            ids.push_back(10); v.push_back(20);
            ids.push_back(11); v.push_back(21);
            ids.push_back(12); v.push_back(22);
            ids.push_back(13); v.push_back(23);

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(ids, "id"), use(v, "val");

            int sum;
            sql << "select sum(val) from soci_test", into(sum);
            CHECK(sum == 86);

            // bulk inserts of some-null data

            sql << "delete from soci_test";

            v[2].reset();
            v[3].reset();

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(ids, "id"), use(v, "val");

            sql << "select sum(val) from soci_test", into(sum);
            CHECK(sum == 41);
        }


        // bulk inserts of non-null data with const vector

        {
            sql << "delete from soci_test";

            std::vector<int> ids;
            std::vector<std::optional<int> > v;

            ids.push_back(10); v.push_back(20);
            ids.push_back(11); v.push_back(21);
            ids.push_back(12); v.push_back(22);
            ids.push_back(13); v.push_back(23);

            const std::vector<int>& cref_ids = ids;
            const std::vector<std::optional<int> >& cref_v = v;

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(cref_ids, "id"),
                use(cref_v, "val");

            int sum;
            sql << "select sum(val) from soci_test", into(sum);
            CHECK(sum == 86);

            // bulk inserts of some-null data

            sql << "delete from soci_test";

            v[2].reset();
            v[3].reset();

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(cref_ids, "id"),
                use(cref_v, "val");

            sql << "select sum(val) from soci_test", into(sum);
            CHECK(sum == 41);
        }

        // composability with user conversions

        {
            sql << "delete from soci_test";

            std::optional<MyInt> omi1;
            std::optional<MyInt> omi2;

            omi1 = MyInt(125);
            omi2.reset();

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(omi1), use(omi2);

            sql << "select id, val from soci_test", into(omi2), into(omi1);

            CHECK(omi1.has_value() == false);
            CHECK(omi2.has_value());
            CHECK(omi2.value().get() == 125);
        }

        // use with const optional and user conversions

        {
            sql << "delete from soci_test";

            std::optional<MyInt> omi1;
            std::optional<MyInt> omi2;

            omi1 = MyInt(125);
            omi2.reset();

            std::optional<MyInt> const & comi1 = omi1;
            std::optional<MyInt> const & comi2 = omi2;

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(comi1), use(comi2);

            sql << "select id, val from soci_test", into(omi2), into(omi1);

            CHECK(omi1.has_value() == false);
            CHECK(omi2.has_value());
            CHECK(omi2.value().get() == 125);
        }

        // use with rowset and table containing null values

        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            sql << "insert into soci_test(id, val) values(1, 10)";
            sql << "insert into soci_test(id, val) values(2, 11)";
            sql << "insert into soci_test(id, val) values(3, NULL)";
            sql << "insert into soci_test(id, val) values(4, 13)";

            rowset<std::optional<int> > rs = (sql.prepare <<
                "select val from soci_test order by id asc");

            // 1st row
            rowset<std::optional<int> >::const_iterator pos = rs.begin();
            CHECK((*pos).has_value());
            CHECK(10 == (*pos).value());

            // 2nd row
            ++pos;
            CHECK((*pos).has_value());
            CHECK(11 == (*pos).value());

            // 3rd row
            ++pos;
            CHECK((*pos).has_value() == false);

            // 4th row
            ++pos;
            CHECK((*pos).has_value());
            CHECK(13 == (*pos).value());
        }

        // inserting using an i_null indicator with a std::optional should
        // insert null, even if the optional is valid, just as with standard
        // types
        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            {
                indicator ind = i_null;
                std::optional<int> v1(10);
                sql << "insert into soci_test(id, val) values(1, :val)",
                       use(v1, ind);
            }

            // verify the value is fetched correctly as null
            {
                indicator ind;
                std::optional<int> opt;

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 1", into(opt, ind);
                CHECK(ind == i_null);
                CHECK(!opt.has_value());
            }
        }

        // prepared statement inserting non-null and null values alternatively
        // (without passing an explicit indicator)
        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            {
                int id;
                std::optional<int> val;
                statement st = (sql.prepare
                    << "insert into soci_test(id, val) values (:id, :val)",
                       use(id), use(val));

                id = 1;
                val = 10;
                st.execute(true);

                id = 2;
                val = std::optional<int>();
                st.execute(true);

                id = 3;
                val = 11;
                st.execute(true);
            }

            // verify values are fetched correctly
            {
                indicator ind;
                std::optional<int> opt;

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 1", into(opt, ind);
                CHECK(ind == i_ok);
                CHECK(opt.has_value());
                CHECK(opt.value() == 10);

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 2", into(opt, ind);
                CHECK(ind == i_null);
                CHECK(!opt.has_value());

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 3", into(opt, ind);
                CHECK(ind == i_ok);
                REQUIRE(opt.has_value());
                CHECK(opt.value() == 11);
            }
        }
    }
}
#endif

// connection and reconnection tests
TEST_CASE_METHOD(common_tests, "Connection and reconnection", "[core][connect]")
{
    {
        // empty session
        soci::session sql;

        CHECK(!sql.is_connected());

        // idempotent:
        sql.close();

        try
        {
            sql.reconnect();
            FAIL("expected exception not thrown");
        }
        catch (soci_error const &e)
        {
            CHECK(e.get_error_message() ==
               "Cannot reconnect without previous connection.");
        }

        // open from empty session
        sql.open(backEndFactory_, connectString_);
        CHECK(sql.is_connected());
        sql.close();
        CHECK(!sql.is_connected());

        // reconnecting from closed session
        sql.reconnect();
        CHECK(sql.is_connected());

        // opening already connected session
        try
        {
            sql.open(backEndFactory_, connectString_);
            FAIL("expected exception not thrown");
        }
        catch (soci_error const &e)
        {
            CHECK(e.get_error_message() ==
               "Cannot open already connected session.");
        }

        CHECK(sql.is_connected());
        sql.close();

        // open from closed
        sql.open(backEndFactory_, connectString_);
        CHECK(sql.is_connected());

        // reconnect from already connected session
        sql.reconnect();
        CHECK(sql.is_connected());
    }

    {
        soci::session sql;

        try
        {
            sql << "this statement cannot execute";
            FAIL("expected exception not thrown");
        }
        catch (soci_error const &e)
        {
            CHECK(e.get_error_message() ==
                "Session is not connected.");
        }
    }

    {
        // check move semantics of session

        #if  __GNUC__ >= 13 || defined (__clang__)
        SOCI_GCC_WARNING_SUPPRESS(self-move)
        #endif

        soci::session sql_0;
        soci::session sql_1 = std::move(sql_0);

        CHECK(!sql_0.is_connected());
        CHECK(!sql_1.is_connected());

        sql_0.open(backEndFactory_, connectString_);
        CHECK(sql_0.is_connected());
        CHECK(sql_0.get_backend());

        sql_1 = std::move(sql_0);
        CHECK(!sql_0.is_connected());
        CHECK(!sql_0.get_backend());
        CHECK(sql_1.is_connected());
        CHECK(sql_1.get_backend());

        sql_1 = std::move(sql_1);
        CHECK(sql_1.is_connected());
        CHECK(sql_1.get_backend());

        #if __GNUC__ >= 13 || defined (__clang__)
        SOCI_GCC_WARNING_RESTORE(self-move)
        #endif
    }
}

// connection pool - simple sequential test, no multiple threads
TEST_CASE_METHOD(common_tests, "Connection pool", "[core][connection][pool]")
{
    // phase 1: preparation
    const size_t pool_size = 10;
    connection_pool pool(pool_size);

    for (std::size_t i = 0; i != pool_size; ++i)
    {
        session & sql = pool.at(i);
        sql.open(backEndFactory_, connectString_);
    }

    // phase 2: usage
    for (std::size_t i = 0; i != pool_size; ++i)
    {
        // poor man way to lease more than one connection
        soci::session sql_unused1(pool);
        soci::session sql(pool);
        soci::session sql_unused2(pool);
        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            char c('a');
            sql << "insert into soci_test(c) values(:c)", use(c);
            sql << "select c from soci_test", into(c);
            CHECK(c == 'a');
        }
    }
}

// Issue 66 - test query transformation callback feature
static std::string no_op_transform(std::string query)
{
    return query;
}

static std::string lower_than_g(std::string query)
{
    return query + " WHERE c < 'g'";
}

struct where_condition
{
    where_condition(std::string const& where)
        : where_(where)
    {}

    std::string operator()(std::string const& query) const
    {
        return query + " WHERE " + where_;
    }

    std::string where_;
};


void run_query_transformation_test(test_context_base const& tc, session& sql)
{
    // create and populate the test table
    auto_table_creator tableCreator(tc.table_creator_1(sql));

    for (char c = 'a'; c <= 'z'; ++c)
    {
        sql << "insert into soci_test(c) values(\'" << c << "\')";
    }

    char const* query = "select count(*) from soci_test";

    // free function, no-op
    {
        sql.set_query_transformation(no_op_transform);
        int count;
        sql << query, into(count);
        CHECK(count == 'z' - 'a' + 1);
    }

    // free function
    {
        sql.set_query_transformation(lower_than_g);
        int count;
        sql << query, into(count);
        CHECK(count == 'g' - 'a');
    }

    // function object with state
    {
        sql.set_query_transformation(where_condition("c > 'g' AND c < 'j'"));
        int count = 0;
        sql << query, into(count);
        CHECK(count == 'j' - 'h');
        count = 0;
        sql.set_query_transformation(where_condition("c > 's' AND c <= 'z'"));
        sql << query, into(count);
        CHECK(count == 'z' - 's');
    }

#if 0
    // lambda is just presented as an example to curious users
    {
        sql.set_query_transformation(
            [](std::string const& query) {
                return query + " WHERE c > 'g' AND c < 'j'";
        });

        int count = 0;
        sql << query, into(count);
        CHECK(count == 'j' - 'h');
    }
#endif

    // prepared statements

    // constant effect (pre-prepare set transformation)
    {
        // set transformation after statement is prepared
        sql.set_query_transformation(lower_than_g);
        // prepare statement
        int count;
        statement st = (sql.prepare << query, into(count));
        // observe transformation effect
        st.execute(true);
        CHECK(count == 'g' - 'a');
        // reset transformation
        sql.set_query_transformation(no_op_transform);
        // observe the same transformation, no-op set above has no effect
        count = 0;
        st.execute(true);
        CHECK(count == 'g' - 'a');
    }

    // no effect (post-prepare set transformation)
    {
        // reset
        sql.set_query_transformation(no_op_transform);

        // prepare statement
        int count;
        statement st = (sql.prepare << query, into(count));
        // set transformation after statement is prepared
        sql.set_query_transformation(lower_than_g);
        // observe no effect of WHERE clause injection
        st.execute(true);
        CHECK(count == 'z' - 'a' + 1);
    }
}

TEST_CASE_METHOD(common_tests, "Query transformation", "[core][query-transform]")
{
    soci::session sql(backEndFactory_, connectString_);
    run_query_transformation_test(tc_, sql);
}

TEST_CASE_METHOD(common_tests, "Query transformation with connection pool", "[core][query-transform][pool]")
{
    // phase 1: preparation
    const size_t pool_size = 10;
    connection_pool pool(pool_size);

    for (std::size_t i = 0; i != pool_size; ++i)
    {
        session & sql = pool.at(i);
        sql.open(backEndFactory_, connectString_);
    }

    soci::session sql(pool);
    run_query_transformation_test(tc_, sql);
}

// Originally, submitted to SQLite3 backend and later moved to common test.
// Test commit b394d039530f124802d06c3b1a969c3117683152
// Author: Mika Fischer <mika.fischer@zoopnet.de>
// Date:   Thu Nov 17 13:28:07 2011 +0100
// Implement get_affected_rows for SQLite3 backend
TEST_CASE_METHOD(common_tests, "Get affected rows", "[core][affected-rows]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_4(sql));
    if (!tableCreator.get())
    {
        WARN("test get_affected_rows skipped (function not implemented)");
        return;
    }


    for (int i = 0; i != 10; i++)
    {
        sql << "insert into soci_test(val) values(:val)", use(i);
    }

    int step = 2;
    statement st1 = (sql.prepare <<
        "update soci_test set val = val + :step where val = 5", use(step, "step"));
    st1.execute(true);
    CHECK(st1.get_affected_rows() == 1);

    // attempts to run the query again, no rows should be affected
    st1.execute(true);
    CHECK(st1.get_affected_rows() == 0);

    statement st2 = (sql.prepare <<
        "update soci_test set val = val + 1");
    st2.execute(true);

    CHECK(st2.get_affected_rows() == 10);

    statement st3 = (sql.prepare <<
        "delete from soci_test where val <= 5");
    st3.execute(true);

    CHECK(st3.get_affected_rows() == 5);

    statement st4 = (sql.prepare <<
        "update soci_test set val = val + 1");
    st4.execute(true);

    CHECK(st4.get_affected_rows() == 5);

    std::vector<int> v(5, 0);
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        v[i] = (7 + static_cast<int>(i));
    }

    // test affected rows for bulk operations.
    statement st5 = (sql.prepare <<
        "delete from soci_test where val = :v", use(v));
    st5.execute(true);

    CHECK(st5.get_affected_rows() == 5);

    if (tc_.has_partial_update_bug())
    {
        WARN("Skipping partial update test due to a known backend bug");
        return;
    }

    std::vector<std::string> w(2, "1");
    w[1] = "a"; // this invalid value may cause an exception.
    statement st6 = (sql.prepare <<
        "insert into soci_test(val) values(:val)", use(w));
    CHECK_THROWS_AS(st6.execute(true), soci_error);
    CHECK(st6.get_affected_rows() == 1);

    // confirm the partial insertion.
    int val = 0;
    sql << "select count(val) from soci_test", into(val);
    CHECK(val == 1);
}

// test fix for: Backend is not set properly with connection pool (pull #5)
TEST_CASE_METHOD(common_tests, "Backend with connection pool", "[core][pool]")
{
    const size_t pool_size = 1;
    connection_pool pool(pool_size);

    for (std::size_t i = 0; i != pool_size; ++i)
    {
        session & sql = pool.at(i);
        sql.open(backEndFactory_, connectString_);
    }

    soci::session sql(pool);
    sql.reconnect();
    sql.begin(); // no crash expected
}

// test fix for: Session from connection pool not set backend properly when call open
TEST_CASE_METHOD(common_tests, "Session from connection pool call open reset backend", "[core][pool]")
{
    const size_t pool_size = 1;
    connection_pool pool(pool_size);

    soci::session sql(pool);
    sql.open(backEndFactory_, connectString_);
    REQUIRE_NOTHROW( sql.begin() );
}

// issue 67 - Allocated statement backend memory leaks on exception
// If the test runs under memory debugger and it passes, then
// soci::details::statement_impl::backEnd_ must not leak
TEST_CASE_METHOD(common_tests, "Backend memory leak", "[core][leak]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    try
    {
        rowset<row> rs1 = (sql.prepare << "select * from soci_testX");

        // TODO: On Linux, no exception thrown; neither from prepare, nor from execute?
        // soci_odbc_test_postgresql:
        //     /home/travis/build/SOCI/soci/src/core/test/common-tests.h:3505:
        //     void soci::tests::common_tests::test_issue67(): Assertion `!"exception expected"' failed.
        //FAIL("exception expected"); // relax temporarily
    }
    catch (soci_error const &e)
    {
        (void)e;
    }
}

// issue 154 - Calling undefine_and_bind and then define_and_bind causes a leak.
// If the test runs under memory debugger and it passes, then
// soci::details::standard_use_type_backend and vector_use_type_backend must not leak
TEST_CASE_METHOD(common_tests, "Bind memory leak", "[core][leak]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    sql << "insert into soci_test(id) values (1)";
    {
        int id = 1;
        int val = 0;
        statement st(sql);
        st.exchange(use(id));
        st.alloc();
        st.prepare("select id from soci_test where id = :1");
        st.define_and_bind();
        st.undefine_and_bind();
        st.exchange(soci::into(val));
        st.define_and_bind();
        st.execute(true);
        CHECK(val == 1);
    }
    // vector variation
    {
        std::vector<int> ids(1, 2);
        std::vector<int> vals(1, 1);
        int val = 0;
        statement st(sql);
        st.exchange(use(ids));
        st.alloc();
        st.prepare("insert into soci_test(id, val) values (:1, :2)");
        st.define_and_bind();
        st.undefine_and_bind();
        st.exchange(use(vals));
        st.define_and_bind();
        st.execute(true);
        sql << "select val from soci_test where id = 2", into(val);
        CHECK(val == 1);
    }
}

// Helper functions for issue 723 test
namespace {

    // Creates a std::tm with UK DST threshold 31st March 2019 01:00:00
    std::tm create_uk_dst_threshold()
    {
        std::tm dst_threshold = std::tm();
        dst_threshold.tm_year = 119;  // 2019
        dst_threshold.tm_mon = 2;     // March
        dst_threshold.tm_mday = 31;   // 31st
        dst_threshold.tm_hour = 1;    // 1AM
        dst_threshold.tm_min = 0;
        dst_threshold.tm_sec = 0;
        dst_threshold.tm_isdst = -1;   // Determine DST from OS
        return dst_threshold;
    }

    // Sanity check to verify that the DST threshold causes mktime to modify
    // the input hour (the condition that causes issue 723).
    // This check really shouldn't fail but since it is the basis of the test
    // it is worth verifying.
    bool does_mktime_modify_input_hour()
    {
        std::tm dst_threshold = create_uk_dst_threshold();
        std::tm verify_mktime = dst_threshold;
        mktime(&verify_mktime);
        return verify_mktime.tm_hour != dst_threshold.tm_hour;
    }

    // We don't have any way to change the time zone for just this process
    // under MSW, so we just skip this test when not running in UK time-zone
    // there. Under Unix systems we can however switch to UK time zone
    // temporarily by just setting the TZ environment variable.
#ifndef _WIN32
    // Helper RAII class changing time zone to the specified one in its ctor
    // and restoring the original time zone in its dtor.
    class tz_setter
    {
    public:
        explicit tz_setter(const std::string& time_zone)
        {
            char* tz_value = getenv("TZ");
            if (tz_value != NULL)
            {
                original_tz_value_ = tz_value;
            }

            setenv("TZ", time_zone.c_str(), 1 /* overwrite */);
            tzset();
        }

        ~tz_setter()
        {
            // Restore TZ value so other tests aren't affected.
            if (original_tz_value_.empty())
                unsetenv("TZ");
            else
                setenv("TZ", original_tz_value_.c_str(), 1);
            tzset();
        }

    private:
        std::string original_tz_value_;
    };
#endif // !_WIN32
}

// Issue 723 - std::tm timestamp problem with DST.
// When reading date/time on Daylight Saving Time threshold, hour value is
// silently changed.
TEST_CASE_METHOD(common_tests, "std::tm timestamp problem with DST", "[core][into][tm][dst]")
{
#ifdef _WIN32
    if (!does_mktime_modify_input_hour())
    {
        WARN("The DST test can only be run in the UK time zone, please switch to it manually.");
        return;
    }
#else // !_WIN32
    // Set UK timezone for this test scope.
    tz_setter switch_to_UK_tz("Europe/London");

    if (!does_mktime_modify_input_hour())
    {
        WARN("Switching to the UK time zone unexpectedly failed, skipping the DST test.");
        return;
    }
#endif // _WIN32/!_WIN32

    // Open session and create table with a date/time column.
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    // Round trip dst threshold time to database.
    std::tm write_time = create_uk_dst_threshold();
    sql << "insert into soci_test(tm) values(:tm)", use(write_time);
    std::tm read_time = std::tm();
    sql << "select tm from soci_test", soci::into(read_time);

    // Check that the round trip was consistent.
    std::tm dst_threshold = create_uk_dst_threshold();
    CHECK(read_time.tm_year == dst_threshold.tm_year);
    CHECK(read_time.tm_mon == dst_threshold.tm_mon);
    CHECK(read_time.tm_mday == dst_threshold.tm_mday);
    CHECK(read_time.tm_hour == dst_threshold.tm_hour);
    CHECK(read_time.tm_min == dst_threshold.tm_min);
    CHECK(read_time.tm_sec == dst_threshold.tm_sec);
}

TEST_CASE_METHOD(common_tests, "Insert error", "[core][insert][exception]")
{
    soci::session sql(backEndFactory_, connectString_);

    struct pk_table_creator : table_creator_base
    {
        explicit pk_table_creator(session& sql) : table_creator_base(sql)
        {
            // For some backends (at least Firebird), it is important to
            // execute the DDL statements in a separate transaction, so start
            // one here and commit it before using the new table below.
            sql.begin();
            sql << "create table soci_test("
                        "name varchar(100) not null primary key, "
                        "age integer not null"
                   ")";
            sql.commit();
        }
    } table_creator(sql);

    SECTION("literal SQL queries appear in the error message")
    {
        sql << "insert into soci_test(name, age) values ('John', 74)";
        sql << "insert into soci_test(name, age) values ('Paul', 72)";
        sql << "insert into soci_test(name, age) values ('George', 72)";

        try
        {
            // Oops, this should have been 'Ringo'
            sql << "insert into soci_test(name, age) values ('John', 74)";

            FAIL("exception expected on unique constraint violation not thrown");
        }
        catch (soci_error const &e)
        {
            REQUIRE_THAT(e.what(),
                Catch::Contains("insert into soci_test(name, age) values ('John', 74)")
            );
        }
    }

    SECTION("SQL queries parameters appear in the error message")
    {
        char const* const names[] = { "John", "Paul", "George", "John", NULL };
        int const ages[] = { 74, 72, 72, 74, 0 };

        std::string name;
        int age;

        statement st = (sql.prepare <<
            "insert into soci_test(name, age) values (:name, :age)",
            use(name), use(age));
        try
        {
            int const *a = ages;
            for (char const* const* n = names; *n; ++n, ++a)
            {
                name = *n;
                age = *a;
                st.execute(true);
            }

            FAIL("exception expected on unique constraint violation with prepared statement not thrown");
        }
        catch (soci_error const &e)
        {
            // Oracle converts all parameter names to upper case internally, so
            // we must check for the substring case-insensitively.
            REQUIRE_THAT(e.what(),
                Catch::Contains(R"(with :name="John", :age=74)", Catch::CaseSensitive::No)
            );
        }
    }

    SECTION("SQL queries vector parameters appear in the error message")
    {
        std::vector<std::string> names{"John", "Paul", "George", "John"};
        std::vector<int> ages{74, 72, 72, 74};

        statement st = (sql.prepare <<
            "insert into soci_test(name, age) values (:name, :age)",
            use(names), use(ages));
        try
        {
            st.execute(true);

            FAIL("exception expected on unique constraint violation with prepared bulk statement not thrown");
        }
        catch (soci_error const &e)
        {
            // Unfortunately, some backends don't provide the values here but
            // just use the generic "<vector>" placeholder. Don't fail the test
            // just because of that.
            std::string const msg = e.what();

            if (msg.find("<vector>") == std::string::npos)
            {
                REQUIRE_THAT(msg,
                    Catch::Contains(R"(with :name="John", :age=74)", Catch::CaseSensitive::No)
                );
            }
        }
    }
}

namespace
{

// This is just a helper to avoid duplicating the same code in two sections in
// the test below, it's logically part of it.
void check_for_exception_on_truncation(session& sql)
{
    // As the name column has length 20, inserting a longer string into it
    // shouldn't work, unless we're dealing with a database that doesn't
    // respect column types at all (hello SQLite).
    try
    {
        std::string const long_name("George Raymond Richard Martin");
        sql << "insert into soci_test(name) values(:name)", use(long_name);

        // If insert didn't throw, it should have at least preserved the data
        // (only SQLite does this currently).
        std::string name;
        sql << "select name from soci_test", into(name);
        CHECK(name == long_name);
    }
    catch (soci_error const &)
    {
        // Unfortunately the contents of the message differ too much between
        // the backends (most give an error about value being "too long",
        // Oracle says "too large" while SQL Server (via ODBC) just says that
        // it "would be truncated"), so we can't really check that we received
        // the right error here -- be optimistic and hope that we did.
    }
}

// And another helper for the test below.
void check_for_no_truncation(session& sql, bool with_padding)
{
    const std::string str20 = "exactly of length 20";

    sql << "delete from soci_test";

    // Also check that there is no truncation when inserting a string of
    // the same length as the column size.
    CHECK_NOTHROW( (sql << "insert into soci_test(name) values(:s)", use(str20)) );

    std::string s;
    sql << "select name from soci_test", into(s);

    // Firebird can pad CHAR(N) columns when using UTF-8 encoding.
    // the result will be padded to 80 bytes (UTF-8 max for 20 chars)
    if (with_padding)
      CHECK_EQUAL_PADDED(s, str20)
    else
      CHECK( s == str20 );
}

} // anonymous namespace

TEST_CASE_METHOD(common_tests, "Truncation error", "[core][insert][truncate][exception]")
{
    soci::session sql(backEndFactory_, connectString_);

    if (tc_.has_silent_truncate_bug(sql))
    {
        WARN("Database is broken and silently truncates input data.");
        return;
    }

    SECTION("Error given for char column")
    {
        struct fixed_name_table_creator : table_creator_base
        {
            fixed_name_table_creator(session& sql)
                : table_creator_base(sql)
            {
                sql << "create table soci_test(name char(20))";
            }
        } tableCreator(sql);

        tc_.on_after_ddl(sql);

        check_for_exception_on_truncation(sql);

        // Firebird can pad CHAR(N) columns when using UTF-8 encoding.
        check_for_no_truncation(sql, sql.get_backend_name() == "firebird");
    }

    SECTION("Error given for varchar column")
    {
        // Reuse one of the standard tables which has a varchar(20) column.
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        check_for_exception_on_truncation(sql);

        check_for_no_truncation(sql, false);
    }
}

TEST_CASE_METHOD(common_tests, "Blank padding", "[core][insert][exception]")
{
    soci::session sql(backEndFactory_, connectString_);
    if (!tc_.enable_std_char_padding(sql))
    {
        WARN("This backend doesn't pad CHAR(N) correctly, skipping test.");
        return;
    }

    struct fixed_name_table_creator : table_creator_base
    {
        fixed_name_table_creator(session& sql)
            : table_creator_base(sql)
        {
            sql.begin();
            sql << "create table soci_test(sc char, name char(10), name2 varchar(10))";
            sql.commit();
        }
    } tableCreator(sql);

    std::string test1 = "abcde     ";
    std::string singleChar = "a";
    sql << "insert into soci_test(sc, name,name2) values(:sc,:name,:name2)",
            use(singleChar), use(test1), use(test1);

    std::string sc, tchar,tvarchar;
    sql << "select sc,name,name2 from soci_test",
            into(sc), into(tchar), into(tvarchar);

    // Firebird can pad "a" to "a   " when using UTF-8 encoding.
    CHECK_EQUAL_PADDED(sc, singleChar);
    CHECK_EQUAL_PADDED(tchar, test1);
    CHECK(tvarchar == test1);

    // Check 10-space string - same as inserting empty string since spaces will
    // be padded up to full size of the column.
    test1 = "          ";
    singleChar = " ";
    sql << "update soci_test set sc=:sc, name=:name, name2=:name2",
            use(singleChar), use(test1), use(test1);
    sql << "select sc, name,name2 from soci_test",
            into(sc), into(tchar), into(tvarchar);

    CHECK_EQUAL_PADDED(sc, singleChar);
    CHECK_EQUAL_PADDED(tchar, test1);
    CHECK(tvarchar == test1);
}

TEST_CASE_METHOD(common_tests, "Select without table", "[core][select][dummy_from]")
{
    soci::session sql(backEndFactory_, connectString_);

    int plus17;
    sql << ("select abs(-17)" + sql.get_dummy_from_clause()),
           into(plus17);

    CHECK(plus17 == 17);
}

TEST_CASE_METHOD(common_tests, "String length", "[core][string][length]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    std::string s("123");
    REQUIRE_NOTHROW((
        sql << "insert into soci_test(str) values(:s)", use(s)
    ));

    std::string sout;
    size_t slen;
    REQUIRE_NOTHROW((
        sql << "select str," + tc_.sql_length("str") + " from soci_test",
           into(sout), into(slen)
    ));
    CHECK(slen == 3);
    CHECK(sout.length() == 3);
    CHECK(sout == s);

    sql << "delete from soci_test";


    std::vector<std::string> v;
    v.push_back("Hello");
    v.push_back("");
    v.push_back("whole of varchar(20)");

    REQUIRE_NOTHROW((
        sql << "insert into soci_test(str) values(:s)", use(v)
    ));

    std::vector<std::string> vout(10);
    // Although none of the strings here is really null, Oracle handles the
    // empty string as being null, so to avoid an error about not providing
    // the indicator when retrieving a null value, we must provide it here.
    std::vector<indicator> vind(10);
    std::vector<unsigned int> vlen(10);

    REQUIRE_NOTHROW((
        sql << "select str," + tc_.sql_length("str") + " from soci_test"
               " order by " + tc_.sql_length("str"),
               into(vout, vind), into(vlen)
    ));

    REQUIRE(vout.size() == 3);
    REQUIRE(vlen.size() == 3);

    CHECK(vlen[0] == 0);
    CHECK(vout[0].length() == 0);

    CHECK(vlen[1] == 5);
    CHECK(vout[1].length() == 5);

    CHECK(vlen[2] == 20);
    CHECK(vout[2].length() == 20);
}

TEST_CASE_METHOD(common_tests, "Logger", "[core][log]")
{
    // Logger class used for testing: appends all queries to the provided
    // buffer.
    class test_log_impl : public soci::logger_impl
    {
    public:
        explicit test_log_impl(std::vector<std::string>& logbuf)
            : m_logbuf(logbuf)
        {
        }

        virtual void start_query(std::string const & query)
        {
            m_logbuf.push_back(query);
        }

    private:
        virtual logger_impl* do_clone() const
        {
            return new test_log_impl(m_logbuf);
        }

        std::vector<std::string>& m_logbuf;
    };

    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    soci::logger const logger_orig = sql.get_logger();

    std::vector<std::string> logbuf;
    sql.set_logger(new test_log_impl(logbuf));

    int count;
    sql << "select count(*) from soci_test", into(count);

    REQUIRE( logbuf.size() == 1 );
    CHECK( logbuf.front() == "select count(*) from soci_test" );

    sql.set_logger(logger_orig);
}

} // namespace test_cases

// Implement test_context_common ctor here: like this, just using this class
// pulls in the tests defined in this file.
//
// These variables are defined in other files we want to force linking with.
extern volatile bool soci_use_test_boost;
extern volatile bool soci_use_test_connparams;
extern volatile bool soci_use_test_custom;
extern volatile bool soci_use_test_dynamic;
extern volatile bool soci_use_test_lob;
extern volatile bool soci_use_test_manual;
extern volatile bool soci_use_test_rowset;

test_context_common::test_context_common()
{
#ifdef SOCI_HAVE_BOOST
    soci_use_test_boost = true;
#endif

    soci_use_test_connparams = true;
    soci_use_test_custom = true;
    soci_use_test_dynamic = true;
    soci_use_test_lob = true;
    soci_use_test_manual = true;
    soci_use_test_rowset = true;
}

} // namespace tests

} // namespace soci
