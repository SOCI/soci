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

namespace
{

// Fixture creating the session to be killed and another one used for killing
// it, which also owns the test table, so that it remains usable even if the
// session being tested can't reconnect.
class connection_loss_tests : public common_tests
{
protected:
    // Must be called first and the test must be skipped if it returns false.
    bool init()
    {
        std::string const sessionIdQuery = tc_.get_session_id_query();
        if (sessionIdQuery.empty())
        {
            WARN("Killing sessions not supported by this backend, skipping.");
            return false;
        }

        mainSession_.open(backEndFactory_, connectString_);

        tableCreator_.reset(tc_.table_creator_1(mainSession_));
        {
            soci::transaction tr(mainSession_);
            mainSession_ << "insert into soci_test(id) values (17)";
            tr.commit();
        }

        sql_.open(backEndFactory_,
                  tc_.get_connect_string_disabling_autoreconnect());

        sql_ << sessionIdQuery, into(id_);
        REQUIRE(!id_.empty());

        // If this fails, the database user probably lacks the permissions
        // needed to see (and kill) the other sessions, see the test context
        // functions.
        INFO("Checking that session " << id_ << " can be found from another one");
        REQUIRE(is_session_alive());

        return true;
    }

    // Kill the session being tested and wait until it's really gone.
    void kill_session()
    {
        // Don't fail just because the statement killing the session threw, as
        // it may still be killed (e.g. Oracle may give ORA-00031 "session
        // marked for kill" error), but remember the error to show it if the
        // session is not killed.
        std::string killError;
        try
        {
            mainSession_ << tc_.get_kill_session_statement(id_);
        }
        catch (soci_error const& e)
        {
            killError = e.what();
        }

        bool killed = false;
        for (int i = 0; i < 100; ++i)
        {
            if (!is_session_alive())
            {
                killed = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        INFO("Error killing session " << id_ << ": " << killError);
        REQUIRE(killed);
    }

    // Check that starting a new transaction fails after losing the connection
    // and doesn't succeed without contacting the server, as this would make
    // the error happen only later, when executing the first statement of the
    // new transaction, which is too late to transparently reconnect and retry.
    void check_begin_fails()
    {
        try
        {
            sql_.begin();
            FAIL_CHECK("Starting a transaction after losing the connection "
                       "unexpectedly succeeded");

            // Don't leave the transaction open, whatever happens.
            try
            {
                sql_.rollback();
            }
            catch (soci_error const&)
            {
            }
        }
        catch (soci_error const& e)
        {
            INFO("Error starting transaction after losing the connection: "
                 << e.what());
            CHECK(!sql_.is_connected());
        }
    }

    // Check that the session can be used again after reconnecting.
    void check_reconnect()
    {
        REQUIRE_NOTHROW(sql_.reconnect());
        CHECK(sql_.is_connected());

        soci::transaction tr(sql_);

        int n = 0;
        sql_ << "select id from soci_test", into(n);
        CHECK(n == 17);

        tr.commit();
    }

    // The session being tested.
    soci::session sql_;

private:
    bool is_session_alive()
    {
        int count = 0;
        mainSession_ << tc_.get_session_alive_query(id_), into(count);
        return count != 0;
    }

    soci::session mainSession_;
    auto_table_creator tableCreator_;

    // ID of sql_ session as returned by get_session_id_query().
    std::string id_;
};

} // anonymous namespace

// Losing the connection in the middle of a transaction must be reported and it
// must be possible to reconnect and use the session normally after it.
TEST_CASE_METHOD(connection_loss_tests, "Connection loss in transaction",
                 "[core][connect][reconnect]")
{
    if (!init())
        return;

    {
        // Use begin() and rollback() directly instead of soci::transaction, as
        // we need to check what happens when they fail.
        sql_.begin();

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
                sql_ << "select id from soci_test where id = :id",
                    use(idToRead), into(n);
            };
        }
        SECTION("Prepared statement")
        {
            st.reset(new statement((sql_.prepare <<
                "select id from soci_test where id = :id",
                use(idToRead), into(n))));
            read = [&]() { st->execute(true); };
        }

        // Really start the transaction in the database before losing the
        // connection, by reading something.
        read();
        CHECK(n == 17);

        kill_session();

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
            CHECK(!sql_.is_connected());
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
            sql_.rollback();
        }
        catch (soci_error const& e)
        {
            INFO("Error rolling back after losing the connection: " << e.what());
        }
    }

    check_begin_fails();

    check_reconnect();
}

// Losing the connection while not using the session at all can only be
// detected by is_connected(), which can be called by the application to check
// if the connection is still alive, and starting a transaction after it must
// fail too, even if no errors had been reported before.
TEST_CASE_METHOD(connection_loss_tests, "Connection loss detected by polling",
                 "[core][connect][reconnect]")
{
    if (!init())
        return;

    kill_session();

    // Calling is_connected() repeatedly must keep returning false.
    CHECK(!sql_.is_connected());
    CHECK(!sql_.is_connected());

    check_begin_fails();

    check_reconnect();
}

} // namespace tests

} // namespace soci
