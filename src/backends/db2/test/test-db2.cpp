//
// Copyright (C) 2011-2013 Denis Chapligin
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-db2.h"
#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>
#include <ctime>

using namespace soci;

std::string connectString;
backend_factory const &backEnd = *soci::factory_db2();


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

void test1()
{
    {
        session sql(backEnd, connectString);

        sql << "SELECT CURRENT TIMESTAMP FROM SYSIBM.SYSDUMMY1";
        sql << "SELECT " << 123 << " FROM SYSIBM.SYSDUMMY1";

        std::string query = "CREATE TABLE DB2INST1.TEST (ID BIGINT,DATA VARCHAR(8))";
        sql << query;

        int i = 7;
        sql << "insert into db2inst1.TEST (id) values (:id)", use(i,"id");
        sql << "select id from db2inst1.TEST where id=7", into(i);

        
#if defined (__LP64__) || ( __WORDSIZE == 64 )
        long int li = 9;
        sql << "insert into db2inst1.TEST (id) values (:id)", use(li,"id");
        sql << "select id from db2inst1.TEST where id=9", into(li);
#endif

        long long ll = 11;
        sql << "insert into db2inst1.TEST (id) values (:id)", use(ll,"id");
        sql << "select id from db2inst1.TEST where id=11", into(ll);

        indicator ind = i_ok;
        sql << "insert into db2inst1.TEST (id) values (:id)", use(i,ind,"id");
        sql << "select id from db2inst1.TEST where id=7", into(i,ind);

        std::vector<int> numbers(100);
        sql << "insert into db2inst1.TEST (id) values (:id)", use(numbers,"id");
        sql << "select id from db2inst1.TEST", into(numbers);

        std::vector<indicator> inds(100);
        sql << "insert into db2inst1.TEST (id) values (:id)", use(numbers,inds,"id");
        sql << "select id from db2inst1.TEST", into(numbers,inds);

        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", into(i));
            st.execute();
            st.fetch();
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", into(i, ind));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", into(numbers));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", into(numbers, inds));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", use(i));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", use(i, ind));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", use(numbers));
        }
        {
            statement st = (sql.prepare << "select id from db2inst1.TEST", use(numbers, inds));
        }

        sql<<"DROP TABLE DB2INST1.TEST";

        sql.commit();
    }

    std::cout << "test 1 passed" << std::endl;
}

void test2() {
    {
        session sql(backEnd, connectString);

        std::string query = "CREATE TABLE DB2INST1.TEST (ID BIGINT,DATA VARCHAR(8),DT TIMESTAMP)";
        sql << query;

        int i = 7;
        std::string n("test");
        sql << "insert into db2inst1.TEST (id,data) values (:id,:name)", use(i,"id"),use(n,"name");
        sql << "select id,data from db2inst1.TEST where id=7", into(i),into(n);

        i = 8;
        indicator ind = i_ok;
        sql << "insert into db2inst1.TEST (id) values (:id)", use(i,"id");
        sql << "select id,data from db2inst1.TEST where id=8", into(i),into(n,ind);
        assert(ind==i_null);

        std::tm dt;
        sql << "select current timestamp from sysibm.sysdummy1",into(dt);
        sql << "insert into db2inst1.TEST (dt) values (:dt)",use(dt,"dt");

        sql<<"DROP TABLE DB2INST1.TEST";
        sql.commit();
    }

    std::cout << "test 2 passed" << std::endl;
}

void test3() {
    {
        session sql(backEnd, connectString);

        std::string query = "CREATE TABLE DB2INST1.TEST (ID BIGINT,DATA VARCHAR(8),DT TIMESTAMP)";
        sql << query;

        std::vector<std::string> strings(100);
        for(std::vector<std::string>::iterator it=strings.begin();it!=strings.end();it++) {
    	    *it="test";
        }
        sql << "insert into db2inst1.TEST (data) values (:data)", use(strings,"data");
        rowset<std::string> rs = (sql.prepare<<"SELECT data from db2inst1.TEST");

        sql<<"DROP TABLE DB2INST1.TEST";
        sql.commit();
    }

    std::cout << "test 3 passed" << std::endl;
}

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
            << " \'DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off\'\n";
        std::exit(1);
    }

    try
    {
        test1();
        test2();
        test3();
        // ...

        std::cout << "\nOK, all tests passed.\n\n";

        return EXIT_SUCCESS;
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }

    return EXIT_FAILURE;
}
