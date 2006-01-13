#include "soci.h"
#include "soci-empty.h"
#include <iostream>
#include <string>
#include <cassert>
#include <ctime>

using namespace SOCI;

std::string connectString;
std::string backEndName = "empty";


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

struct Person
{
    int id;
    std::string firstName;
    std::string lastName;
};

namespace SOCI
{
    template<> struct TypeConversion<Person>
    {
        typedef Row base_type;
        static Person from(Row& /* r */)
        {
            Person p;
            return p;
        }
    };
}

void test1()
{
    {
        Session sql(backEndName, connectString);

        sql << "Do what I want.";
        sql << "Do what I want " << 123 << " times.";

        std::string query = "some query";
        sql << query;

        int i = 7;
        sql << "insert", use(i);
        sql << "select", into(i);

        eIndicator ind = eOK;
        sql << "insert", use(i, ind);
        sql << "select", into(i, ind);

        std::vector<int> numbers(100);
        sql << "insert", use(numbers);
        sql << "select", into(numbers);

        std::vector<eIndicator> inds(100);
        sql << "insert", use(numbers, inds);
        sql << "select", into(numbers, inds);

        {
            Statement st = (sql.prepare << "select", into(i));
            st.execute();
            st.fetch();
        }
        {
            Statement st = (sql.prepare << "select", into(i, ind));
        }
        {
            Statement st = (sql.prepare << "select", into(numbers));
        }
        {
            Statement st = (sql.prepare << "select", into(numbers, inds));
        }
        {
            Statement st = (sql.prepare << "insert", use(i));
        }
        {
            Statement st = (sql.prepare << "insert", use(i, ind));
        }
        {
            Statement st = (sql.prepare << "insert", use(numbers));
        }
        {
            Statement st = (sql.prepare << "insert", use(numbers, inds));
        }
        {
            Person p;
            sql << "select person", into(p);
        }

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
            << " \'connect_string_for_empty_backend\'\n";
        exit(1);
    }

    try
    {
        test1();
        // test2();
        // ...

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
