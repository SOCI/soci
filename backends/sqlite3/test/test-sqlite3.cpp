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

using namespace SOCI;
using namespace SOCI::tests;

std::string connectString;
BackEndFactory const &backEnd = sqlite3;

// ROWID test
// In sqlite3 the row id can be called ROWID, _ROWID_ or oid
void test1()
{
    {
        Session sql(backEnd, connectString);

        try { sql << "drop table test1"; }
        catch (SOCIError const &) {} // ignore if error

        sql <<
        "create table test1 ("
        "    id integer,"
        "    name varchar(100)"
        ")";

        sql << "insert into test1(id, name) values(7, \'John\')";

        RowID rid(sql);
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

// Blob Test
void test2()
{
    {
        Session sql(backEnd, connectString);
#ifdef SQLITE_ENABLE_COLUMN_METADATA

        try { sql << "drop table test2"; }
        catch (SOCIError const &) {} // ignore if error

        sql <<
            "create table test2 ("
             "    id integer,"
             "    img blob"
             ")";

        char buf[] = "abcdefghijklmnopqrstuvwxyz";

        sql << "insert into test2(id, img) values(7, '')";

        {
            BLOB b(sql);

            sql << "select img from test2 where id = 7", into(b);
            assert(b.getLen() == 0);

            b.write(0, buf, sizeof(buf));
            assert(b.getLen() == sizeof(buf));

            b.append(buf, sizeof(buf));
            assert(b.getLen() == 2 * sizeof(buf));
        }
        {
            BLOB b(sql);
            sql << "select img from test2 where id = 7", into(b);
            assert(b.getLen() == 2 * sizeof(buf));
            char buf2[100];
            b.read(0, buf2, 10);
            assert(strncmp(buf2, "abcdefghij", 10) == 0);

            // make sure that we can't read past the end of the data
            int len = b.read(50, buf2, 10);
            assert(len == 4);
            assert(strncmp(buf2, "xyz", 10) == 0);
        }
        {
            BLOB b(sql);
            sql << "select img from test2 where id = 7", into(b);
            assert(b.getLen() == 2 * sizeof(buf));
            b.trim(sizeof(buf));

            BLOB b2(sql);
            sql << "select img from test2 where id = 7", into(b2);
            assert(b2.getLen() == sizeof(buf));            
        }

        sql << "drop table test2";

#else
        try
        {
            // expected error
            BLOB b(sql);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string msg = e.what();
            assert(msg ==
                "Blob not currently supported in this Sqlite3 backend. "
                "Compile both Sqlite3 and SOCI using "
                "-DSQLITE_ENABLE_COLUMN_METADATA");
            std::cout << msg << std::endl;
        }
#endif
        
    }
    
    std::cout << "test 2 passed" << std::endl;
}


// This test was put in to fix a problem that occurs when there are both 
//into and use elements in the same query and one of them (into) binds 
//to a vector object.
void test3()
{
    {
        Session sql(backEnd, connectString);

        try { sql << "drop table test3"; }
        catch (SOCIError const &) {} //ignore error if table doesn't exist

        sql << "Create table test3( id integer, name varchar, subname varchar);";

        sql << "Insert into test3(id,name,subname) values( 1,'john','smith')";
        sql << "Insert into test3(id,name,subname) values( 2,'george','vals')";
        sql << "Insert into test3(id,name,subname) values( 3,'ann','smith')";
        sql << "Insert into test3(id,name,subname) values( 4,'john','grey')";
        sql << "Insert into test3(id,name,subname) values( 5,'anthony','wall')";

        std::vector<int> v(10);

        Statement s(sql.prepare << "Select id from test3 where name = :name");

        std::string name = "john";

        s.exchange(use(name, "name"));
        s.exchange(into(v));

        s.defineAndBind();
        s.execute(true);

        assert(v.size() == 2);
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
        return "datetime(\'" + dateString + "\')";
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
        std::cout << "usage: " << argv[0]
                  << " database_file_name\n"       
                  << "example: " << argv[0]
                  << " foo.data\n";
        exit(1);
    }

    try
    {
        TestContext tc(backEnd, connectString);
        CommonTests tests(tc);
        tests.run();
        
        std::cout << "\nSOCI sqlite3 Tests:\n\n";

        test1();
        test2();
        test3();
        
        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (SOCI::SOCIError const & e)
    {
        std::cout << "SOCIERROR: " << e.what() << '\n';
    }
    catch (std::exception const & e)
    {
        std::cout << "EXCEPTION: " << e.what() << '\n';
    }
}
