#include "catch2/catch.hpp"
#include "fmt/format.h"
#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"

namespace test_soci_sqlite
{

TEST_CASE("should successfully connect using sqlite3 backend")
{
  const auto& connectString{"../database2.sqlite3"};
  const auto &table{"table1"};
  const soci::backend_factory& backEnd = *soci::factory_sqlite3();
  soci::session sql(backEnd, connectString);
  fmt::print("soci database connected successfully with empty backend\n");
  CHECK( sql.is_connected() );
}

} // test_soci_sqlite
