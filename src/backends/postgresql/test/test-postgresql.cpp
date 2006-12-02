//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-postgresql.h"
#include "common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cmath>
#include <ctime>
#include <cstdlib>

using namespace SOCI;
using namespace SOCI::tests;

std::string connectString;
BackEndFactory const &backEnd = postgresql;

// Postgres-specific tests

struct OidTableCreator : public TableCreatorBase
{
    OidTableCreator(Session& session)
    : TableCreatorBase(session)
    {
        session << "create table soci_test ("
                " id integer,"
                " name varchar(100)"
                ") with oids";
    }
};

// ROWID test
// Note: in PostgreSQL, there is no ROWID, there is OID.
// It is still provided as a separate type for "portability",
// whatever that means.
void test1()
{
    {
        Session sql(backEnd, connectString);

        OidTableCreator tableCreator(sql);

        sql << "insert into soci_test(id, name) values(7, \'John\')";

        RowID rid(sql);
        sql << "select oid from soci_test where id = 7", into(rid);

        int id;
        std::string name;

#ifndef SOCI_PGSQL_NOPARAMS

        sql << "select id, name from soci_test where oid = :rid",
            into(id), into(name), use(rid);

#else
        // Older PostgreSQL does not support use elements.

        PostgreSQLRowIDBackEnd *rbe
            = static_cast<PostgreSQLRowIDBackEnd *>(rid.getBackEnd());

        unsigned long oid = rbe->value_;

        sql << "select id, name from soci_test where oid = " << oid,
            into(id), into(name);

#endif // SOCI_PGSQL_NOPARAMS

        assert(id == 7);
        assert(name == "John");
    }

    std::cout << "test 1 passed" << std::endl;
}

// function call test
class FunctionCreator : FunctionCreatorBase 
{
public:

    FunctionCreator(Session& session)
    : FunctionCreatorBase(session)
    {
        // before a language can be used it must be defined
        // if it has already been defined then an error will occur
        try { session << "create language plpgsql"; }
        catch (SOCIError const &) {} // ignore if error

#ifndef SOCI_PGSQL_NOPARAMS

        session  <<
            "create or replace function soci_test(msg varchar) "
            "returns varchar as $$ "
            "begin "
            "  return msg; "
            "end $$ language plpgsql";
#else

       session <<
            "create or replace function soci_test(varchar) "
            "returns varchar as \' "
            "begin "
            "  return $1; "
            "end \' language plpgsql";
#endif 
    }

protected:

    std::string dropStatement()
    {
        return "drop function soci_test(varchar)";
    }
};

void test2()
{
    {
        Session sql(backEnd, connectString);

        FunctionCreator functionCreator(sql);

        std::string in("my message");
        std::string out;

#ifndef SOCI_PGSQL_NOPARAMS

        Statement st = (sql.prepare <<
            "select soci_test(:input)",
            into(out),
            use(in, "input"));

#else
        // Older PostgreSQL does not support use elements.

        std::string in("my message");
        std::string out;
        Statement st = (sql.prepare <<
            "select soci_test(\'" << in << "\')",
            into(out));

#endif // SOCI_PGSQL_NOPARAMS

        st.execute(true);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;

#ifndef SOCI_PGSQL_NOPARAMS

            Procedure proc = (sql.prepare <<
                "soci_test(:input)",
                into(out), use(in, "input"));

#else
        // Older PostgreSQL does not support use elements.

            Procedure proc = (sql.prepare <<
                "soci_test(\'" << in << "\')", into(out));

#endif // SOCI_PGSQL_NOPARAMS

            proc.execute(true);
            assert(out == in);
        }
    }

    std::cout << "test 2 passed" << std::endl;
}

// BLOB test
struct BlobTableCreator : public TableCreatorBase
{
    BlobTableCreator(Session& session)
    : TableCreatorBase(session)
    {
        session <<
             "create table soci_test ("
             "    id integer,"
             "    img oid"
             ")";
    }
};

void test3()
{
    {
        Session sql(backEnd, connectString);

        BlobTableCreator tableCreator(sql);
        
        char buf[] = "abcdefghijklmnopqrstuvwxyz";

        sql << "insert into soci_test(id, img) values(7, lo_creat(-1))";

        // in PostgreSQL, BLOB operations must be withing transaction block
        sql.begin();

        {
            BLOB b(sql);

            sql << "select img from soci_test where id = 7", into(b);
            assert(b.getLen() == 0);

            b.write(0, buf, sizeof(buf));
            assert(b.getLen() == sizeof(buf));

            b.append(buf, sizeof(buf));
            assert(b.getLen() == 2 * sizeof(buf));
        }
        {
            BLOB b(sql);
            sql << "select img from soci_test where id = 7", into(b);
            assert(b.getLen() == 2 * sizeof(buf));
            char buf2[100];
            b.read(0, buf2, 10);
            assert(strncmp(buf2, "abcdefghij", 10) == 0);
        }

        unsigned long oid;
        sql << "select img from soci_test where id = 7", into(oid);
        sql << "select lo_unlink(" << oid << ")";

        sql.commit();
    }

    std::cout << "test 3 passed" << std::endl;
}

// DDL Creation objects for common tests
struct TableCreator1 : public TableCreatorBase
{
    TableCreator1(Session& session)
        : TableCreatorBase(session) 
    {
        session << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "tm timestamp, i1 integer, i2 integer, i3 integer, " 
                 "name varchar(20))";
    }
};

struct TableCreator2 : public TableCreatorBase
{
    TableCreator2(Session& session)
        : TableCreatorBase(session)
    {
        session  << "create table soci_test(num_float float8, num_int integer,"
                     " name varchar(20), sometime timestamp, chr char)";
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

//
// Support for SOCI Common Tests
//

class TestContext : public TestContextBase
{
public:
    TestContext(BackEndFactory const &backEnd, 
                std::string const &connectString)
        : TestContextBase(backEnd, connectString) {}

    TableCreatorBase* tableCreator1(Session& s) const
    {
        return new TableCreator1(s);
    }

    TableCreatorBase* tableCreator2(Session& s) const
    {
        return new TableCreator2(s);
    }

    TableCreatorBase* tableCreator3(Session& s) const
    {
        return new TableCreator3(s);
    }

    std::string toDateTime(std::string const &dateString) const
    {
        return "timestamptz(\'" + dateString + "\')";
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
        std::cout << "usage: " << argv[0]
            << " connectstring\n"
            << "example: " << argv[0]
            << " \'connect_string_for_PostgreSQL\'\n";
        return EXIT_FAILURE;
    }

    try
    {
        TestContext tc(backEnd, connectString);
        CommonTests tests(tc);
        tests.run();

        std::cout << "\nSOCI Postgres Tests:\n\n";

        test1();
        test2();
        test3();

        std::cout << "\nOK, all tests passed.\n\n";
        return EXIT_SUCCESS;
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
