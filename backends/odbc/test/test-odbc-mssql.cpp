//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-odbc.h"
#include "common-tests.h"
#include <iostream>
#include <string>
#include <cassert>
#include <ctime>
#include <cmath>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = odbc;


// DDL Creation objects for common tests
struct TableCreator1 : public table_creator_base
{
    TableCreator1(session& session)
        : table_creator_base(session) 
    {
        session << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ul numeric(20), d float, "
                 "tm datetime, i1 integer, i2 integer, i3 integer, " 
                 "name varchar(20))";
    }
};

struct TableCreator2 : public table_creator_base
{
    TableCreator2(session& session)
        : table_creator_base(session)
    {
        session  << "create table soci_test(num_float float, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct TableCreator3 : public table_creator_base
{
    TableCreator3(session& session)
        : table_creator_base(session)
    {
        session << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

//
// Support for SOCI Common Tests
//

class TestContext : public test_context_base
{
public:
    TestContext(backend_factory const &backEnd, 
                std::string const &connectString)
        : test_context_base(backEnd, connectString) {}

    table_creator_base* table_creator_1(session& s) const
    {
        return new TableCreator1(s);
    }

    table_creator_base* table_creator_2(session& s) const
    {
        return new TableCreator2(s);
    }

    table_creator_base* table_creator_3(session& s) const
    {
        return new TableCreator3(s);
    }

    std::string to_date_time(std::string const &dateString) const
    {
        return "convert(datetime, \'" + dateString + "\', 120)";
    }
};

int main(int argc, char** argv)
{

#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run asser()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc == 2)
    {
        connectString = argv[1];
    }
    else
    {
        connectString = "FILEDSN=./test-mssql.dsn";
    }
    try
    {
        TestContext tc(backEnd, connectString);
        common_tests tests(tc);
        tests.run();
        std::cout << "\nOK, all tests passed.\n";
    }
    catch (soci::odbc_soci_error const & e)
    {
        std::cout << "ODBC Error Code: " << e.odbc_error_code() << std::endl
                  << "Native Error Code: " << e.native_error_code() << std::endl
                  << "SOCI Message: " << e.what() << std::endl
                  << "ODBC Message: " << e.odbc_error_message() << std::endl;
    }
    catch (std::exception const & e)
    {
        std::cout << "STD::EXECEPTION " << e.what() << '\n';
    }
}
