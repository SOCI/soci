#include "catch2/catch.hpp"
#include "soci/soci.h"
#include <iostream>

namespace soci_none
{

TEST_CASE("should include soci successfully")
{
  std::cout << "soci included successfully\n";
}

} // soci_none
