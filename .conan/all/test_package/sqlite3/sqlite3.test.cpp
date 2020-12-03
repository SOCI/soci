#include "catch2/catch.hpp"
#include "fmt/format.h"
#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "sqlite3.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <string>

const auto& connectString{"../database1.sqlite3"};
const auto& table{"table1"};
const soci::backend_factory& backEnd = *soci::factory_sqlite3();
soci::session sql(backEnd, connectString);

bool tableExists(const std::string& tableName)
{
  bool exists = false;
  try
  {
    sql << fmt::format(R"(SELECT count(*) FROM {0} ;)", tableName);

    exists = true;
  } catch (const std::exception& ex)
  {
  }

  return exists;
}

void createTable(const std::string& tableName)
{
  if (!tableExists(tableName))
  {
    sql << fmt::format(R"(
CREATE TABLE {0} (
    id INTEGER,
    name VARCHAR2(100)
);)", tableName);
  }
}

void insertInto(const std::string& tableName)
{
  if (tableExists(tableName))
  {
    sql << fmt::format(R"EOL(INSERT INTO {0} (id, name)
VALUES
       (7, 'John'),
       (9, 'Jane');
)EOL", tableName);
  }
}

int getNumberOfRows(const std::string& tableName)
{
  int count = 0;

  if (tableExists(tableName))
  {
    sql << fmt::format(R"(SELECT count(*) FROM {0};)", tableName),
        soci::into(count);
  }

  return count;
}

bool tableDropped(const std::string& tableName)
{
  bool result = false;

  if (tableExists(tableName))
  {
    sql << fmt::format("DROP TABLE {0}", tableName);
    result = true;
  }

  return result;
}

std::pair<int, std::string> getValues(const std::string& tableName, int idToFind)
{
  int id = 0;
  std::string name{};

  sql << fmt::format("SELECT id, name FROM {0} WHERE id = {1}", tableName, idToFind),
      soci::into(id),
      soci::into(name);

  return {id, name};
}

TEST_CASE("should be connected")
{
  CHECK( sql.is_connected() );
}

TEST_CASE("should try to drop a nonexistent table")
{
  tableDropped(table);
  CHECK_FALSE( tableDropped(table) );
}

TEST_CASE("should create a table")
{
  createTable(table);
  CHECK( tableExists(table) );
}

TEST_CASE("should insert two rows into table")
{
  insertInto(table);
  CHECK( getNumberOfRows(table) == 2 );
}

TEST_CASE("should get a row from a given value")
{
  const int idToFind = 9;
  const auto [id, name] = getValues(table, idToFind);

  CHECK( id == 9);
  CHECK( name == "Jane");
}

TEST_CASE("should drop a table")
{
  CHECK( tableDropped(table) );
}
