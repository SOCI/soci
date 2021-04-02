//
// Copyright (C) 2021 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//

// This is the simplest possible example of using SOCI: just connect to the
// database using the provided command line argument.

#include <soci/soci.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <connection-string>\n";
        return EXIT_FAILURE;
    }

    char const* const connectString = argv[1];

    try
    {
        soci::session sql(connectString);

        std::cout << "Successfully connected to \"" << connectString << "\", "
                  << "using \"" << sql.get_backend_name() << "\" backend.\n";

        return EXIT_SUCCESS;
    }
    catch (soci::soci_error const& e)
    {
        std::cerr << "Connection to \"" << connectString << "\" failed: "
                  << e.what() << "\n";
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << "Unexpected standard exception occurred: "
                  << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unexpected unknown exception occurred.\n";
    }

    return EXIT_FAILURE;
}
