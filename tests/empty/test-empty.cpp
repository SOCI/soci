//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/empty/soci-empty.h"
#include "test-context.h"

#include <catch.hpp>

#include <string>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = *soci::factory_empty();

// NOTE:
// This file is supposed to serve two purposes:
// 1. To be a starting point for implementing new tests (for new backends).
// 2. To exercise (at least some of) the syntax and try the SOCI library
//    against different compilers, even in those environments where there
//    is no database. SOCI uses advanced template techniques which are known
//    to cause problems on different versions of popular compilers, and this
//    test is handy to verify that the code is accepted by as many compilers
//    as possible.
//
// Both of these purposes mean that the actual code here is meaningless
// from the database-development point of view. For new tests, you may wish
// to remove this code and keep only the general structure of this file.

struct Person
{
    int id;
    std::string firstName;
    std::string lastName;
};

namespace soci
{
    template<> struct type_conversion<Person>
    {
        typedef values base_type;
        static void from_base(values & /* r */, indicator /* ind */,
            Person & /* p */)
        {
        }
    };
}

TEST_CASE("Dummy test", "[empty]")
{
    soci::session sql(backEnd, connectString);

    sql << "Do what I want.";
    sql << "Do what I want " << 123 << " times.";

    char const* const query = "some query";
    sql << query;

    {
        std::string squery = "some query";
        sql << squery;
    }

    int i = 7;
    sql << "insert", use(i);
    sql << "select", into(i);
    sql << query, use(i);
    sql << query, into(i);

#if defined (__LP64__) || ( __WORDSIZE == 64 )
    long int li = 9;
    sql << "insert", use(li);
    sql << "select", into(li);
#endif

    long long ll = 11;
    sql << "insert", use(ll);
    sql << "select", into(ll);

    indicator ind = i_ok;
    sql << "insert", use(i, ind);
    sql << "select", into(i, ind);
    sql << query, use(i, ind);
    sql << query, use(i, ind);

    std::vector<int> numbers(100);
    sql << "insert", use(numbers);
    sql << "select", into(numbers);

    std::vector<indicator> inds(100);
    sql << "insert", use(numbers, inds);
    sql << "select", into(numbers, inds);

    {
        statement st = (sql.prepare << "select", into(i));
        st.execute();
        st.fetch();
    }
    {
        statement st = (sql.prepare << query, into(i));
        st.execute();
        st.fetch();
    }
    {
        statement st = (sql.prepare << "select", into(i, ind));
        statement sq = (sql.prepare << query, into(i, ind));
    }
    {
        statement st = (sql.prepare << "select", into(numbers));
    }
    {
        statement st = (sql.prepare << "select", into(numbers, inds));
    }
    {
        statement st = (sql.prepare << "insert", use(i));
        statement sq = (sql.prepare << query, use(i));
    }
    {
        statement st = (sql.prepare << "insert", use(i, ind));
        statement sq = (sql.prepare << query, use(i, ind));
    }
    {
        statement st = (sql.prepare << "insert", use(numbers));
    }
    {
        statement st = (sql.prepare << "insert", use(numbers, inds));
    }
    {
        Person p;
        sql << "select person", into(p);
    }
}

// Each test must define the test context class which implements the base class
// pure virtual functions in a backend-specific way.
//
// Unlike in all the other tests we inherit directly from test_context_base and
// not test_context_common because we don't want to link in, and hence run, all
// the common tests that would fail for this dummy empty backend.
class test_context :public test_context_base
{
public:
    test_context() = default;

    std::string get_example_connection_string() const override
    {
        return "connect_string_for_empty_backend";
    }

    std::string get_backend_name() const override
    {
        return "empty";
    }

    // As we don't use common tests with this pseudo-backend, we don't actually
    // need to implement these functions -- but they still must be defined.
    std::string to_date_time(std::string const&) const override
    {
        FAIL("Not implemented");
        return {};
    }

    table_creator_base* table_creator_1(session&) const override
    {
        FAIL("Not implemented");
        return nullptr;
    }

    table_creator_base* table_creator_2(session&) const override
    {
        FAIL("Not implemented");
        return nullptr;
    }

    table_creator_base* table_creator_3(session&) const override
    {
        FAIL("Not implemented");
        return nullptr;
    }

    table_creator_base* table_creator_4(session&) const override
    {
        FAIL("Not implemented");
        return nullptr;
    }


    std::string sql_length(std::string const&) const override
    {
        FAIL("Not implemented");
        return {};
    }
};

test_context tc_empty;
