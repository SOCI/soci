#include "soci.h"
#include "soci-empty.h"
#include <iostream>
#include <string>
#include <cassert>
#include <ctime>

using namespace SOCI;

std::string connectString;
std::string backEndName = "empty";

// fundamental tests
void test1()
{
    {
        Session session(backEndName, connectString);

        // ...
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
            << "example: " << argv[0] << " \'connect_string_for_empty_backend\'\n";
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
