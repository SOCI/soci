#include "soci.h"
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

        // repeated fetch and bulk fetch of std::tm
        {
            // create and populate the test table
            sql << "create table test3 (id integer, tm timestamp)";

            int const rowsToTest = 8;
            for (int i = 0; i != rowsToTest; ++i)
            {
                std::ostringstream ss;
                ss << 2000 + i << "-0" << 1 + i << '-' << 20 - i << ' '
                    << 15 + i << ':' << 50 - i << ':' << 40 + i;

                sql << "insert into test3(id, tm) values(" << i
                    << ", \'" << ss.str() << "\')";
            }

            int count;
            sql << "select count(*) from test3", into(count);
            assert(count == rowsToTest);

            {
                std::tm t;
                int i = 0;

                Statement st = (sql.prepare <<
                    "select tm from test3 order by id", into(t));

                st.execute();
                while (st.fetch())
                {
                    assert(t.tm_year + 1900 == 2000 + i);
                    assert(t.tm_mon + 1 == 1 + i);
                    assert(t.tm_mday == 20 - i);
                    assert(t.tm_hour == 15 + i);
                    assert(t.tm_min == 50 - i);
                    assert(t.tm_sec == 40 + i);

                    ++i;
                }
                assert(i == rowsToTest);
            }
            {
                int i = 0;

                std::vector<std::tm> vec(3);
                Statement st = (sql.prepare <<
                    "select tm from test3 order by id", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t j = 0; j != vec.size(); ++j)
                    {
                        assert(vec[j].tm_year + 1900 == 2000 + i);
                        assert(vec[j].tm_mon + 1 == 1 + i);
                        assert(vec[j].tm_mday == 20 - i);
                        assert(vec[j].tm_hour == 15 + i);
                        assert(vec[j].tm_min == 50 - i);
                        assert(vec[j].tm_sec == 40 + i);

                        ++i;
                    }

                    vec.resize(3);
                }
                assert(i == rowsToTest);
            }

            sql << "drop table test3";
        }
    }

    std::cout << "test 3 passed" << std::endl;
}

// test for indicators (repeated fetch and bulk)
void test4()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test4"; }
        catch (SOCIError const &) {} // ignore if error

        sql << "create table test4 (id integer, val integer)";

        sql << "insert into test4(id, val) values(1, 10)";
        sql << "insert into test4(id, val) values(2, 11)";
        sql << "insert into test4(id, val) values(3, NULL)";
        sql << "insert into test4(id, val) values(4, NULL)";
        sql << "insert into test4(id, val) values(5, 12)";

        {
            int val;
            eIndicator ind;

            Statement st = (sql.prepare <<
                "select val from test4 order by id", into(val, ind));

            st.execute();
            assert(st.fetch());
            assert(ind == eOK);
            assert(val == 10);
            assert(st.fetch());
            assert(ind == eOK);
            assert(val == 11);
            assert(st.fetch());
            assert(ind == eNull);
            assert(st.fetch());
            assert(ind == eNull);
            assert(st.fetch());
            assert(ind == eOK);
            assert(val == 12);
            assert(!st.fetch());
        }
        {
            std::vector<int> vals(3);
            std::vector<eIndicator> inds(3);

            Statement st = (sql.prepare <<
                "select val from test4 order by id", into(vals, inds));

            st.execute();
            assert(st.fetch());
            assert(vals.size() == 3);
            assert(inds.size() == 3);
            assert(inds[0] == eOK);
            assert(vals[0] == 10);
            assert(inds[1] == eOK);
            assert(vals[1] == 11);
            assert(inds[2] == eNull);
            assert(st.fetch());
            assert(vals.size() == 2);
            assert(inds[0] == eNull);
            assert(inds[1] == eOK);
            assert(vals[1] == 12);
            assert(!st.fetch());
        }

        // additional test for "no data" condition
        {
            std::vector<int> vals(3);
            std::vector<eIndicator> inds(3);

            Statement st = (sql.prepare <<
                "select val from test4 where 0 = 1", into(vals, inds));

            assert(!st.execute(1));
        }

        sql << "drop table test4";
    }

    std::cout << "test 4 passed" << std::endl;
}

// test for different sizes of data vector and indicators vector
// (library should force ind. vector to have same size as data vector)
void test5()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test5"; }
        catch (SOCIError const &) {} // ignore if error

        sql << "create table test5 (id integer, val integer)";

        sql << "insert into test5(id, val) values(1, 10)";
        sql << "insert into test5(id, val) values(2, 11)";
        sql << "insert into test5(id, val) values(3, NULL)";
        sql << "insert into test5(id, val) values(4, NULL)";
        sql << "insert into test5(id, val) values(5, 12)";

        {
            std::vector<int> vals(4);
            std::vector<eIndicator> inds;

            Statement st = (sql.prepare <<
                "select val from test5 order by id", into(vals, inds));

            st.execute();
            st.fetch();
            assert(vals.size() == 4);
            assert(inds.size() == 4);
            vals.resize(3);
            st.fetch();
            assert(vals.size() == 1);
            assert(inds.size() == 1);
        }

        sql << "drop table test5";
    }

    std::cout << "test 5 passed" << std::endl;
}

// "use" tests, type conversions, etc.
void test6()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test6"; }
        catch (SOCIError const &) {} // ignore if error

        // test for char
        {
            sql << "create table test6 (c char)";

            char c('a');
            sql << "insert into test6(c) values($1)", use(c);

            c = 'b';
            sql << "select c from test6", into(c);
            assert(c == 'a');

            sql << "drop table test6";
        }

        // test for char[]
        {
            sql << "create table test6 (s varchar(10))";

            char s[] = "Hello";
            sql << "insert into test6(s) values($1)", use(s);

            std::string str;
            sql << "select s from test6", into(str);

            assert(str == "Hello");

            sql << "drop table test6";
        }

        // test for std::string
        {
            sql << "create table test6 (s varchar(20))";

            std::string s = "Hello SOCI!";
            sql << "insert into test6(s) values($1)", use(s);

            std::string str;
            sql << "select s from test6", into(str);

            assert(str == "Hello SOCI!");

            sql << "drop table test6";
        }

        // test for short
        {
            sql << "create table test6 (s integer)";

            short s = 123;
            sql << "insert into test6(s) values($1)", use(s);

            short s2 = 0;
            sql << "select s from test6", into(s2);

            assert(s2 == 123);

            sql << "drop table test6";
        }

        // test for int
        {
            sql << "create table test6 (i integer)";

            int i = -12345678;
            sql << "insert into test6(i) values($1)", use(i);

            int i2 = 0;
            sql << "select i from test6", into(i2);

            assert(i2 == -12345678);

            sql << "drop table test6";
        }

        // test for unsigned long
        {
            sql << "create table test6 (num numeric(20))";

            unsigned long ul = 4000000000ul;
            sql << "insert into test6(num) values($1)", use(ul);

            std::string s;
            sql << "select num from test6", into(s);

            assert(s == "4000000000");

            sql << "drop table test6";
        }

        // test for double
        {
            sql << "create table test6 (d float8)";

            double d = 3.14159265;
            sql << "insert into test6(d) values($1)", use(d);

            double d2 = 0;
            sql << "select d from test6", into(d2);

            assert(std::abs(d2 - d) < 0.0001);

            sql << "drop table test6";
        }

        // test for std::tm
        {
            sql << "create table test6 (tm timestamptz)";

            std::tm t;
            t.tm_year = 105;
            t.tm_mon = 10;
            t.tm_mday = 19;
            t.tm_hour = 21;
            t.tm_min = 39;
            t.tm_sec = 57;
            sql << "insert into test6(tm) values($1)", use(t);

            std::tm t2;
            t2.tm_year = 0;
            t2.tm_mon = 0;
            t2.tm_mday = 0;
            t2.tm_hour = 0;
            t2.tm_min = 0;
            t2.tm_sec = 0;

            sql << "select tm from test6", into(t2);

            assert(t.tm_year == 105);
            assert(t.tm_mon  == 10);
            assert(t.tm_mday == 19);
            assert(t.tm_hour == 21);
            assert(t.tm_min  == 39);
            assert(t.tm_sec  == 57);

            sql << "drop table test6";
        }

        // test for repeated use
        {
            sql << "create table test6 (id integer)";

            int i;
            Statement st = (sql.prepare
                << "insert into test6(id) values($1)", use(i));

            i = 5;
            st.execute(1);
            i = 6;
            st.execute(1);
            i = 7;
            st.execute(1);

            std::vector<int> v(5);
            sql << "select id from test6 order by id", into(v);

            assert(v.size() == 3);
            assert(v[0] == 5);
            assert(v[1] == 6);
            assert(v[2] == 7);

            sql << "drop table test6";
        }

    }

    std::cout << "test 6 passed" << std::endl;
}

// test for multiple use (and into) elements
void test7()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test7"; }
        catch (SOCIError const &) {} // ignore if error

        {
            sql << "create table test7 (i1 integer, i2 integer, i3 integer)";

            int i1 = 5;
            int i2 = 6;
            int i3 = 7;

            sql << "insert into test7(i1, i2, i3) values($1, $2, $3)",
                use(i1), use(i2), use(i3);

            i1 = 0;
            i2 = 0;
            i3 = 0;
            sql << "select i1, i2, i3 from test7",
                into(i1), into(i2), into(i3);

            assert(i1 == 5);
            assert(i2 == 6);
            assert(i3 == 7);

            // same for vectors
            sql << "delete from test7";

            i1 = 0;
            i2 = 0;
            i3 = 0;

            Statement st = (sql.prepare
                << "insert into test7(i1, i2, i3) values($1, $2, $3)",
                use(i1), use(i2), use(i3));

            i1 = 1;
            i2 = 2;
            i3 = 3;
            st.execute(1);
            i1 = 4;
            i2 = 5;
            i3 = 6;
            st.execute(1);
            i1 = 7;
            i2 = 8;
            i3 = 9;
            st.execute(1);
            
            std::vector<int> v1(5);
            std::vector<int> v2(5);
            std::vector<int> v3(5);

            sql << "select i1, i2, i3 from test7 order by i1",
                into(v1), into(v2), into(v3);

            assert(v1.size() == 3);
            assert(v2.size() == 3);
            assert(v3.size() == 3);
            assert(v1[0] == 1);
            assert(v1[1] == 4);
            assert(v1[2] == 7);
            assert(v2[0] == 2);
            assert(v2[1] == 5);
            assert(v2[2] == 8);
            assert(v3[0] == 3);
            assert(v3[1] == 6);
            assert(v3[2] == 9);

            sql << "drop table test7";
        }
    }

    std::cout << "test 7 passed" << std::endl;
}

// use vector elements
void test8()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test8"; }
        catch (SOCIError const &) {} // ignore if error

        // test for char
        {
            sql << "create table test8 (c char)";

            std::vector<char> v;
            v.push_back('a');
            v.push_back('b');
            v.push_back('c');
            v.push_back('d');

            sql << "insert into test8(c) values($1)", use(v);

            std::vector<char> v2(4);

            sql << "select c from test8 order by c", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == 'a');
            assert(v2[1] == 'b');
            assert(v2[2] == 'c');
            assert(v2[3] == 'd');

            sql << "drop table test8";
        }

        // test for std::string
        {
            sql << "create table test8 (str varchar(10))";

            std::vector<std::string> v;
            v.push_back("ala");
            v.push_back("ma");
            v.push_back("kota");

            sql << "insert into test8(str) values($1)", use(v);

            std::vector<std::string> v2(4);

            sql << "select str from test8 order by str", into(v2);
            assert(v2.size() == 3);
            assert(v2[0] == "ala");
            assert(v2[1] == "kota");
            assert(v2[2] == "ma");

            sql << "drop table test8";
        }

        // test for short
        {
            sql << "create table test8 (sh int2)";

            std::vector<short> v;
            v.push_back(-5);
            v.push_back(6);
            v.push_back(7);
            v.push_back(123);

            sql << "insert into test8(sh) values($1)", use(v);

            std::vector<short> v2(4);

            sql << "select sh from test8 order by sh", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == -5);
            assert(v2[1] == 6);
            assert(v2[2] == 7);
            assert(v2[3] == 123);

            sql << "drop table test8";
        }

        // test for int
        {
            sql << "create table test8 (i int4)";

            std::vector<int> v;
            v.push_back(-2000000000);
            v.push_back(0);
            v.push_back(1);
            v.push_back(2000000000);

            sql << "insert into test8(i) values($1)", use(v);

            std::vector<int> v2(4);

            sql << "select i from test8 order by i", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == -2000000000);
            assert(v2[1] == 0);
            assert(v2[2] == 1);
            assert(v2[3] == 2000000000);

            sql << "drop table test8";
        }

        // test for unsigned long
        {
            sql << "create table test8 (ul int4)";

            std::vector<unsigned long> v;
            v.push_back(0);
            v.push_back(1);
            v.push_back(123);
            v.push_back(1000);

            sql << "insert into test8(ul) values($1)", use(v);

            std::vector<unsigned long> v2(4);

            sql << "select ul from test8 order by ul", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == 0);
            assert(v2[1] == 1);
            assert(v2[2] == 123);
            assert(v2[3] == 1000);

            sql << "drop table test8";
        }

        // test for char
        {
            sql << "create table test8 (d float8)";

            std::vector<double> v;
            v.push_back(0);
            v.push_back(-0.0001);
            v.push_back(0.0001);
            v.push_back(3.1415926);

            sql << "insert into test8(d) values($1)", use(v);

            std::vector<double> v2(4);

            sql << "select d from test8 order by d", into(v2);
            assert(v2.size() == 4);
            assert(std::abs(v2[0] + 0.0001) < 0.00001);
            assert(std::abs(v2[1]) < 0.0001);
            assert(std::abs(v2[2] - 0.0001) < 0.00001);
            assert(std::abs(v2[3] - 3.1415926 < 0.0001));

            sql << "drop table test8";
        }

        // test for std::tm
        {
            sql << "create table test8 (tm timestamp)";

            std::vector<std::tm> v;
            std::tm t;
            t.tm_year = 105;
            t.tm_mon  = 10;
            t.tm_mday = 26;
            t.tm_hour = 22;
            t.tm_min  = 45;
            t.tm_sec  = 17;

            v.push_back(t);

            t.tm_sec = 37;
            v.push_back(t);

            t.tm_mday = 25;
            v.push_back(t);

            sql << "insert into test8(tm) values($1)", use(v);

            std::vector<std::tm> v2(4);

            sql << "select tm from test8 order by tm", into(v2);
            assert(v2.size() == 3);
            assert(v2[0].tm_year == 105);
            assert(v2[0].tm_mon  == 10);
            assert(v2[0].tm_mday == 25);
            assert(v2[0].tm_hour == 22);
            assert(v2[0].tm_min  == 45);
            assert(v2[0].tm_sec  == 37);
            assert(v2[1].tm_year == 105);
            assert(v2[1].tm_mon  == 10);
            assert(v2[1].tm_mday == 26);
            assert(v2[1].tm_hour == 22);
            assert(v2[1].tm_min  == 45);
            assert(v2[1].tm_sec  == 17);
            assert(v2[2].tm_year == 105);
            assert(v2[2].tm_mon  == 10);
            assert(v2[2].tm_mday == 26);
            assert(v2[2].tm_hour == 22);
            assert(v2[2].tm_min  == 45);
            assert(v2[2].tm_sec  == 37);

            sql << "drop table test8";
        }
    }

    std::cout << "test 8 passed" << std::endl;
}

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
        test4();
        test5();
        test6();
        test7();
        test8();

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
