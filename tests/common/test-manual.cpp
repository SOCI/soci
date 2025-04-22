//
// Copyright (C) 2004-2024 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"

#include "soci/callbacks.h"

#include <catch.hpp>

#include <iostream>

#include "test-context.h"

namespace soci
{

namespace tests
{

// This variable is referenced from test-common.cpp to force linking this file.
volatile bool soci_use_test_manual = false;

// These tests are disabled by default, as they require manual intevention, but
// can be run by explicitly giving their names on the command line.

// Check if reconnecting to the database after losing connection to it works.
TEST_CASE_METHOD(common_tests, "Reconnect", "[keep-alive][.]")
{
    soci::session sql(backEndFactory_, connectString_);
    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    int id = 17;
    sql << "insert into soci_test (id) values (:id)", use(id);

    REQUIRE_NOTHROW( sql.commit() );
    CHECK( sql.is_connected() );

    std::cout << "Please break connection to the database "
                 "(stop the server, unplug the network cable, ...) "
                 "and press Enter" << std::endl;
    std::cin.get();

    try
    {
        CHECK( !sql.is_connected() );

        int id2;
        sql << "select id from soci_test", into(id2);

        FAIL("Connection to the database still available");
        return;
    }
    catch (soci_error const& e)
    {
        if ( sql.get_backend_name() == "odbc" ||
                e.get_error_category() == soci_error::unknown )
        {
            WARN( "Skipping error check because ODBC driver returned "
                  "unknown error: " << e.what() );
        }
        else
        {
            INFO( "Exception message: " << e.what() );
            CHECK( e.get_error_category() == soci_error::connection_error );
        }
    }

    std::cout << "Please undo the previous action "
                 "(restart the server, plug the cable back, ...) "
                 "and press Enter" << std::endl;
    std::cin.get();

    REQUIRE_NOTHROW( sql.reconnect() );
    CHECK( sql.is_connected() );

    int id2 = 1234;
    sql << "select id from soci_test", into(id2);
    CHECK( id2 == id );
}

// Check if automatically reconnecting to the database works.
//
// Note: this test doesn't work at all, failover doesn't happen neither with
// Oracle nor with PostgreSQL (which are the only backends for which it's
// implemented at all) and it's not clear how is it even supposed to work.
TEST_CASE_METHOD(common_tests, "Failover", "[keep-alive][.]")
{
    soci::session sql(backEndFactory_, connectString_);

    class MyCallback : public soci::failover_callback
    {
    public:
        MyCallback() : attempted_(false), reconnected_(false)
        {
        }

        bool did_reconnect() const { return reconnected_; }

        void started() override
        {
            std::cout << "Please undo the previous action "
                         "(restart the server, plug the cable back, ...) "
                         "and press Enter" << std::endl;
            std::cin.get();
        }

        void failed(bool& retry, std::string&) override
        {
            // We only retry once.
            retry = !attempted_;
            attempted_ = true;
        }

        void finished(soci::session&) override
        {
            reconnected_ = true;
        }

        void aborted() override
        {
            FAIL( "Failover aborted" );
        }

    private:
        bool attempted_;
        bool reconnected_;
    } myCallback;

    sql.set_failover_callback(myCallback);

    auto_table_creator tableCreator(tc_.table_creator_1(sql));

    int id = 17;
    sql << "insert into soci_test (id) values (:id)", use(id);
    REQUIRE_NOTHROW( sql.commit() );

    std::cout << "Please break connection to the database "
                 "(stop the server, unplug the network cable, ...) "
                 "and press Enter" << std::endl;
    std::cin.get();

    int id2;
    sql << "select id from soci_test", into(id2);
    CHECK( id2 == id );

    CHECK( myCallback.did_reconnect() );
}

// This pseudo-test allows to execute an arbitrary SQL query by defining
// SOCI_TEST_SQL environment variable and examine the resulting error.
TEST_CASE_METHOD(common_tests, "Execute", "[.]")
{
    auto const text = std::getenv("SOCI_TEST_SQL");
    if (!text)
    {
        FAIL( "SOCI_TEST_SQL environment variable must be set." );
    }

    soci::session sql(backEndFactory_, connectString_);
    try
    {
        sql << text;

        WARN("Statement executed successfully.");
    }
    catch (soci_error const& e)
    {
        char const* const categories[] =
        {
            "connection_error",
            "invalid_statement",
            "no_privilege",
            "no_data",
            "constraint_violation",
            "unknown_transaction_state",
            "system_error",
            "unknown"
        };

        unsigned const cat = e.get_error_category();
        REQUIRE(cat < sizeof(categories) / sizeof(categories[0]));

        WARN("Statement execution failed: " << e.what() << "\n"
             "Error category: " << categories[cat] << "\n");

        auto const& backend = e.get_backend_name();
        if ( !backend.empty() )
        {
            if ( e.get_backend_error_code() )
                WARN(backend << " error " << e.get_backend_error_code() << "\n");
            if ( !e.get_sqlstate().empty() )
                WARN(backend << " SQL state " << e.get_sqlstate() << "\n");
        }
    }
}

} // namespace tests

} // namespace soci
