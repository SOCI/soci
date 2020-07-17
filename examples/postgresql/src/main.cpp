#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>
#include <exception>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

using namespace soci;
using namespace std;

bool get_name(string &name)
{
    cout << "Enter name: ";
    cin >> name;
    return true;
}

int main()
{
    try
    {
        session sql(*soci::factory_postgresql(), "test.db");

        try
        {
            sql << "drop table phonebook";
        }
        catch (soci_error const &)
        {
        }

        sql << "create table phonebook ("
               "    phone varchar(100),"
               "    name varchar(100)"
               ")";

        int count;
        sql << "select count(*) from phonebook", into(count);

        cout << "We have " << count << " entries in the phonebook.\n";

        string name;
        while (get_name(name))
        {
            string phone;
            indicator ind;
            sql << "select phone from phonebook where name = :name", into(phone, ind), use(name);

            if (ind == i_ok)
            {
                cout << "The phone number is " << phone << '\n';
            }
            else
            {
                cout << "There is no phone for " << name << '\n';
            }
        }
    }
    catch (exception const &e)
    {
        cerr << "Error: " << e.what() << '\n';
    }
}