#include <soci/soci.h>
#include <soci/empty/soci-empty.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <connection-string>\n";
        return EXIT_FAILURE;
    }

    std::string connectString{argv[1]};

    try
    {
        soci::session sql(soci::empty, connectString);

        std::cout << "Successfully connected to \"" << connectString << "\", "
                  << "using \"" << sql.get_backend_name() << "\" backend.\n";

        return EXIT_SUCCESS;
    }
    catch (const soci::soci_error& e)
    {
        std::cerr << "Connection to \"" << connectString << "\" failed: "
                  << e.what() << "\n";
    }
    catch (const std::runtime_error& e)
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
