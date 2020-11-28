#include "soci/soci.h"
#include "soci/sqlite3/soci-sqlite3.h"
#include "sqlite3.h"
#include <exception>
#include <iostream>
#include <ostream>
#include <string>

std::string connectString{};
const soci::backend_factory& backEnd = *soci::factory_sqlite3();

int main()
{
  std::cout << "Hola SOCI\n";

  soci::session sql(backEnd, connectString);

  try {
    sql << "DROP TABLE test1";
  } catch (const soci::soci_error& err) {
    std::cerr << err.what() << '\n'; // do nothing if drop fails, it means the table doesn't exist yet
  }

  sql << R"(
CREATE TABLE test1 (
    id INTEGER,
    name VARCHAR(100)
);
)";

  sql << R"EOL(INSERT INTO test1(id, name) VALUES(7, 'John'))EOL";

  soci::rowid rid(sql);
  sql << "SELECT oid FROM test1 WHERE id = 7", into(rid);

  int id;
  std::string name;

  sql << "SELECT id, name FROM test1 WHERE oid = :rid",
      soci::into(id),
      soci::into(name),
      soci::use(rid);

  std::cout << "id: " << id << '\n';
  std::cout << "name: " << name << '\n';

  sql << "DROP TABLE test1";

  std::cout << "adiós SOCI\n";

  return 0;
}
