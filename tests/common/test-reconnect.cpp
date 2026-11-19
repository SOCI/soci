//
// Copyright (C) 2026 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include <catch.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "test-context.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_reconnect = false;

// Tests in this file check what happens when the connection to the database
// is lost, which is simulated by killing the session from another one, so
// they only run for the backends implementing the corresponding functions of
// test_context_base.

// Losing the connection in the middle of a transaction must be reported and it
// must be possible to reconnect and use the session normally after it.
TEST_CASE_METHOD(common_tests, "Connection loss in transaction",
                 "[core][connect][reconnect]")
{
    std::string const sessionIdQuery = tc_.get_session_id_query();
    if (sessionIdQuery.empty())
    {
        WARN("Killing sessions not supported by this backend, skipping.");
        return;
    }

    // This session is used to kill the other one and also owns the table, so
    // that it remains usable even if the other session can't reconnect.
    soci::session mainSession(backEndFactory_, connectString_);

    auto_table_creator tableCreator(tc_.table_creator_1(mainSession));
    {
        soci::transaction tr(mainSession);
        mainSession << "insert into soci_test(id) values (17)";
        tr.commit();
    }

    soci::session
        sql(backEndFactory_, tc_.get_connect_string_disabling_autoreconnect());

    std::string id;
    sql << sessionIdQuery, into(id);
    REQUIRE(!id.empty());
    INFO("Session ID: " << id);

    auto const isSessionAlive = [&]()
    {
        int count = 0;
        mainSession << tc_.get_session_alive_query(id), into(count);
        return count != 0;
    };

    // If this fails, the database user probably lacks the permissions needed
    // to see (and kill) the other sessions, see the test context functions.
    INFO("Checking that the session can be found from another one");
    REQUIRE(isSessionAlive());

    {
        // Use begin() and rollback() directly instead of soci::transaction, as
        // we need to check what happens when they fail.
        sql.begin();

        // Read using either one-time queries or the same prepared statement
        // executed with different parameters, as both can behave differently.
        int n = 0;
        int idToRead = 17;
        std::function<void ()> read;
        std::unique_ptr<statement> st;
        SECTION("One-time query")
        {
            read = [&]()
            {
                sql << "select id from soci_test where id = :id",
                    use(idToRead), into(n);
            };
        }
        SECTION("Prepared statement")
        {
            st.reset(new statement((sql.prepare <<
                "select id from soci_test where id = :id",
                use(idToRead), into(n))));
            read = [&]() { st->execute(true); };
        }

        // Really start the transaction in the database before losing the
        // connection, by reading something.
        read();
        CHECK(n == 17);

        // Don't fail just because the statement killing the session threw, as
        // it may still be killed (e.g. Oracle may give ORA-00031 "session
        // marked for kill" error), but remember the error to show it if the
        // session is not killed.
        std::string killError;
        try
        {
            mainSession << tc_.get_kill_session_statement(id);
        }
        catch (soci_error const& e)
        {
            killError = e.what();
        }

        bool killed = false;
        for (int i = 0; i < 100; ++i)
        {
            if (!isSessionAlive())
            {
                killed = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        INFO("Error killing the session: " << killError);
        REQUIRE(killed);

        // Check that the error is really due to the connection loss, as
        // reading could fail for any other reason too.
        try
        {
            // Use an ID which doesn't exist, as the actual value is irrelevant
            // here.
            idToRead = 18;
            read();
            FAIL_CHECK("Reading after losing the connection unexpectedly succeeded");
        }
        catch (soci_error const& e)
        {
            INFO("Error after losing the connection: " << e.what());
            CHECK(!sql.is_connected());
            CHECK(e.get_error_category() == soci_error::connection_error);
        }

        // The statement can't be used any more, destroy it before trying to
        // use the session again.
        st.reset();

        // Rolling back the transaction may or may not fail, depending on the
        // backend, as the connection is lost, but it mustn't leave the session
        // in a state in which the loss can't be detected any more.
        try
        {
            sql.rollback();
        }
        catch (soci_error const& e)
        {
            INFO("Error rolling back after losing the connection: " << e.what());
        }
    }

    // Starting a new transaction must fail as well, and not succeed without
    // contacting the server, as this would make the error happen only later,
    // when executing the first statement of the new transaction, which is too
    // late to transparently reconnect and retry.
    try
    {
        sql.begin();
        FAIL_CHECK("Starting a transaction after losing the connection "
                   "unexpectedly succeeded");

        // Don't leave the transaction open, whatever happens.
        try
        {
            sql.rollback();
        }
        catch (soci_error const&)
        {
        }
    }
    catch (soci_error const& e)
    {
        INFO("Error starting transaction after losing the connection: "
             << e.what());
        CHECK(!sql.is_connected());
    }

    // But the session can be used again after reconnecting.
    REQUIRE_NOTHROW(sql.reconnect());
    CHECK(sql.is_connected());

    {
        soci::transaction tr(sql);

        int n = 0;
        sql << "select id from soci_test", into(n);
        CHECK(n == 17);

        tr.commit();
    }
}

} // namespace tests

} // namespace soci
