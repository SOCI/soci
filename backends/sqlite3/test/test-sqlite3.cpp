//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-sqlite3.h"
#include "common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cmath>
#include <ctime>

using namespace soci;
using namespace soci::tests;

std::string connectString;
backend_factory const &backEnd = sqlite3;

// ROWID test
// In sqlite3 the row id can be called ROWID, _ROWID_ or oid
void test1()
{
    {
        session sql(backEnd, connectString);

        try { sql << "drop table test1"; }
        catch (soci_error const &) {} // ignore if error

        sql <<
        "create table test1 ("
        "    id integer,"
        "    name varchar(100)"
        ")";

        sql << "insert into test1(id, name) values(7, \'John\')";

        rowid rid(sql);
        sql << "select oid from test1 where id = 7", into(rid);

        int id;
        std::string name;

        sql << "select id, name from test1 where oid = :rid",
        into(id), into(name), use(rid);

        assert(id == 7);
        assert(name == "John");

        sql << "drop table test1";
    }

    std::cout << "test 1 passed" << std::endl;
}

// BLOB test
struct blob_table_creator : public table_creator_base
{
    blob_table_creator(session& session)
    : table_creator_base(session)
    {
        session <<
             "create table soci_test ("
             "    id integer,"
             "    img blob"
             ")";
    }
};

void test2()
{
    {
        session sql(backEnd, connectString);

        blob_table_creator tableCreator(sql);

        char buf[] = "abcdefghijklmnopqrstuvwxyz";

        sql << "insert into soci_test(id, img) values(7, '')";

        {
            blob b(sql);

            sql << "select img from soci_test where id = 7", into(b);
            assert(b.get_len() == 0);

            b.write(0, buf, sizeof(buf));
            assert(b.get_len() == sizeof(buf));
            sql << "update soci_test set img=? where id = 7", use(b);

            b.append(buf, sizeof(buf));
            assert(b.get_len() == 2 * sizeof(buf));
            sql << "insert into soci_test(id, img) values(8, ?)", use(b);
        }
        {
            blob b(sql);
            sql << "select img from soci_test where id = 8", into(b);
            assert(b.get_len() == 2 * sizeof(buf));
            char buf2[100];
            b.read(0, buf2, 10);
            assert(strncmp(buf2, "abcdefghij", 10) == 0);

            sql << "select img from soci_test where id = 7", into(b);
            assert(b.get_len() == sizeof(buf));

        }
    }

    std::cout << "test 2 passed" << std::endl;
}

// This test was put in to fix a problem that occurs when there are both
//into and use elements in the same query and one of them (into) binds
//to a vector object.
void test3()
{
    {
        session sql(backEnd, connectString);

        try { sql << "drop table test3"; }
        catch (soci_error const &) {} //ignore error if table doesn't exist

        sql << "Create table test3( id integer, name varchar, subname varchar);";

        sql << "Insert into test3(id,name,subname) values( 1,'john','smith')";
        sql << "Insert into test3(id,name,subname) values( 2,'george','vals')";
        sql << "Insert into test3(id,name,subname) values( 3,'ann','smith')";
        sql << "Insert into test3(id,name,subname) values( 4,'john','grey')";
        sql << "Insert into test3(id,name,subname) values( 5,'anthony','wall')";

        std::vector<int> v(10);

        statement s(sql.prepare << "Select id from test3 where name = :name");

        std::string name = "john";

        s.exchange(use(name, "name"));
        s.exchange(into(v));

        s.define_and_bind();
        s.execute(true);

        assert(v.size() == 2);
    }
    std::cout << "test 3 passed" << std::endl;
}


// 
// Test case from Amnon David 11/1/2007
// I've noticed that table schemas in SQLite3 can sometimes have typeless 
// columns. One (and only?) example is the sqlite_sequence that sqlite 
// creates for autoincrement . Attempting to traverse this table caused 
// SOCI to crash. I've made the following code change in statement.cpp to 
// create a workaround:
void test4()
{
    {
        // we need to have an table that uses autoincrement to test this.
        session sql(backEnd, connectString);
        sql << "create table nulltest (col INTEGER PRIMARY KEY AUTOINCREMENT, name char)";
        sql << "insert into nulltest(name) values('john')";
        sql << "insert into nulltest(name) values('james')";

        int key;
        std::string name;
        sql << "select * from nulltest", into(key), into(name);
        assert(name == "john");

        rowset<row> rs = (sql.prepare << "select * from sqlite_sequence");
        rowset<row>::const_iterator it = rs.begin();
        row const& r1 = (*it);
        assert(r1.get<std::string>(0) == "nulltest");
        assert(r1.get<std::string>(1) == "2");
    }
    std::cout << "test 4 passed" << std::endl;
}

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
        return "datetime(\'" + dateString + "\')";
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
        // If no file name is specfied then work in-memory
        connectString = ":memory:";
    }

    try
    {

        TestContext tc(backEnd, connectString);
        common_tests tests(tc);
        tests.run(false);

        std::cout << "\nSOCI sqlite3 Tests:\n\n";

        test1();
        test2();
        test3();
        test4();

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (soci::soci_error const & e)
    {
        std::cout << "SOCIERROR: " << e.what() << '\n';
    }
    catch (std::exception const & e)
    {
        std::cout << "EXCEPTION: " << e.what() << '\n';
    }
}
