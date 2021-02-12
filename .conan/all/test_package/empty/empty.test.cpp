#include "catch2/catch.hpp"
#include "fmt/format.h"
#include "soci/soci.h"
#include "soci/empty/soci-empty.h"

namespace test_soci_empty
{

TEST_CASE("should successfully connect with empty backend")
{
  const auto &connectString{"../database1.empty.db"};
  const auto &table{"table1"};
  const soci::backend_factory& backEnd = *soci::factory_empty();
  soci::session sql(backEnd, connectString);
  fmt::print("soci database connected successfully with empty backend\n");
  CHECK( sql.is_connected() );
}

} // test_soci_empty
