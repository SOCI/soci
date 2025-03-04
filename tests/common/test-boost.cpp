//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#ifdef SOCI_HAVE_BOOST

// explicitly pull conversions for Boost's optional, tuple and fusion:
#include <boost/version.hpp>
#include "soci/boost-optional.h"
#include "soci/boost-tuple.h"
#include "soci/boost-gregorian-date.h"
#if defined(BOOST_VERSION) && BOOST_VERSION >= 103500
#include "soci/boost-fusion.h"
#endif // BOOST_VERSION

#include <catch.hpp>

#include <string>
#include <vector>

#include "test-assert.h"
#include "test-myint.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_boost = false;

// test for handling NULL values with boost::optional
// (both into and use)
TEST_CASE_METHOD(common_tests, "NULL with optional", "[core][boost][null]")
{

    soci::session sql(backEndFactory_, connectString_);

    // create and populate the test table
    auto_table_creator tableCreator0(tc_.table_creator_1(sql));
    {
        sql << "insert into soci_test(val) values(7)";

        {
            // verify non-null value is fetched correctly
            boost::optional<int> opt;
            sql << "select val from soci_test", into(opt);
            CHECK(opt.is_initialized());
            CHECK(opt.get() == 7);

            // indicators can be used with optional
            // (although that's just a consequence of implementation,
            // not an intended feature - but let's test it anyway)
            indicator ind;
            opt.reset();
            sql << "select val from soci_test", into(opt, ind);
            CHECK(opt.is_initialized());
            CHECK(opt.get() == 7);
            CHECK(ind == i_ok);

            // verify null value is fetched correctly
            sql << "select i1 from soci_test", into(opt);
            CHECK(opt.is_initialized() == false);

            // and with indicator
            opt = 5;
            sql << "select i1 from soci_test", into(opt, ind);
            CHECK(opt.is_initialized() == false);
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

            std::vector<boost::optional<int> > v(10);
            sql << "select val from soci_test order by val", into(v);

            CHECK(v.size() == 5);
            CHECK(v[0].is_initialized());
            CHECK(v[0].get() == 5);
            CHECK(v[1].is_initialized());
            CHECK(v[1].get() == 6);
            CHECK(v[2].is_initialized());
            CHECK(v[2].get() == 7);
            CHECK(v[3].is_initialized());
            CHECK(v[3].get() == 8);
            CHECK(v[4].is_initialized());
            CHECK(v[4].get() == 9);

            // readout of nulls

            sql << "update soci_test set val = null where id = 2 or id = 4";

            std::vector<int> ids(5);
            sql << "select id, val from soci_test order by id", into(ids), into(v);

            CHECK(v.size() == 5);
            CHECK(ids.size() == 5);
            CHECK(v[0].is_initialized());
            CHECK(v[0].get() == 5);
            CHECK(v[1].is_initialized() == false);
            CHECK(v[2].is_initialized());
            CHECK(v[2].get() == 7);
            CHECK(v[3].is_initialized() == false);
            CHECK(v[4].is_initialized());
            CHECK(v[4].get() == 9);

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
                        CHECK(v[i].is_initialized() == false);
                    }
                    else
                    {
                        CHECK(v[i].is_initialized());
                        CHECK(v[i].get() == id + 4);
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
            //CHECK(r1.get_properties(0).get_exchnage_data_type() == db_int32);
            CHECK(r1.get_properties(1).get_data_type() == dt_integer);
            CHECK(r1.get_properties(1).get_db_type() == db_int32);
            CHECK(r1.get_properties(2).get_data_type() == dt_string);
            CHECK(r1.get_properties(2).get_db_type() == db_string);
            //CHECK(r1.get<int>(0) == 1);
            CHECK(r1.get<int>(1) == 5);
            CHECK(r1.get<std::string>(2) == "abc");
            CHECK(r1.get<boost::optional<int> >(1).is_initialized());
            CHECK(r1.get<boost::optional<int> >(1).get() == 5);
            CHECK(r1.get<boost::optional<std::string> >(2).is_initialized());
            CHECK(r1.get<boost::optional<std::string> >(2).get() == "abc");

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
            CHECK(r2.get<boost::optional<int> >(1).is_initialized() == false);

            // stream-like data extraction

            ++it;
            row const &r3 = (*it);

            boost::optional<int> io;
            boost::optional<std::string> so;

            r3.skip(); // move to val and str columns
            r3 >> io >> so;

            CHECK(io.is_initialized());
            CHECK(io.get() == 7);
            CHECK(so.is_initialized());
            CHECK(so.get() == "ghi");

            ++it;
            row const &r4 = (*it);

            r3.skip(); // move to val and str columns
            r4 >> io >> so;

            CHECK(io.is_initialized() == false);
            CHECK(so.is_initialized() == false);
        }

        // inserts of non-null const data
        {
            sql << "delete from soci_test";

            const int id = 10;
            const boost::optional<int> val = 11;

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
            std::vector<boost::optional<int> > v;

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
            std::vector<boost::optional<int> > v;

            ids.push_back(10); v.push_back(20);
            ids.push_back(11); v.push_back(21);
            ids.push_back(12); v.push_back(22);
            ids.push_back(13); v.push_back(23);

            const std::vector<int>& cref_ids = ids;
            const std::vector<boost::optional<int> >& cref_v = v;

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

            boost::optional<MyInt> omi1;
            boost::optional<MyInt> omi2;

            omi1 = MyInt(125);
            omi2.reset();

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(omi1), use(omi2);

            sql << "select id, val from soci_test", into(omi2), into(omi1);

            CHECK(omi1.is_initialized() == false);
            CHECK(omi2.is_initialized());
            CHECK(omi2.get().get() == 125);
        }

        // use with const optional and user conversions

        {
            sql << "delete from soci_test";

            boost::optional<MyInt> omi1;
            boost::optional<MyInt> omi2;

            omi1 = MyInt(125);
            omi2.reset();

            boost::optional<MyInt> const & comi1 = omi1;
            boost::optional<MyInt> const & comi2 = omi2;

            sql << "insert into soci_test(id, val) values(:id, :val)",
                use(comi1), use(comi2);

            sql << "select id, val from soci_test", into(omi2), into(omi1);

            CHECK(omi1.is_initialized() == false);
            CHECK(omi2.is_initialized());
            CHECK(omi2.get().get() == 125);
        }

        // use with rowset and table containing null values

        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            sql << "insert into soci_test(id, val) values(1, 10)";
            sql << "insert into soci_test(id, val) values(2, 11)";
            sql << "insert into soci_test(id, val) values(3, NULL)";
            sql << "insert into soci_test(id, val) values(4, 13)";

            rowset<boost::optional<int> > rs = (sql.prepare <<
                "select val from soci_test order by id asc");

            // 1st row
            rowset<boost::optional<int> >::const_iterator pos = rs.begin();
            CHECK((*pos).is_initialized());
            CHECK(10 == (*pos).get());

            // 2nd row
            ++pos;
            CHECK((*pos).is_initialized());
            CHECK(11 == (*pos).get());

            // 3rd row
            ++pos;
            CHECK((*pos).is_initialized() == false);

            // 4th row
            ++pos;
            CHECK((*pos).is_initialized());
            CHECK(13 == (*pos).get());
        }

        // inserting using an i_null indicator with a boost::optional should
        // insert null, even if the optional is valid, just as with standard
        // types
        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            {
                indicator ind = i_null;
                boost::optional<int> v1(10);
                sql << "insert into soci_test(id, val) values(1, :val)",
                       use(v1, ind);
            }

            // verify the value is fetched correctly as null
            {
                indicator ind;
                boost::optional<int> opt;

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 1", into(opt, ind);
                CHECK(ind == i_null);
                CHECK(!opt.is_initialized());
            }
        }

        // prepared statement inserting non-null and null values alternatively
        // (without passing an explicit indicator)
        {
            auto_table_creator tableCreator(tc_.table_creator_1(sql));

            {
                int id;
                boost::optional<int> val;
                statement st = (sql.prepare
                    << "insert into soci_test(id, val) values (:id, :val)",
                       use(id), use(val));

                id = 1;
                val = 10;
                st.execute(true);

                id = 2;
                val = boost::optional<int>();
                st.execute(true);

                id = 3;
                val = 11;
                st.execute(true);
            }

            // verify values are fetched correctly
            {
                indicator ind;
                boost::optional<int> opt;

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 1", into(opt, ind);
                CHECK(ind == i_ok);
                CHECK(opt.is_initialized());
                CHECK(opt.get() == 10);

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 2", into(opt, ind);
                CHECK(ind == i_null);
                CHECK(!opt.is_initialized());

                ind = i_truncated;
                opt = 0;
                sql << "select val from soci_test where id = 3", into(opt, ind);
                CHECK(ind == i_ok);
                REQUIRE(opt.is_initialized());
                CHECK(opt.get() == 11);
            }
        }
    }
}

TEST_CASE_METHOD(common_tests, "Boost tuple", "[core][boost][tuple]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_2(sql));
    {
        boost::tuple<double, int, std::string> t1(3.5, 7, "Joe Hacker");
        ASSERT_EQUAL(t1.get<0>(), 3.5);
        CHECK(t1.get<1>() == 7);
        CHECK(t1.get<2>() == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // basic query

        boost::tuple<double, int, std::string> t2;
        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(t2.get<0>(), 3.5);
        CHECK(t2.get<1>() == 7);
        CHECK(t2.get<2>() == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // composability with boost::optional

        // use:
        boost::tuple<double, boost::optional<int>, std::string> t1(
            3.5, boost::optional<int>(7), "Joe Hacker");
        ASSERT_EQUAL(t1.get<0>(), 3.5);
        CHECK(t1.get<1>().is_initialized());
        CHECK(t1.get<1>().get() == 7);
        CHECK(t1.get<2>() == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::tuple<double, boost::optional<int>, std::string> t2;
        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(t2.get<0>(), 3.5);
        CHECK(t2.get<1>().is_initialized());
        CHECK(t2.get<1>().get() == 7);
        CHECK(t2.get<2>() == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // composability with user-provided conversions

        // use:
        boost::tuple<double, MyInt, std::string> t1(3.5, 7, "Joe Hacker");
        ASSERT_EQUAL(t1.get<0>(), 3.5);
        CHECK(t1.get<1>().get() == 7);
        CHECK(t1.get<2>() == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::tuple<double, MyInt, std::string> t2;

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(t2.get<0>(), 3.5);
        CHECK(t2.get<1>().get() == 7);
        CHECK(t2.get<2>() == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // let's have fun - composition of tuple, optional and user-defined type

        // use:
        boost::tuple<double, boost::optional<MyInt>, std::string> t1(
            3.5, boost::optional<MyInt>(7), "Joe Hacker");
        ASSERT_EQUAL(t1.get<0>(), 3.5);
        CHECK(t1.get<1>().is_initialized());
        CHECK(t1.get<1>().get().get() == 7);
        CHECK(t1.get<2>() == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::tuple<double, boost::optional<MyInt>, std::string> t2;

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(t2.get<0>(), 3.5);
        CHECK(t2.get<1>().is_initialized());
        CHECK(t2.get<1>().get().get() == 7);
        CHECK(t2.get<2>() == "Joe Hacker");

        sql << "update soci_test set num_int = NULL";

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(t2.get<0>(), 3.5);
        CHECK(t2.get<1>().is_initialized() == false);
        CHECK(t2.get<2>() == "Joe Hacker");
    }

    {
        // rowset<tuple>

        sql << "insert into soci_test(num_float, num_int, name) values(4.0, 8, 'Tony Coder')";
        sql << "insert into soci_test(num_float, num_int, name) values(4.5, NULL, 'Cecile Sharp')";
        sql << "insert into soci_test(num_float, num_int, name) values(5.0, 10, 'Djhava Ravaa')";

        typedef boost::tuple<double, boost::optional<int>, std::string> T;

        rowset<T> rs = (sql.prepare
            << "select num_float, num_int, name from soci_test order by num_float asc");

        rowset<T>::const_iterator pos = rs.begin();

        ASSERT_EQUAL(pos->get<0>(), 3.5);
        CHECK(pos->get<1>().is_initialized() == false);
        CHECK(pos->get<2>() == "Joe Hacker");

        ++pos;
        ASSERT_EQUAL(pos->get<0>(), 4.0);
        CHECK(pos->get<1>().is_initialized());
        CHECK(pos->get<1>().get() == 8);
        CHECK(pos->get<2>() == "Tony Coder");

        ++pos;
        ASSERT_EQUAL(pos->get<0>(), 4.5);
        CHECK(pos->get<1>().is_initialized() == false);
        CHECK(pos->get<2>() == "Cecile Sharp");

        ++pos;
        ASSERT_EQUAL(pos->get<0>(),  5.0);
        CHECK(pos->get<1>().is_initialized());
        CHECK(pos->get<1>().get() == 10);
        CHECK(pos->get<2>() == "Djhava Ravaa");

        ++pos;
        CHECK(pos == rs.end());
    }
}

#if defined(BOOST_VERSION) && BOOST_VERSION >= 103500

TEST_CASE_METHOD(common_tests, "Boost fusion", "[core][boost][fusion]")
{

    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_2(sql));
    {
        boost::fusion::vector<double, int, std::string> t1(3.5, 7, "Joe Hacker");
        ASSERT_EQUAL(boost::fusion::at_c<0>(t1), 3.5);
        CHECK(boost::fusion::at_c<1>(t1) == 7);
        CHECK(boost::fusion::at_c<2>(t1) == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // basic query

        boost::fusion::vector<double, int, std::string> t2;
        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(boost::fusion::at_c<0>(t2), 3.5);
        CHECK(boost::fusion::at_c<1>(t2) == 7);
        CHECK(boost::fusion::at_c<2>(t2) == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // composability with boost::optional

        // use:
        boost::fusion::vector<double, boost::optional<int>, std::string> t1(
            3.5, boost::optional<int>(7), "Joe Hacker");
        ASSERT_EQUAL(boost::fusion::at_c<0>(t1), 3.5);
        CHECK(boost::fusion::at_c<1>(t1).is_initialized());
        CHECK(boost::fusion::at_c<1>(t1).get() == 7);
        CHECK(boost::fusion::at_c<2>(t1) == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::fusion::vector<double, boost::optional<int>, std::string> t2;
        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(boost::fusion::at_c<0>(t2), 3.5);
        CHECK(boost::fusion::at_c<1>(t2).is_initialized());
        CHECK(boost::fusion::at_c<1>(t2) == 7);
        CHECK(boost::fusion::at_c<2>(t2) == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // composability with user-provided conversions

        // use:
        boost::fusion::vector<double, MyInt, std::string> t1(3.5, 7, "Joe Hacker");
        ASSERT_EQUAL(boost::fusion::at_c<0>(t1), 3.5);
        CHECK(boost::fusion::at_c<1>(t1).get() == 7);
        CHECK(boost::fusion::at_c<2>(t1) == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::fusion::vector<double, MyInt, std::string> t2;

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(boost::fusion::at_c<0>(t2), 3.5);
        CHECK(boost::fusion::at_c<1>(t2).get() == 7);
        CHECK(boost::fusion::at_c<2>(t2) == "Joe Hacker");

        sql << "delete from soci_test";
    }

    {
        // let's have fun - composition of tuple, optional and user-defined type

        // use:
        boost::fusion::vector<double, boost::optional<MyInt>, std::string> t1(
            3.5, boost::optional<MyInt>(7), "Joe Hacker");
        ASSERT_EQUAL(boost::fusion::at_c<0>(t1), 3.5);
        CHECK(boost::fusion::at_c<1>(t1).is_initialized());
        CHECK(boost::fusion::at_c<1>(t1).get().get() == 7);
        CHECK(boost::fusion::at_c<2>(t1) == "Joe Hacker");

        sql << "insert into soci_test(num_float, num_int, name) values(:d, :i, :s)", use(t1);

        // into:
        boost::fusion::vector<double, boost::optional<MyInt>, std::string> t2;

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(boost::fusion::at_c<0>(t2), 3.5);
        CHECK(boost::fusion::at_c<1>(t2).is_initialized());
        CHECK(boost::fusion::at_c<1>(t2).get().get() == 7);
        CHECK(boost::fusion::at_c<2>(t2) == "Joe Hacker");

        sql << "update soci_test set num_int = NULL";

        sql << "select num_float, num_int, name from soci_test", into(t2);

        ASSERT_EQUAL(boost::fusion::at_c<0>(t2), 3.5);
        CHECK(boost::fusion::at_c<1>(t2).is_initialized() == false);
        CHECK(boost::fusion::at_c<2>(t2) == "Joe Hacker");
    }

    {
        // rowset<fusion::vector>

        sql << "insert into soci_test(num_float, num_int, name) values(4.0, 8, 'Tony Coder')";
        sql << "insert into soci_test(num_float, num_int, name) values(4.5, NULL, 'Cecile Sharp')";
        sql << "insert into soci_test(num_float, num_int, name) values(5.0, 10, 'Djhava Ravaa')";

        typedef boost::fusion::vector<double, boost::optional<int>, std::string> T;

        rowset<T> rs = (sql.prepare
            << "select num_float, num_int, name from soci_test order by num_float asc");

        rowset<T>::const_iterator pos = rs.begin();

        ASSERT_EQUAL(boost::fusion::at_c<0>(*pos), 3.5);
        CHECK(boost::fusion::at_c<1>(*pos).is_initialized() == false);
        CHECK(boost::fusion::at_c<2>(*pos) == "Joe Hacker");

        ++pos;
        ASSERT_EQUAL(boost::fusion::at_c<0>(*pos), 4.0);
        CHECK(boost::fusion::at_c<1>(*pos).is_initialized());
        CHECK(boost::fusion::at_c<1>(*pos).get() == 8);
        CHECK(boost::fusion::at_c<2>(*pos) == "Tony Coder");

        ++pos;
        ASSERT_EQUAL(boost::fusion::at_c<0>(*pos), 4.5);
        CHECK(boost::fusion::at_c<1>(*pos).is_initialized() == false);
        CHECK(boost::fusion::at_c<2>(*pos) == "Cecile Sharp");

        ++pos;
        ASSERT_EQUAL(boost::fusion::at_c<0>(*pos), 5.0);
        CHECK(boost::fusion::at_c<1>(*pos).is_initialized());
        CHECK(boost::fusion::at_c<1>(*pos).get() == 10);
        CHECK(boost::fusion::at_c<2>(*pos) == "Djhava Ravaa");

        ++pos;
        CHECK(pos == rs.end());
    }
}

#endif // defined(BOOST_VERSION) && BOOST_VERSION >= 103500

// test for boost::gregorian::date
TEST_CASE_METHOD(common_tests, "Boost date", "[core][boost][datetime]")
{
    soci::session sql(backEndFactory_, connectString_);

    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        std::tm nov15 = std::tm();
        nov15.tm_year = 105;
        nov15.tm_mon = 10;
        nov15.tm_mday = 15;
        nov15.tm_hour = 0;
        nov15.tm_min = 0;
        nov15.tm_sec = 0;

        sql << "insert into soci_test(tm) values(:tm)", use(nov15);

        boost::gregorian::date bgd;
        sql << "select tm from soci_test", into(bgd);

        CHECK(bgd.year() == 2005);
        CHECK(bgd.month() == 11);
        CHECK(bgd.day() == 15);

        sql << "update soci_test set tm = NULL";
        try
        {
            sql << "select tm from soci_test", into(bgd);
            FAIL("expected exception not thrown");
        }
        catch (soci_error const & e)
        {
            CHECK(e.get_error_message() ==
                "Null value not allowed for this type");
        }
    }

    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        boost::gregorian::date bgd(2008, boost::gregorian::May, 5);

        sql << "insert into soci_test(tm) values(:tm)", use(bgd);

        std::tm t = std::tm();
        sql << "select tm from soci_test", into(t);

        CHECK(t.tm_year == 108);
        CHECK(t.tm_mon == 4);
        CHECK(t.tm_mday == 5);
    }

}

} // namespace tests

} // namespace soci

#endif // SOCI_HAVE_BOOST
