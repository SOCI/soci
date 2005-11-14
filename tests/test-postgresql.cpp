#include "soci.h"
#include "soci-postgresql.h"
#include <iostream>
#include <string>
#include <cassert>
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

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
