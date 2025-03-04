//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton, Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <iomanip>
#include <iostream>

#include <catch.hpp>

#include "test-context.h"

// user-defined object for the "vector of custom type objects" tests.
class MyOptionalString
{
public:
    MyOptionalString() : valid_(false) {}
    MyOptionalString(const std::string& str) : valid_(true), str_(str) {}
    void set(const std::string& str) { valid_ = true; str_ = str; }
    void reset() { valid_ = false; str_.clear(); }
    bool is_valid() const { return valid_; }
    const std::string &get() const { return str_; }
private:
    bool valid_;
    std::string str_;
};

std::ostream& operator<<(std::ostream& ostr, const MyOptionalString& optstr)
{
  ostr << (optstr.is_valid() ? "\"" + optstr.get() + "\"" : std::string("(null)"));

  return ostr;
}

std::ostream& operator<<(std::ostream& ostr, const std::vector<MyOptionalString>& vec)
{
    if ( vec.empty() )
    {
        ostr << "Empty vector";
    }
    else
    {
        ostr << "Vector of size " << vec.size() << " containing\n";
        for ( size_t n = 0; n < vec.size(); ++n )
        {
            ostr << "\t [" << std::setw(3) << n << "] = " << vec[n] << "\n";
        }
    }

    return ostr;
}

namespace soci
{

// basic type conversion for string based user-defined type which can be null
template<> struct type_conversion<MyOptionalString>
{
    typedef std::string base_type;

    static void from_base(const base_type& in, indicator ind, MyOptionalString& out)
    {
        if (ind == i_null)
        {
            out.reset();
        }
        else
        {
            out.set(in);
        }
    }

    static void to_base(const MyOptionalString& in, base_type& out, indicator& ind)
    {
        if (in.is_valid())
        {
            out = in.get();
            ind = i_ok;
        }
        else
        {
            ind = i_null;
        }
    }
};

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_custom = false;

// use vector elements with type convertion
TEST_CASE_METHOD(common_tests, "Use vector of custom type objects", "[core][use][vector][type_conversion]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    // Unfortunately there is no portable way to indicate whether nulls should
    // appear at the beginning or the end (SQL 2003 "NULLS LAST" is still not
    // supported by MS SQL in 2021...), so use this column just to order by it.
    std::vector<int> i;
    i.push_back(0);
    i.push_back(1);
    i.push_back(2);

    std::vector<MyOptionalString> v;
    v.push_back(MyOptionalString("string")); // A not empty valid string.
    v.push_back(MyOptionalString());         // Invalid string mapped to null.
    v.push_back(MyOptionalString(""));       // An empty but still valid string.

    sql << "insert into soci_test(id, str) values(:i, :v)", use(i), use(v);

    SECTION("standard type")
    {
        std::vector<std::string> values(3);
        std::vector<indicator> inds(3);
        sql << "select str from soci_test order by id", into(values, inds);

        REQUIRE(values.size() == 3);
        REQUIRE(inds.size() == 3);

        CHECK(inds[0] == soci::i_ok);
        CHECK(values[0] == "string");

        CHECK(inds[1] == soci::i_null);

        if ( !tc_.treats_empty_strings_as_null() )
        {
            CHECK(inds[2] == soci::i_ok);
            CHECK(values[2] == "");
        }
    }

    SECTION("user type")
    {
        std::vector<MyOptionalString> values(3);
        std::vector<indicator> inds(3);
        sql << "select str from soci_test order by id", into(values, inds);

        REQUIRE(values.size() == 3);
        REQUIRE(inds.size() == 3);

        CHECK(inds[0] == soci::i_ok);
        CHECK(values[0].is_valid());
        CHECK(values[0].get() == "string");

        CHECK(!values[1].is_valid());
        CHECK(inds[1] == soci::i_null);

        if ( !tc_.treats_empty_strings_as_null() )
        {
            CHECK(inds[2] == soci::i_ok);
            CHECK(values[2].is_valid());
            CHECK(values[2].get() == "");
        }
    }
}

TEST_CASE_METHOD(common_tests, "Into vector of custom type objects", "[core][into][vector][type_conversion]")
{
    soci::session sql(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    // Column used for sorting only, see above.
    std::vector<int> i;
    i.push_back(0);
    i.push_back(1);
    i.push_back(2);

    std::vector<std::string> values(3);
    values[0] = "string";

    std::vector<indicator> inds;
    inds.push_back(i_ok);
    inds.push_back(i_null);
    inds.push_back(i_ok);

    sql << "insert into soci_test(id, str) values(:i, :v)", use(i), use(values, inds);

    std::vector<MyOptionalString> v2(4);

    sql << "select str from soci_test order by id", into(v2);

    INFO("Got back " << v2);
    REQUIRE(v2.size() == 3);

    CHECK(v2[0].is_valid());
    CHECK(v2[0].get() == "string");

    CHECK(!v2[1].is_valid());

    if ( !tc_.treats_empty_strings_as_null() )
    {
        CHECK(v2[2].is_valid());
        CHECK(v2[2].get().empty());
    }
}

} // namespace tests

} // namespace soci
