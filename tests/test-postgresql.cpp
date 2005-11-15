#include "soci.h"
#include "soci-postgresql.h"
#include <iostream>
#include <string>
#include <cassert>
#include <cmath>
#include <ctime>

using namespace SOCI;

std::string connectString;
std::string backEndName = "postgresql";

// fundamental tests
void test1()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test1"; }
        catch (SOCIError const &) {} // ignore if error

        sql << "create table test1 (id integer)";
        sql << "drop table test1";

        try
        {
            // expected error
            sql << "drop table test1_nosuchtable";
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string msg = e.what();
            assert(msg ==
                "ERROR:  table \"test1_nosuchtable\" does not exist\n");
        }

        sql << "create table test1 (id integer)";
        sql << "insert into test1 (id) values (" << 123 << ")";
        sql << "delete from test1";

        sql << "drop table test1";
    }

    std::cout << "test 1 passed" << std::endl;
}

// "into" tests, type conversions, etc.
void test2()
{
    {
        Session sql(backEndName, connectString);

        char c('a');
        sql << "select \'c\'", into(c);
        assert(c == 'c');

        char buf[4];
        sql << "select \'ABC\'", into(buf);
        assert(buf[0] == 'A');
        assert(buf[1] == 'B');
        assert(buf[2] == 'C');
        assert(buf[3] == '\0');
        sql << "select \'Hello\'", into(buf);
        assert(buf[0] == 'H');
        assert(buf[1] == 'e');
        assert(buf[2] == 'l');
        assert(buf[3] == '\0');

        std::string str;
        sql << "select \'Hello, PostgreSQL!\'", into(str);
        assert(str == "Hello, PostgreSQL!");

        short sh(0);
        sql << "select 3", into(sh);
        assert(sh == 3);

        int i(0);
        sql << "select 5", into(i);
        assert(i == 5);

        unsigned long ul(0);
        sql << "select 7", into(ul);
        assert(ul == 7);

        double d(0.0);
        sql << "select 3.14159265", into(d);
        assert(std::abs(d - 3.14159265) < 0.001);

        std::tm t;
        sql << "select date(\'2005-11-15\')", into(t);
        assert(t.tm_year == 105);
        assert(t.tm_mon  == 10);
        assert(t.tm_mday == 15);
        assert(t.tm_hour == 0);
        assert(t.tm_min  == 0);
        assert(t.tm_sec  == 0);

        sql << "select timestamptz(\'2005-11-15 22:14:17\')", into(t);
        assert(t.tm_year == 105);
        assert(t.tm_mon  == 10);
        assert(t.tm_mday == 15);
        assert(t.tm_hour == 22);
        assert(t.tm_min  == 14);
        assert(t.tm_sec  == 17);

        // test indicators
        eIndicator ind;
        sql << "select 2", into(i, ind);
        assert(ind == eOK);
        sql << "select NULL", into(i, ind);
        assert(ind == eNull);
        sql << "select \'Hello\'", into(buf, ind);
        assert(ind == eTruncated);

        try
        {
            // expect error
            sql << "select NULL", into(i);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string error = e.what();
            assert(error ==
                "Null value fetched and no indicator defined.");
        }

        sql << "select 5 where 0 = 1", into(i, ind);
        assert(ind == eNoData);

        try
        {
            // expect error
            sql << "select 5 where 0 = 1", into(i);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string error = e.what();
            assert(error ==
                "No data fetched and no indicator defined.");
        }
    }

    std::cout << "test 2 passed" << std::endl;
}


int main(int argc, char** argv)
{
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
        exit(1);
    }

    try
    {
        test1();
        test2();

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
