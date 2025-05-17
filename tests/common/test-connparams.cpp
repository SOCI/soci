//
// Copyright (C) 2024 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

#include "test-context.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_connparams = false;

// A helper to check that parsing the given connection string works.
connection_parameters parse_connection_string(std::string const& connstr)
{
    connection_parameters params(backEnd, connstr);

    REQUIRE_NOTHROW(params.extract_options_from_space_separated_string());

    return params;
}

// A similar one checking that the given connection string is invalid.
void check_invalid_connection_string(std::string const& connstr)
{
    INFO(R"(Parsing invalid connection string ")" << connstr << R"(")");

    connection_parameters params(backEnd, connstr);

    CHECK_THROWS_AS(params.extract_options_from_space_separated_string(),
                    soci_error);
}

// Another helper to check that the given option has the expected value.
void
check_option(connection_parameters const& params,
             char const* name,
             std::string const& expected)
{
    std::string value;
    if ( params.get_option(name, value) )
    {
        CHECK(value == expected);
    }
    else
    {
        FAIL_CHECK(R"(Option ")" << name << R"(" not found)");
    }
}

TEST_CASE("Connection string parsing", "[core][connstring]")
{
    SECTION("Invalid")
    {
        connection_parameters params(backEnd, "");

        // Missing name.
        check_invalid_connection_string("=");
        check_invalid_connection_string("foo=ok =bar");

        // Missing quote.
        check_invalid_connection_string(R"(foo=")");
        check_invalid_connection_string(R"(foo="bar)");
        check_invalid_connection_string(R"(foo="bar" baz="quux )");
        check_invalid_connection_string(R"(foo=')");
        check_invalid_connection_string(R"(foo='bar)");
        check_invalid_connection_string(R"(foo='bar' baz='quux )");

        // This one is not invalid (empty values are allowed), but it check
        // because it used to dereference an invalid iterator (see #1175).
        REQUIRE_NOTHROW(parse_connection_string("bloordyblop="));
    }

    SECTION("Typical")
    {
        std::string const s = "dbname=/some/path host=some.where readonly=1 port=1234";
        INFO(R"(Parsing connection string ")" << s << R"(")");

        auto params = parse_connection_string(s);

        check_option(params, "dbname", "/some/path");
        check_option(params, "host", "some.where");
        check_option(params, "port", "1234");
        check_option(params, "readonly", "1");

        std::string value;
        CHECK_FALSE(params.get_option("user", value));
    }

    SECTION("Double Quotes")
    {
        std::string const s = R"(user="foo" pass="" service="bar baz")";
        INFO(R"(Parsing connection string ")" << s << R"(")");

        auto params = parse_connection_string(s);

        check_option(params, "user", "foo");
        check_option(params, "pass", "");
        check_option(params, "service", "bar baz");
    }

    SECTION("Single Quotes")
    {
        std::string const s = R"(user='foo' pass='' service='bar baz')";
        INFO(R"(Parsing connection string ")" << s << R"(")");

        auto params = parse_connection_string(s);

        check_option(params, "user", "foo");
        check_option(params, "pass", "");
        check_option(params, "service", "bar baz");
    }
}

TEST_CASE("connection_parameters::extract_option", "[core][connstring]")
{
    std::string value;

    connection_parameters params(backEnd, "foo bar=baz");
    params.extract_options_from_space_separated_string();

    // Extracting an option must remove it.
    CHECK(params.extract_option("foo", value));
    CHECK(!params.get_option("foo", value));

    CHECK_FALSE(params.extract_option("baz", value));

    CHECK(params.extract_option("bar", value));
    CHECK(!params.get_option("bar", value));
}

TEST_CASE("connection_parameters::build_string_from_options", "[core][connstring]")
{
    connection_parameters params(backEnd, R"(foo bar="baz" quux="1 2")");
    params.extract_options_from_space_separated_string();

    // Check that unecessary quotes are removed, empty value are explicitly
    // specified and the required quotes are kept.
    CHECK(params.build_string_from_options('\'') == "bar=baz foo='' quux='1 2'");
}

// Test loading backend dynamically if possible (this symbol is defined by
// the build system if shared libraries are used).
#ifdef SOCI_DYNAMIC_BACKEND
TEST_CASE_METHOD(common_tests, "backend::load_by_name", "[backend][dynamic]")
{
    CHECK_NOTHROW(soci::session(tc_.get_backend_name(), tc_.get_connect_string()));
}
#endif // SOCI_DYNAMIC_BACKEND

} // namespace tests

} // namespace soci
