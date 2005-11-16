#include "soci.h"
#include "soci-postgresql.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <cmath>
#include <ctime>

using namespace SOCI;

std::string connectString;
std::string backEndName = "postgresql";

// fundamental tests
void test1()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test1"; }
        catch (SOCIError const &) {} // ignore if error

        sql << "create table test1 (id integer)";
        sql << "drop table test1";

        try
        {
            // expected error
            sql << "drop table test1_nosuchtable";
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string msg = e.what();
            assert(msg ==
                "ERROR:  table \"test1_nosuchtable\" does not exist\n");
        }

        sql << "create table test1 (id integer)";
        sql << "insert into test1 (id) values (" << 123 << ")";
        sql << "delete from test1";

        sql << "drop table test1";
    }

    std::cout << "test 1 passed" << std::endl;
}

// "into" tests, type conversions, etc.
void test2()
{
    {
        Session sql(backEndName, connectString);

        char c('a');
        sql << "select \'c\'", into(c);
        assert(c == 'c');

        char buf[4];
        sql << "select \'ABC\'", into(buf);
        assert(buf[0] == 'A');
        assert(buf[1] == 'B');
        assert(buf[2] == 'C');
        assert(buf[3] == '\0');
        sql << "select \'Hello\'", into(buf);
        assert(buf[0] == 'H');
        assert(buf[1] == 'e');
        assert(buf[2] == 'l');
        assert(buf[3] == '\0');

        std::string str;
        sql << "select \'Hello, PostgreSQL!\'", into(str);
        assert(str == "Hello, PostgreSQL!");

        short sh(0);
        sql << "select 3", into(sh);
        assert(sh == 3);

        int i(0);
        sql << "select 5", into(i);
        assert(i == 5);

        unsigned long ul(0);
        sql << "select 7", into(ul);
        assert(ul == 7);

        double d(0.0);
        sql << "select 3.14159265", into(d);
        assert(std::abs(d - 3.14159265) < 0.001);

        std::tm t;
        sql << "select date(\'2005-11-15\')", into(t);
        assert(t.tm_year == 105);
        assert(t.tm_mon  == 10);
        assert(t.tm_mday == 15);
        assert(t.tm_hour == 0);
        assert(t.tm_min  == 0);
        assert(t.tm_sec  == 0);

        sql << "select timestamptz(\'2005-11-15 22:14:17\')", into(t);
        assert(t.tm_year == 105);
        assert(t.tm_mon  == 10);
        assert(t.tm_mday == 15);
        assert(t.tm_hour == 22);
        assert(t.tm_min  == 14);
        assert(t.tm_sec  == 17);

        // test indicators
        eIndicator ind;
        sql << "select 2", into(i, ind);
        assert(ind == eOK);
        sql << "select NULL", into(i, ind);
        assert(ind == eNull);
        sql << "select \'Hello\'", into(buf, ind);
        assert(ind == eTruncated);

        try
        {
            // expect error
            sql << "select NULL", into(i);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string error = e.what();
            assert(error ==
                "Null value fetched and no indicator defined.");
        }

        sql << "select 5 where 0 = 1", into(i, ind);
        assert(ind == eNoData);

        try
        {
            // expect error
            sql << "select 5 where 0 = 1", into(i);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            std::string error = e.what();
            assert(error ==
                "No data fetched and no indicator defined.");
        }
    }

    std::cout << "test 2 passed" << std::endl;
}

// repeated fetch and bulk fetch
void test3()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test3"; }
        catch (SOCIError const &) {} // ignore if error

        // repeated fetch and bulk fetch of char
        {
            // create and populate the test table
            sql << "create table test3 (c char)";

            char c;
            for (c = 'a'; c <= 'z'; ++c)
            {
                sql << "insert into test3(c) values(\'" << c << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == 'z' - 'a' + 1);

            {
                char c2 = 'a';

                Statement st = (sql.prepare <<
                    "select c from test3 order by c", into(c));

                st.execute();
                while (st.fetch())
                {
                    assert(c == c2);
                    ++c2;
                }
                assert(c2 == 'a' + count);
            }
            {
                char c2 = 'a';

                std::vector<char> vec(10);
                Statement st = (sql.prepare <<
                    "select c from test3 order by c", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t i = 0; i != vec.size(); ++i)
                    {
                        assert(c2 == vec[i]);
                        ++c2;
                    }

                    vec.resize(10);
                }
                assert(c2 == 'a' + count);
            }

            sql << "drop table test3";
        }

        // repeated fetch and bulk fetch of std::string
        {
            // create and populate the test table
            sql << "create table test3 (str varchar(10))";

            int const rowsToTest = 10;
            for (int i = 0; i != rowsToTest; ++i)
            {
                std::ostringstream ss;
                ss << "Hello_" << i;

                sql << "insert into test3(str) values(\'" << ss.str() << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == rowsToTest);

            {
                int i = 0;
                std::string s;
                Statement st = (sql.prepare <<
                    "select str from test3 order by str", into(s));

                st.execute();
                while (st.fetch())
                {
                    std::ostringstream ss;
                    ss << "Hello_" << i;
                    assert(s == ss.str());
                    ++i;
                }
                assert(i == rowsToTest);
            }
            {
                int i = 0;

                std::vector<std::string> vec(4);
                Statement st = (sql.prepare <<
                    "select str from test3 order by str", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t j = 0; j != vec.size(); ++j)
                    {
                        std::ostringstream ss;
                        ss << "Hello_" << i;
                        assert(ss.str() == vec[j]);
                        ++i;
                    }

                    vec.resize(4);
                }
                assert(i == rowsToTest);
            }

            sql << "drop table test3";
        }

        // repeated fetch and bulk fetch of short
        {
            // create and populate the test table
            sql << "create table test3 (sh int2)";

            short const rowsToTest = 100;
            short sh;
            for (sh = 0; sh != rowsToTest; ++sh)
            {
                sql << "insert into test3(sh) values(\'" << sh << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == rowsToTest);

            {
                short sh2 = 0;

                Statement st = (sql.prepare <<
                    "select sh from test3 order by sh", into(sh));

                st.execute();
                while (st.fetch())
                {
                    assert(sh == sh2);
                    ++sh2;
                }
                assert(sh2 == rowsToTest);
            }
            {
                short sh2 = 0;

                std::vector<short> vec(8);
                Statement st = (sql.prepare <<
                    "select sh from test3 order by sh", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t i = 0; i != vec.size(); ++i)
                    {
                        assert(sh2 == vec[i]);
                        ++sh2;
                    }

                    vec.resize(8);
                }
                assert(sh2 == rowsToTest);
            }

            sql << "drop table test3";
        }

        // repeated fetch and bulk fetch of int
        {
            // create and populate the test table
            sql << "create table test3 (id int4)";

            int const rowsToTest = 100;
            int i;
            for (i = 0; i != rowsToTest; ++i)
            {
                sql << "insert into test3(id) values(\'" << i << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == rowsToTest);

            {
                int i2 = 0;

                Statement st = (sql.prepare <<
                    "select id from test3 order by id", into(i));

                st.execute();
                while (st.fetch())
                {
                    assert(i == i2);
                    ++i2;
                }
                assert(i2 == rowsToTest);
            }
            {
                int i2 = 0;

                std::vector<int> vec(8);
                Statement st = (sql.prepare <<
                    "select id from test3 order by id", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t i = 0; i != vec.size(); ++i)
                    {
                        assert(i2 == vec[i]);
                        ++i2;
                    }

                    vec.resize(8);
                }
                assert(i2 == rowsToTest);
            }

            sql << "drop table test3";
        }

        // repeated fetch and bulk fetch of unsigned long
        {
            // create and populate the test table
            sql << "create table test3 (ul int4)";

            unsigned long const rowsToTest = 100;
            unsigned long ul;
            for (ul = 0; ul != rowsToTest; ++ul)
            {
                sql << "insert into test3(ul) values(\'" << ul << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == static_cast<int>(rowsToTest));

            {
                unsigned long ul2 = 0;

                Statement st = (sql.prepare <<
                    "select ul from test3 order by ul", into(ul));

                st.execute();
                while (st.fetch())
                {
                    assert(ul == ul2);
                    ++ul2;
                }
                assert(ul2 == rowsToTest);
            }
            {
                unsigned long ul2 = 0;

                std::vector<unsigned long> vec(8);
                Statement st = (sql.prepare <<
                    "select ul from test3 order by ul", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t i = 0; i != vec.size(); ++i)
                    {
                        assert(ul2 == vec[i]);
                        ++ul2;
                    }

                    vec.resize(8);
                }
                assert(ul2 == rowsToTest);
            }

            sql << "drop table test3";
        }

        // repeated fetch and bulk fetch of double
        {
            // create and populate the test table
            sql << "create table test3 (d float8)";

            int const rowsToTest = 100;
            double d = 0.0;
            for (int i = 0; i != rowsToTest; ++i)
            {
                sql << "insert into test3(d) values(\'" << d << "\')";
                d += 0.6;
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == rowsToTest);

            {
                double d2 = 0.0;
                int i = 0;

                Statement st = (sql.prepare <<
                    "select d from test3 order by d", into(d));

                st.execute();
                while (st.fetch())
                {
                    assert(std::abs(d - d2) < 0.001);
                    d2 += 0.6;
                    ++i;
                }
                assert(i == rowsToTest);
            }
            {
                double d2 = 0.0;
                int i = 0;

                std::vector<double> vec(8);
                Statement st = (sql.prepare <<
                    "select d from test3 order by d", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t j = 0; j != vec.size(); ++j)
                    {
                        assert(std::abs(d2 - vec[j]) < 0.001);
                        d2 += 0.6;
                        ++i;
                    }

                    vec.resize(8);
                }
                assert(i == rowsToTest);
            }

            sql << "drop table test3";
        }

        // TODO: the same for std::tm
    }

    std::cout << "test 3 passed" << std::endl;
}

// TODO:
// test for indicators (repeated fetch and bulk)
// test for different sizes of data vector and indicators vector
// (library should force ind. vector to have same size as data vector)

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
            << " \'connect_string_for_PostgreSQL\'\n";
        exit(1);
    }

    try
    {
        test1();
        test2();
        test3();

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
