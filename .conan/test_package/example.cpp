//#include "soci/soci.h"
//#include "soci/sqlite3/soci-sqlite3.h"

#include <exception>
#include <iostream>
#include <ostream>
#include <string>

//
//using namespace soci;
//using namespace std;
//
//bool get_name(string &name) {
//  cout << "Enter name: ";
//  cin >> name;
//  if (name != "Q")
//  {
//    return true;
//  }
//  return false;
//}

int main()
{
  std::cout << "Hola SOCI\n";
  try
  {
//    soci::session sql(*soci::factory_sqlite3(), "service=mydb user=john password=secret");
//
//    int count;
//    sql << "select count(*) from phonebook", into(count);
//
//    cout << "We have " << count << " entries in the phonebook.\n";
//
//    string name;
//    while (get_name(name))
//    {
//      string phone;
//      indicator ind;
//      sql << "select phone from phonebook where name = :name",
//          into(phone, ind), use(name);
//
//      if (ind == i_ok)
//      {
//        cout << "The phone number is " << phone << '\n';
//      }
//      else
//      {
//        cout << "There is no phone for " << name << '\n';
//      }
//    }
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
  }

  return 0;
}
