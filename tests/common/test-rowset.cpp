//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

#include "test-assert.h"
#include "test-padded.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_rowset = false;

// test for rowset creation and copying
TEST_CASE_METHOD(common_tests, "Rowset creation and copying", "[core][rowset]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    {
        // Create empty rowset
        rowset<row> rs1;
        CHECK(rs1.begin() == rs1.end());
    }
    {
        // Load empty rowset
        rowset<row> rs1 = (sql.prepare << "select * from soci_test");
        CHECK(rs1.begin() == rs1.end());
    }

    {
        // Copy construction
        rowset<row> rs1 = (sql.prepare << "select * from soci_test");
        rowset<row> rs2(rs1);
        rowset<row> rs3(rs1);
        rowset<row> rs4(rs3);

        CHECK(rs1.begin() == rs2.begin());
        CHECK(rs1.begin() == rs3.begin());
        CHECK(rs1.end() == rs2.end());
        CHECK(rs1.end() == rs3.end());
    }

    if (!tc_.has_multiple_select_bug())
    {
        // Assignment
        rowset<row> rs1;
        rowset<row> rs2 = (sql.prepare << "select * from soci_test");
        rowset<row> rs3 = (sql.prepare << "select * from soci_test");
        rs1 = rs2;
        rs3 = rs2;

        CHECK(rs1.begin() == rs2.begin());
        CHECK(rs1.begin() == rs3.begin());
        CHECK(rs1.end() == rs2.end());
        CHECK(rs1.end() == rs3.end());
    }
}

// test for simple iterating using rowset iterator (without reading data)
TEST_CASE_METHOD(common_tests, "Rowset iteration", "[core][rowset]")
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
            rowset<row> rs = (sql.prepare << "select * from soci_test");

            CHECK(5 == std::distance(rs.begin(), rs.end()));
        }
        {
            rowset<row> rs = (sql.prepare << "select * from soci_test");

            rs.clear();
            CHECK(rs.begin() == rs.end());
        }
    }

}

// test for reading rowset<row> using iterator
TEST_CASE_METHOD(common_tests, "Reading rows from rowset", "[core][row][rowset]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_2(sql));
    {
        {
            // Empty rowset
            rowset<row> rs = (sql.prepare << "select * from soci_test");
            CHECK(0 == std::distance(rs.begin(), rs.end()));
        }

        {
            // Non-empty rowset
            sql << "insert into soci_test values(3.14, 123, \'Johny\',"
                << tc_.to_date_time("2005-12-19 22:14:17")
                << ", 'a')";
            sql << "insert into soci_test values(6.28, 246, \'Robert\',"
                << tc_.to_date_time("2004-10-01 18:44:10")
                << ", 'b')";

            rowset<row> rs = (sql.prepare << "select * from soci_test");

            rowset<row>::const_iterator it = rs.begin();
            CHECK(it != rs.end());

            //
            // First row
            //
            row const & r1 = (*it);

            // Properties
            CHECK(r1.size() == 5);
            CHECK(r1.get_properties(0).get_data_type() == dt_double);
            CHECK(r1.get_properties(0).get_db_type() == db_double);
            CHECK(r1.get_properties(1).get_data_type() == dt_integer);
            CHECK(r1.get_properties(1).get_db_type() == db_int32);
            CHECK(r1.get_properties(2).get_data_type() == dt_string);
            CHECK(r1.get_properties(2).get_db_type() == db_string);
            CHECK(r1.get_properties(3).get_data_type() == dt_date);
            CHECK(r1.get_properties(3).get_db_type() == db_date);
            CHECK(r1.get_properties(4).get_data_type() == dt_string);
            CHECK(r1.get_properties(4).get_db_type() == db_string);
            CHECK(r1.get_properties("NUM_INT").get_data_type() == dt_integer);
            CHECK(r1.get_properties("NUM_INT").get_db_type() == db_int32);

            // Data

            // Since we didn't specify order by in the above query,
            // the 2 rows may be returned in either order
            // (If we specify order by, we can't do it in a cross db
            // compatible way, because the Oracle table for this has been
            // created with lower case column names)

            std::string name = r1.get<std::string>(2);

            if (name == "Johny")
            {
                ASSERT_EQUAL_APPROX(r1.get<double>(0), 3.14);
                CHECK(r1.get<int>(1) == 123);
                CHECK(r1.get<std::string>(2) == "Johny");
                std::tm t1 = std::tm();
                t1 = r1.get<std::tm>(3);
                CHECK(t1.tm_year == 105);
                CHECK_EQUAL_PADDED(r1.get<std::string>(4), "a");
                ASSERT_EQUAL_APPROX(r1.get<double>("NUM_FLOAT"), 3.14);
                CHECK(r1.get<int>("NUM_INT") == 123);
                CHECK(r1.get<std::string>("NAME") == "Johny");
                CHECK_EQUAL_PADDED(r1.get<std::string>("CHR"), "a");
            }
            else if (name == "Robert")
            {
                ASSERT_EQUAL(r1.get<double>(0), 6.28);
                CHECK(r1.get<int>(1) == 246);
                CHECK(r1.get<std::string>(2) == "Robert");
                std::tm t1 = r1.get<std::tm>(3);
                CHECK(t1.tm_year == 104);
                CHECK(r1.get<std::string>(4) == "b");
                ASSERT_EQUAL(r1.get<double>("NUM_FLOAT"), 6.28);
                CHECK(r1.get<int>("NUM_INT") == 246);
                CHECK(r1.get<std::string>("NAME") == "Robert");
                CHECK_EQUAL_PADDED(r1.get<std::string>("CHR"), "b");
            }
            else
            {
                CAPTURE(name);
                FAIL("expected \"Johny\" or \"Robert\"");
            }

            //
            // Iterate to second row
            //
            ++it;
            CHECK(it != rs.end());

            //
            // Second row
            //
            row const & r2 = (*it);

            // Properties
            CHECK(r2.size() == 5);
            CHECK(r2.get_properties(0).get_data_type() == dt_double);
            CHECK(r2.get_properties(0).get_db_type() == db_double);
            CHECK(r2.get_properties(1).get_data_type() == dt_integer);
            CHECK(r2.get_properties(1).get_db_type() == db_int32);
            CHECK(r2.get_properties(2).get_data_type() == dt_string);
            CHECK(r2.get_properties(2).get_db_type() == db_string);
            CHECK(r2.get_properties(3).get_data_type() == dt_date);
            CHECK(r2.get_properties(3).get_db_type() == db_date);
            CHECK(r2.get_properties(4).get_data_type() == dt_string);
            CHECK(r2.get_properties(4).get_db_type() == db_string);
            CHECK(r2.get_properties("NUM_INT").get_data_type() == dt_integer);
            CHECK(r2.get_properties("NUM_INT").get_db_type() == db_int32);

            std::string newName = r2.get<std::string>(2);
            CHECK(name != newName);

            if (newName == "Johny")
            {
                ASSERT_EQUAL_APPROX(r2.get<double>(0), 3.14);
                CHECK(r2.get<int>(1) == 123);
                CHECK(r2.get<std::string>(2) == "Johny");
                std::tm t2 = r2.get<std::tm>(3);
                CHECK(t2.tm_year == 105);
                CHECK(r2.get<std::string>(4) == "a");
                ASSERT_EQUAL_APPROX(r2.get<double>("NUM_FLOAT"), 3.14);
                CHECK(r2.get<int>("NUM_INT") == 123);
                CHECK(r2.get<std::string>("NAME") == "Johny");
                CHECK(r2.get<std::string>("CHR") == "a");
            }
            else if (newName == "Robert")
            {
                ASSERT_EQUAL_APPROX(r2.get<double>(0), 6.28);
                CHECK(r2.get<int>(1) == 246);
                CHECK(r2.get<std::string>(2) == "Robert");
                std::tm t2 = r2.get<std::tm>(3);
                CHECK(t2.tm_year == 104);
                CHECK_EQUAL_PADDED(r2.get<std::string>(4), "b");
                ASSERT_EQUAL_APPROX(r2.get<double>("NUM_FLOAT"), 6.28);
                CHECK(r2.get<int>("NUM_INT") == 246);
                CHECK(r2.get<std::string>("NAME") == "Robert");
                CHECK_EQUAL_PADDED(r2.get<std::string>("CHR"), "b");
            }
            else
            {
                CAPTURE(newName);
                FAIL("expected \"Johny\" or \"Robert\"");
            }
        }

        {
            // Non-empty rowset with NULL values
            sql << "insert into soci_test "
                << "(num_int, num_float , name, sometime, chr) "
                << "values (0, NULL, NULL, NULL, NULL)";

            rowset<row> rs = (sql.prepare
                     << "select num_int, num_float, name, sometime, chr "
                     << "from soci_test where num_int = 0");

            rowset<row>::const_iterator it = rs.begin();
            CHECK(it != rs.end());

            //
            // First row
            //
            row const& r1 = (*it);

            // Properties
            CHECK(r1.size() == 5);
            CHECK(r1.get_properties(0).get_data_type() == dt_integer);
            CHECK(r1.get_properties(0).get_db_type() == db_int32);
            CHECK(r1.get_properties(1).get_data_type() == dt_double);
            CHECK(r1.get_properties(1).get_db_type() == db_double);
            CHECK(r1.get_properties(2).get_data_type() == dt_string);
            CHECK(r1.get_properties(2).get_db_type() == db_string);
            CHECK(r1.get_properties(3).get_data_type() == dt_date);
            CHECK(r1.get_properties(3).get_db_type() == db_date);
            CHECK(r1.get_properties(4).get_data_type() == dt_string);
            CHECK(r1.get_properties(4).get_db_type() == db_string);

            // Data
            CHECK(r1.get_indicator(0) == soci::i_ok);
            CHECK(r1.get<int>(0) == 0);
            CHECK(r1.get_indicator(1) == soci::i_null);
            CHECK(r1.get_indicator(2) == soci::i_null);
            CHECK(r1.get_indicator(3) == soci::i_null);
            CHECK(r1.get_indicator(4) == soci::i_null);
        }
    }
}

// test for reading rowset<int> using iterator
TEST_CASE_METHOD(common_tests, "Reading ints from rowset", "[core][rowset]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(id) values(1)";
        sql << "insert into soci_test(id) values(2)";
        sql << "insert into soci_test(id) values(3)";
        sql << "insert into soci_test(id) values(4)";
        sql << "insert into soci_test(id) values(5)";
        {
            rowset<int> rs = (sql.prepare << "select id from soci_test order by id asc");

            // 1st row
            rowset<int>::const_iterator pos = rs.begin();
            CHECK(1 == (*pos));

            // 3rd row
            std::advance(pos, 2);
            CHECK(3 == (*pos));

            // 5th row
            std::advance(pos, 2);
            CHECK(5 == (*pos));

            // The End
            ++pos;
            CHECK(pos == rs.end());
        }
    }

}

// test for handling 'use' and reading rowset<std::string> using iterator
TEST_CASE_METHOD(common_tests, "Reading strings from rowset", "[core][rowset]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(str) values('abc')";
        sql << "insert into soci_test(str) values('def')";
        sql << "insert into soci_test(str) values('ghi')";
        sql << "insert into soci_test(str) values('jkl')";
        {
            // Expected result in numbers
            std::string idle("def");
            rowset<std::string> rs1 = (sql.prepare
                    << "select str from soci_test where str = :idle",
                    use(idle));

            CHECK(1 == std::distance(rs1.begin(), rs1.end()));

            // Expected result in value
            idle = "jkl";
            rowset<std::string> rs2 = (sql.prepare
                    << "select str from soci_test where str = :idle",
                    use(idle));

            CHECK(idle == *(rs2.begin()));
        }
    }

}

// test for handling troublemaker
TEST_CASE_METHOD(common_tests, "Rowset expected exception", "[core][exception][rowset]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    sql << "insert into soci_test(str) values('abc')";

    std::string troublemaker;
    CHECK_THROWS_AS(
        rowset<std::string>((sql.prepare << "select str from soci_test", into(troublemaker))),
        soci_error
        );
}

// functor for next test
struct THelper
{
    THelper()
        : val_()
    {
    }
    void operator()(int i)
    {
        val_ = i;
    }
    int val_;
};

// test for handling NULL values with expected exception:
// "Null value fetched and no indicator defined."
TEST_CASE_METHOD(common_tests, "NULL expected exception", "[core][exception][null]")
{
    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator(tc_.table_creator_1(sql));
    sql << "insert into soci_test(val) values(1)";
    sql << "insert into soci_test(val) values(2)";
    sql << "insert into soci_test(val) values(NULL)";
    sql << "insert into soci_test(val) values(3)";

    rowset<int> rs = (sql.prepare << "select val from soci_test order by val asc");

    CHECK_THROWS_AS( std::for_each(rs.begin(), rs.end(), THelper()), soci_error );
}

} // namespace tests

} // namespace soci
