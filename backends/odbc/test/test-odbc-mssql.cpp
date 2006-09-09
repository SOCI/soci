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

using namespace SOCI;
using namespace SOCI::tests;

std::string connectString;
BackEndFactory const &backEnd = odbc;


// DDL Creation objects for common tests
struct TableCreator1 : public TableCreatorBase
{
    TableCreator1(Session& session)
        : TableCreatorBase(session) 
    {
        session << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh smallint, ul numeric(20), d float, "
                 "tm datetime, i1 integer, i2 integer, i3 integer, " 
                 "name varchar(20))";
    }
};

struct TableCreator2 : public TableCreatorBase
{
    TableCreator2(Session& session)
        : TableCreatorBase(session)
    {
        session  << "create table soci_test(num_float float, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct TableCreator3 : public TableCreatorBase
{
    TableCreator3(Session& session)
        : TableCreatorBase(session)
    {
        session << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

// SQL Abstraction functions required for common tests
struct SqlTranslator
{
    static std::string fromDual(std::string const &sql)
    {
        return sql;
    }

    static std::string toDate(std::string const &dateString)
    {
        return "convert(datetime, \'" + dateString + "\', 120)";
    }

    static std::string toDateTime(std::string const &dateString)
    {
        return "convert(datetime, \'" + dateString + "\', 120)";
    }
};


int main(int argc, char** argv)
{
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
        CommonTests<TableCreator1, 
                    TableCreator2, 
                    TableCreator3,
                    SqlTranslator> tests(backEnd, connectString);
        tests.run();
    }
    catch (SOCI::ODBCSOCIError const & e)
    {
        std::cout << "ODBC Error Code: " << e.odbcErrorCode() << std::endl
                  << "Native Error Code: " << e.nativeErrorCode() << std::endl
                  << "SOCI Message: " << e.what() << std::endl
                  << "ODBC Message: " << e.odbcErrorMessage() << std::endl;
    }
    catch (std::exception const & e)
    {
        std::cout << "STD::EXECEPTION " << e.what() << '\n';
    }
}
