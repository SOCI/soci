//
// Copyright (C) 2004-2025 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include "soci-compiler.h"

#include <catch.hpp>

#include "test-context.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_querylog = false;

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

} // namespace tests

} // namespace soci
