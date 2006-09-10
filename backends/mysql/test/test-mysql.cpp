#include "soci.h"
#include "soci-mysql.h"
#include "test/common-tests.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cmath>
#include <ctime>
#include <ciso646>

using namespace SOCI;
using namespace SOCI::tests;

std::string connectString;
BackEndFactory const &backEnd = mysql;


// procedure call test
void test1()
{
    {
        Session sql(backEnd, connectString);
	
        MySQLSessionBackEnd *sessionBackEnd
            = static_cast<MySQLSessionBackEnd *>(sql.getBackEnd());
        std::string version = mysql_get_server_info(sessionBackEnd->conn_);
        int v;
        std::istringstream iss(version);
        if ((iss >> v) and v < 5)
        {
            std::cout << "skipping test 1 (MySQL server version ";
            std::cout << version << " does not support stored procedures)\n";
            return;
        }
	
        try { sql << "drop function myecho"; }
        catch (SOCIError const &) {}
	
        sql <<
            "create function myecho(msg text) "
            "returns text "
            "  return msg; ";
 
        std::string in("my message");
        std::string out;

        Statement st = (sql.prepare <<
            "select myecho(:input)",
            into(out),
            use(in, "input"));

        st.execute(1);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;

            Procedure proc = (sql.prepare <<
                "myecho(:input)",
                into(out), use(in, "input"));

            proc.execute(1);
            assert(out == in);
        }

        sql << "drop function myecho";
    }

    std::cout << "test 12 passed" << std::endl;
}

// DDL Creation objects for common tests
struct TableCreator1 : public TableCreatorBase
{
    TableCreator1(Session& session)
        : TableCreatorBase(session) 
    {
        session << "create table soci_test(id integer, val integer, c char, "
                 "str varchar(20), sh int2, ul numeric(20), d float8, "
                 "tm datetime, i1 integer, i2 integer, i3 integer, " 
                 "name varchar(20))";
    }
};

struct TableCreator2 : public TableCreatorBase
{
    TableCreator2(Session& session)
        : TableCreatorBase(session)
    {
        session  << "create table soci_test(num_float float8, num_int integer,"
                     " name varchar(20), sometime datetime, chr char)";
    }
};

struct TableCreator3 : public TableCreatorBase
{
    TableCreator3(Session& session)
        : TableCreatorBase(session)
    {
        session << "create table soci_test(name varchar(100) not null, "
            "phone varchar(15))";
    }
};

//
// Support for SOCI Common Tests
//

class TestContext : public TestContextBase
{
public:
    TestContext(BackEndFactory const &backEnd, 
                std::string const &connectString)
        : TestContextBase(backEnd, connectString) {}

    TableCreatorBase* tableCreator1(Session& s) const
    {
        return new TableCreator1(s);
    }

    TableCreatorBase* tableCreator2(Session& s) const
    {
        return new TableCreator2(s);
    }

    TableCreatorBase* tableCreator3(Session& s) const
    {
        return new TableCreator3(s);
    }

    std::string toDateTime(std::string const &dateString) const
    {
        return "\'" + dateString + "\'";
    }

};

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
            << "example: " << argv[0]
            << " \"dbname=test user=root password=\'Ala ma kota\'\"\n";
        exit(1);
    }

    try
    {
        TestContext tc(backEnd, connectString);
        CommonTests tests(tc);
        tests.run();

        std::cout << "\nSOCI MySQL Tests:\n\n";

        test1();

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
