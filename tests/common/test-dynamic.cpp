//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

#include "test-assert.h"
#include "test-myint.h"
#include "test-padded.h"

struct PhonebookEntry
{
    std::string name;
    std::string phone;
};

struct PhonebookEntry2 : public PhonebookEntry
{
};

class PhonebookEntry3
{
public:
    void setName(std::string const & n) { name_ = n; }
    std::string getName() const { return name_; }

    void setPhone(std::string const & p) { phone_ = p; }
    std::string getPhone() const { return phone_; }

public:
    std::string name_;
    std::string phone_;
};

namespace soci
{

// basic type conversion on many values (ORM)
template<> struct type_conversion<PhonebookEntry>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, PhonebookEntry &pe)
    {
        // here we ignore the possibility the the whole object might be NULL
        pe.name = v.get<std::string>("NAME");
        pe.phone = v.get<std::string>("PHONE", "<NULL>");
    }

    static void to_base(PhonebookEntry const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.name);
        v.set("PHONE", pe.phone, pe.phone.empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

// type conversion which directly calls values::get_indicator()
template<> struct type_conversion<PhonebookEntry2>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, PhonebookEntry2 &pe)
    {
        // here we ignore the possibility the the whole object might be NULL

        pe.name = v.get<std::string>("NAME");
        indicator ind = v.get_indicator("PHONE"); //another way to test for null
        pe.phone = ind == i_null ? "<NULL>" : v.get<std::string>("PHONE");
    }

    static void to_base(PhonebookEntry2 const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.name);
        v.set("PHONE", pe.phone, pe.phone.empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

template<> struct type_conversion<PhonebookEntry3>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, PhonebookEntry3 &pe)
    {
        // here we ignore the possibility the the whole object might be NULL

        pe.setName(v.get<std::string>("NAME"));
        pe.setPhone(v.get<std::string>("PHONE", "<NULL>"));
    }

    static void to_base(PhonebookEntry3 const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.getName());
        v.set("PHONE", pe.getPhone(), pe.getPhone().empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_dynamic = false;

TEST_CASE_METHOD(common_tests, "Prepared insert with ORM", "[core][orm]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);
    auto_table_creator tableCreator(tc_.table_creator_3(sql));

    PhonebookEntry temp;
    PhonebookEntry e1 = { "name1", "phone1" };
    PhonebookEntry e2 = { "name2", "phone2" };

    //sql << "insert into soci_test values (:NAME, :PHONE)", use(temp);
    statement insertStatement = (sql.prepare << "insert into soci_test values (:NAME, :PHONE)", use(temp));

    temp = e1;
    insertStatement.execute(true);
    temp = e2;
    insertStatement.execute(true);

    int count = 0;

    sql << "select count(*) from soci_test where NAME in ('name1', 'name2')", into(count);

    CHECK(count == 2);
}

TEST_CASE_METHOD(common_tests, "Partial match with ORM", "[core][orm]")
{
    soci::session sql(backEndFactory_, connectString_);
    sql.uppercase_column_names(true);
    auto_table_creator tableCreator(tc_.table_creator_3(sql));

    PhonebookEntry in = { "name1", "phone1" };
    std::string name = "nameA";
    sql << "insert into soci_test values (:NAMED, :PHONE)", use(in), use(name, "NAMED");

    PhonebookEntry out;
    sql << "select * from soci_test where PHONE = 'phone1'", into(out);
    CHECK(out.name == "nameA");
    CHECK(out.phone == "phone1");
}

// Dynamic binding to Row objects
TEST_CASE_METHOD(common_tests, "Dynamic row binding", "[core][dynamic]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);

    auto_table_creator tableCreator(tc_.table_creator_2(sql));

    row r;
    sql << "select * from soci_test", into(r);
    CHECK(sql.got_data() == false);

    sql << "insert into soci_test"
        " values(3.14, 123, \'Johny\',"
        << tc_.to_date_time("2005-12-19 22:14:17")
        << ", 'a')";

    // select into a row
    {
        statement st = (sql.prepare <<
            "select * from soci_test", into(r));
        st.execute(true);
        CHECK(r.size() == 5);

        CHECK(r.get_properties(0).get_data_type() == dt_double);
        CHECK(r.get_properties(0).get_db_type() == db_double);
        CHECK(r.get_properties(1).get_data_type() == dt_integer);
        CHECK(r.get_properties(1).get_db_type() == db_int32);
        CHECK(r.get_properties(2).get_data_type() == dt_string);
        CHECK(r.get_properties(2).get_db_type() == db_string);
        CHECK(r.get_properties(3).get_data_type() == dt_date);
        CHECK(r.get_properties(3).get_db_type() == db_date);

        // type char is visible as string
        // - to comply with the implementation for Oracle
        CHECK(r.get_properties(4).get_data_type() == dt_string);
        CHECK(r.get_properties(4).get_db_type() == db_string);

        CHECK(r.get_properties("NUM_INT").get_data_type() == dt_integer);
        CHECK(r.get_properties("NUM_INT").get_db_type() == db_int32);

        CHECK(r.get_properties(0).get_name() == "NUM_FLOAT");
        CHECK(r.get_properties(1).get_name() == "NUM_INT");
        CHECK(r.get_properties(2).get_name() == "NAME");
        CHECK(r.get_properties(3).get_name() == "SOMETIME");
        CHECK(r.get_properties(4).get_name() == "CHR");

        ASSERT_EQUAL_APPROX(r.get<double>(0), 3.14);
        CHECK(r.get<int>(1) == 123);
        CHECK(r.get<std::string>(2) == "Johny");
        CHECK(r.get<std::tm>(3).tm_year == 105);

        // again, type char is visible as string
        CHECK_EQUAL_PADDED(r.get<std::string>(4), "a");

        ASSERT_EQUAL_APPROX(r.get<double>("NUM_FLOAT"), 3.14);
        CHECK(r.get<int>("NUM_INT") == 123);
        CHECK(r.get<std::string>("NAME") == "Johny");
        CHECK_EQUAL_PADDED(r.get<std::string>("CHR"), "a");

        CHECK(r.get_indicator(0) == i_ok);

        // verify exception thrown on invalid get<>
        bool caught = false;
        try
        {
            r.get<std::string>(0);
        }
        catch (std::bad_cast const &)
        {
            caught = true;
        }
        CHECK(caught);

        // additional test for stream-like extraction
        {
            double d;
            int i;
            std::string s;
            std::tm t = std::tm();
            std::string c;

            r >> d >> i >> s >> t >> c;

            ASSERT_EQUAL_APPROX(d, 3.14);
            CHECK(i == 123);
            CHECK(s == "Johny");
            CHECK(t.tm_year == 105);
            CHECK(t.tm_mon == 11);
            CHECK(t.tm_mday == 19);
            CHECK(t.tm_hour == 22);
            CHECK(t.tm_min == 14);
            CHECK(t.tm_sec == 17);
            CHECK_EQUAL_PADDED(c, "a");
        }
    }

    // additional test to check if the row object can be
    // reused between queries
    {
        sql << "select * from soci_test", into(r);

        CHECK(r.size() == 5);

        CHECK(r.get_properties(0).get_data_type() == dt_double);
        CHECK(r.get_properties(0).get_db_type() == db_double);
        CHECK(r.get_properties(1).get_data_type() == dt_integer);
        CHECK(r.get_properties(1).get_db_type() == db_int32);
        CHECK(r.get_properties(2).get_data_type() == dt_string);
        CHECK(r.get_properties(2).get_db_type() == db_string);
        CHECK(r.get_properties(3).get_data_type() == dt_date);
        CHECK(r.get_properties(3).get_db_type() == db_date);

        sql << "select name, num_int from soci_test", into(r);

        CHECK(r.size() == 2);

        CHECK(r.get_properties(0).get_data_type() == dt_string);
        CHECK(r.get_properties(0).get_db_type() == db_string);
        CHECK(r.get_properties(1).get_data_type() == dt_integer);
        CHECK(r.get_properties(1).get_db_type() == db_int32);

        // Check if row object is movable
        row moved = std::move(r);

        CHECK(moved.size() == 2);
        // We expect the moved-from row to become empty after the move operation
        CHECK(r.size() == 0);

        CHECK(moved.get_properties(0).get_data_type() == dt_string);
        CHECK(moved.get_properties(0).get_db_type() == db_string);
        CHECK(moved.get_properties(1).get_data_type() == dt_integer);
        CHECK(moved.get_properties(1).get_db_type() == db_int32);
    }
}

// more dynamic bindings
TEST_CASE_METHOD(common_tests, "Dynamic row binding 2", "[core][dynamic]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    sql << "insert into soci_test(id, val) values(1, 10)";
    sql << "insert into soci_test(id, val) values(2, 20)";
    sql << "insert into soci_test(id, val) values(3, 30)";

    {
        int id = 2;
        row r;
        sql << "select val from soci_test where id = :id", use(id), into(r);

        CHECK(r.size() == 1);
        CHECK(r.get_properties(0).get_data_type() == dt_integer);
        CHECK(r.get_properties(0).get_db_type() == db_int32);
        CHECK(r.get<int>(0) == 20);
        CHECK(r.get<int32_t>(0) == 20);
    }
    {
        int id;
        row r;
        statement st = (sql.prepare <<
            "select val from soci_test where id = :id", use(id), into(r));

        id = 2;
        st.execute(true);
        CHECK(r.size() == 1);
        CHECK(r.get_properties(0).get_data_type() == dt_integer);
        CHECK(r.get_properties(0).get_db_type() == db_int32);
        CHECK(r.get<int>(0) == 20);
        CHECK(r.get<int32_t>(0) == 20);

        id = 3;
        st.execute(true);
        CHECK(r.size() == 1);
        CHECK(r.get_properties(0).get_data_type() == dt_integer);
        CHECK(r.get_properties(0).get_db_type() == db_int32);
        CHECK(r.get<int>(0) == 30);
        CHECK(r.get<int32_t>(0) == 30);

        id = 1;
        st.execute(true);
        CHECK(r.size() == 1);
        CHECK(r.get_properties(0).get_data_type() == dt_integer);
        CHECK(r.get_properties(0).get_db_type() == db_int32);
        CHECK(r.get<int>(0) == 10);
        CHECK(r.get<int32_t>(0) == 10);
    }
}

// More Dynamic binding to row objects
TEST_CASE_METHOD(common_tests, "Dynamic row binding 3", "[core][dynamic]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);

    auto_table_creator tableCreator(tc_.table_creator_3(sql));

    row r1;
    sql << "select * from soci_test", into(r1);
    CHECK(sql.got_data() == false);

    sql << "insert into soci_test values('david', '(404)123-4567')";
    sql << "insert into soci_test values('john', '(404)123-4567')";
    sql << "insert into soci_test values('doe', '(404)123-4567')";

    row r2;
    statement st = (sql.prepare << "select * from soci_test", into(r2));
    st.execute();

    CHECK(r2.size() == 2);

    int count = 0;
    while (st.fetch())
    {
        ++count;
        CHECK(r2.get<std::string>("PHONE") == "(404)123-4567");
    }
    CHECK(count == 3);
}

// This is like the previous test but with a type_conversion instead of a row
TEST_CASE_METHOD(common_tests, "Dynamic binding with type conversions", "[core][dynamic][type_conversion]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);

    SECTION("simple conversions")
    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        SECTION("between single basic type and user type")
        {
            MyInt mi;
            mi.set(123);
            sql << "insert into soci_test(id) values(:id)", use(mi);

            int i;
            sql << "select id from soci_test", into(i);
            CHECK(i == 123);

            sql << "update soci_test set id = id + 1";

            sql << "select id from soci_test", into(mi);
            CHECK(mi.get() == 124);
        }

        SECTION("with use const")
        {
            MyInt mi;
            mi.set(123);

            MyInt const & cmi = mi;
            sql << "insert into soci_test(id) values(:id)", use(cmi);

            int i;
            sql << "select id from soci_test", into(i);
            CHECK(i == 123);
        }
    }

    SECTION("ORM conversions")
    {
        auto_table_creator tableCreator(tc_.table_creator_3(sql));

        SECTION("conversions based on values")
        {
            PhonebookEntry p1;
            sql << "select * from soci_test", into(p1);
            CHECK(p1.name ==  "");
            CHECK(p1.phone == "");

            p1.name = "david";

            // Note: uppercase column names are used here (and later on)
            // for consistency with how they can be read from database
            // (which means forced to uppercase on Oracle) and how they are
            // set/get in the type conversion routines for PhonebookEntry.
            // In short, IF the database is Oracle,
            // then all column names for binding should be uppercase.
            sql << "insert into soci_test values(:NAME, :PHONE)", use(p1);
            sql << "insert into soci_test values('john', '(404)123-4567')";
            sql << "insert into soci_test values('doe', '(404)123-4567')";

            PhonebookEntry p2;
            statement st = (sql.prepare << "select * from soci_test", into(p2));
            st.execute();

            int count = 0;
            while (st.fetch())
            {
                ++count;
                if (p2.name == "david")
                {
                    // see type_conversion<PhonebookEntry>
                    CHECK(p2.phone =="<NULL>");
                }
                else
                {
                    CHECK(p2.phone == "(404)123-4567");
                }
            }
            CHECK(count == 3);
        }

        SECTION("conversions based on values with use const")
        {
            PhonebookEntry p1;
            p1.name = "Joe Coder";
            p1.phone = "123-456";

            PhonebookEntry const & cp1 = p1;

            sql << "insert into soci_test values(:NAME, :PHONE)", use(cp1);

            PhonebookEntry p2;
            sql << "select * from soci_test", into(p2);
            CHECK(sql.got_data());

            CHECK(p2.name == "Joe Coder");
            CHECK(p2.phone == "123-456");
        }

        SECTION("conversions based on accessor functions (as opposed to direct variable bindings)")
        {
            PhonebookEntry3 p1;
            p1.setName("Joe Hacker");
            p1.setPhone("10010110");

            sql << "insert into soci_test values(:NAME, :PHONE)", use(p1);

            PhonebookEntry3 p2;
            sql << "select * from soci_test", into(p2);
            CHECK(sql.got_data());

            CHECK(p2.getName() == "Joe Hacker");
            CHECK(p2.getPhone() == "10010110");
        }

        SECTION("PhonebookEntry2 type conversion to test calls to values::get_indicator()")
        {
            PhonebookEntry2 p1;
            sql << "select * from soci_test", into(p1);
            CHECK(p1.name ==  "");
            CHECK(p1.phone == "");
            p1.name = "david";

            sql << "insert into soci_test values(:NAME, :PHONE)", use(p1);
            sql << "insert into soci_test values('john', '(404)123-4567')";
            sql << "insert into soci_test values('doe', '(404)123-4567')";

            PhonebookEntry2 p2;
            statement st = (sql.prepare << "select * from soci_test", into(p2));
            st.execute();

            int count = 0;
            while (st.fetch())
            {
                ++count;
                if (p2.name == "david")
                {
                    // see type_conversion<PhonebookEntry2>
                    CHECK(p2.phone =="<NULL>");
                }
                else
                {
                    CHECK(p2.phone == "(404)123-4567");
                }
            }
            CHECK(count == 3);
        }
    }
}

// Dynamic bindings with type casts
TEST_CASE_METHOD(common_tests, "Dynamic row binding 4", "[core][dynamic]")
{
    soci::session sql(backEndFactory_, connectString_);

    SECTION("simple type cast")
    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        sql << "insert into soci_test(id, d, str, tm)"
            << " values(10, 20.0, 'foobar',"
            << tc_.to_date_time("2005-12-19 22:14:17")
            << ")";

        {
            row r;
            sql << "select id from soci_test", into(r);

            CHECK(r.size() == 1);
            CHECK(r.get<int8_t>(0) == 10);
            CHECK(r.get<int16_t>(0) == 10);
            CHECK(r.get<int32_t>(0) == 10);
            CHECK(r.get<int64_t>(0) == 10);
            CHECK(r.get<uint8_t>(0) == 10);
            CHECK(r.get<uint16_t>(0) == 10);
            CHECK(r.get<uint32_t>(0) == 10);
            CHECK(r.get<uint64_t>(0) == 10);
            CHECK_THROWS_AS(r.get<double>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<std::string>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<std::tm>(0), std::bad_cast);
        }
        {
            row r;
            sql << "select d from soci_test", into(r);

            CHECK(r.size() == 1);
            CHECK_THROWS_AS(r.get<int8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int64_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint64_t>(0), std::bad_cast);
            ASSERT_EQUAL_APPROX(r.get<double>(0), 20.0);
            CHECK_THROWS_AS(r.get<std::string>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<std::tm>(0), std::bad_cast);
        }
        {
            row r;
            sql << "select str from soci_test", into(r);

            CHECK(r.size() == 1);
            CHECK_THROWS_AS(r.get<int8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int64_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint64_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<double>(0), std::bad_cast);
            CHECK(r.get<std::string>(0) == "foobar");
            CHECK_THROWS_AS(r.get<std::tm>(0), std::bad_cast);
        }
        {
            row r;
            sql << "select tm from soci_test", into(r);

            CHECK(r.size() == 1);
            CHECK_THROWS_AS(r.get<int8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<int64_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint8_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint16_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint32_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<uint64_t>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<double>(0), std::bad_cast);
            CHECK_THROWS_AS(r.get<std::string>(0), std::bad_cast);
            CHECK(r.get<std::tm>(0).tm_year == 105);
            CHECK(r.get<std::tm>(0).tm_mon == 11);
            CHECK(r.get<std::tm>(0).tm_mday == 19);
            CHECK(r.get<std::tm>(0).tm_hour == 22);
            CHECK(r.get<std::tm>(0).tm_min == 14);
            CHECK(r.get<std::tm>(0).tm_sec == 17);
        }
    }
    SECTION("overflowing type cast")
    {
        auto_table_creator tableCreator(tc_.table_creator_1(sql));

        sql << "insert into soci_test(id)"
            << " values("
            << (std::numeric_limits<int32_t>::max)()
            << ")";

        row r;
        sql << "select id from soci_test", into(r);

        intmax_t v = (intmax_t)(std::numeric_limits<int32_t>::max)();
        uintmax_t uv = (uintmax_t)(std::numeric_limits<int32_t>::max)();

        CHECK(r.size() == 1);
        CHECK_THROWS_AS(r.get<int8_t>(0), std::bad_cast);
        CHECK_THROWS_AS(r.get<int16_t>(0), std::bad_cast);
        CHECK(r.get<int32_t>(0) == v);
        CHECK(r.get<int64_t>(0) == v);
        CHECK_THROWS_AS(r.get<uint8_t>(0), std::bad_cast);
        CHECK_THROWS_AS(r.get<uint16_t>(0), std::bad_cast);
        CHECK(r.get<uint32_t>(0) == uv);
        CHECK(r.get<uint64_t>(0) == uv);
    }
}

// This is like the first dynamic binding test but with rowset and iterators use
TEST_CASE_METHOD(common_tests, "Dynamic binding with rowset", "[core][dynamic][type_conversion]")
{
    soci::session sql(backEndFactory_, connectString_);

    sql.uppercase_column_names(true);

    {
        auto_table_creator tableCreator(tc_.table_creator_3(sql));

        PhonebookEntry p1;
        sql << "select * from soci_test", into(p1);
        CHECK(p1.name ==  "");
        CHECK(p1.phone == "");

        p1.name = "david";

        sql << "insert into soci_test values(:NAME, :PHONE)", use(p1);
        sql << "insert into soci_test values('john', '(404)123-4567')";
        sql << "insert into soci_test values('doe', '(404)123-4567')";

        rowset<PhonebookEntry> rs = (sql.prepare << "select * from soci_test");

        int count = 0;
        for (rowset<PhonebookEntry>::const_iterator it = rs.begin(); it != rs.end(); ++it)
        {
            ++count;
            PhonebookEntry const& p2 = (*it);
            if (p2.name == "david")
            {
                // see type_conversion<PhonebookEntry>
                CHECK(p2.phone =="<NULL>");
            }
            else
            {
                CHECK(p2.phone == "(404)123-4567");
            }
        }

        CHECK(3 == count);
    }
}

} // namespace tests

} // namespace soci
