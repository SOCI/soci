//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef COMMON_TESTS_H_INCLUDED
#define COMMON_TESTS_H_INCLUDED

#include "soci.h"
#include "soci-config.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

// Objects used later in tests 14,15
struct PhonebookEntry
{
    std::string name;
    std::string phone;
};

struct PhonebookEntry2 : public PhonebookEntry 
{
};

namespace SOCI
{
// basic type conversion
template<> struct TypeConversion<PhonebookEntry>
{
    typedef SOCI::Values base_type;
    static PhonebookEntry from(Values const &v)
    {
        PhonebookEntry p;
        p.name = v.get<std::string>("name");
        p.phone = v.get<std::string>("phone", "<NULL>");
        return p;
    }
    static Values to(PhonebookEntry &p)
    {
        Values v;
        v.set("name", p.name);
        v.set("phone", p.phone, p.phone.empty() ? eNull : eOK);
        return v;
    }
};    

// type conversion which directly calls Values::indicator()
template<> struct TypeConversion<PhonebookEntry2>
{
    typedef SOCI::Values base_type;
    static PhonebookEntry2 from(Values const &v)
    {
        PhonebookEntry2 p;
        p.name = v.get<std::string>("name");
        eIndicator ind = v.indicator("phone"); //another way to test for null
        p.phone = ind == eNull ? "<NULL>" : v.get<std::string>("phone");
        return p;
    }
    static Values to(PhonebookEntry2 &p)
    {
        Values v;
        v.set("name", p.name);
        v.set("phone", p.phone, p.phone.empty() ? eNull : eOK);
        return v;
    }
};    

}

namespace SOCI
{
namespace tests
{

class TableCreatorBase
{
public:
    TableCreatorBase(Session& session)
        : mSession(session) { drop(); }

    virtual ~TableCreatorBase() { drop();}
private:
    void drop()
    {
        try { mSession << "drop table soci_test"; } catch(SOCIError&) {} 
    }
    Session& mSession;
};

class ProcedureCreatorBase
{
public:
    ProcedureCreatorBase(Session& session)
        : mSession(session) { drop(); }

    virtual ~ProcedureCreatorBase() { drop();}
private:
    void drop()
    {
        try { mSession << "drop procedure soci_test"; } catch(SOCIError&) {} 
    }
    Session& mSession;
};

class FunctionCreatorBase
{
public:
    FunctionCreatorBase(Session& session)
        : mSession(session) { drop(); }

    virtual ~FunctionCreatorBase() { drop();}

protected:
    virtual std::string dropStatement()
    { 
        return "drop function soci_test";
    }

private:
    void drop()
    {
        try { mSession << dropStatement(); } catch(SOCIError&) {} 
    }
    Session& mSession;
};

class TestContextBase
{
public:
    TestContextBase(BackEndFactory const &backEnd,
                    std::string const &connectString)
        : backEndFactory_(backEnd),
          connectString_(connectString) {}

    const BackEndFactory& getBackEndFactory() const
    {
        return backEndFactory_;
    }
    
    std::string getConnectString() const
    {
        return connectString_;
    }

    virtual std::string toDateTime(std::string const &dateTime) const = 0;

    virtual TableCreatorBase* tableCreator1(Session&) const = 0;
    virtual TableCreatorBase* tableCreator2(Session&) const = 0;
    virtual TableCreatorBase* tableCreator3(Session&) const = 0;

    virtual ~TestContextBase() {} // quiet the compiler

private:
    BackEndFactory const &backEndFactory_;
    std::string const connectString_;
};

class CommonTests
{
public:
    CommonTests(TestContextBase const &tc)
    : tc_(tc), 
      backEndFactory_(tc.getBackEndFactory()),
      connectString_(tc.getConnectString())
    {}

    void run(bool dbSupportsTransactions = true)
    {
    std::cout<<"\nSOCI Common Tests:\n\n";

    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8(); 
    test9();

    if (dbSupportsTransactions)
    {
        test10();
    }
    else
    {
        std::cout<<"skipping test 10 (database doesn't support transactions)\n";
    }

    test11();
    test12();
    test13();
    test14();
    test15();
    test16();
    test17();
    test18();
    test19();
    test20();
    test21();
    test22();
    test23();
    test24();
    test25();
    }

private:
    TestContextBase const & tc_;
    BackEndFactory const &backEndFactory_;
    std::string const connectString_;

typedef std::auto_ptr<TableCreatorBase> AutoTableCreator;

void test1()
{
    Session sql(backEndFactory_, connectString_);
    
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));

    std::string msg;
    try
    {
        // expected error
        sql << "drop table soci_test_nosuchtable";
        assert(false);
    }
    catch (SOCIError const &e)
    {
        msg = e.what();
    }
    assert(!msg.empty()); 

    sql << "insert into soci_test (id) values (" << 123 << ")";
    int id;
    sql << "select id from soci_test", into(id);
    assert(id == 123);

    std::cout << "test 1 passed\n";
}

// "into" tests, type conversions, etc.
void test2()
{
    {
        Session sql(backEndFactory_, connectString_);

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            char c('a');
            sql << "insert into soci_test(c) values(:c)", use(c);
            sql << "select c from soci_test", into(c);
            assert(c == 'a');
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::string abc("ABC");
            sql << "insert into soci_test(str) values(:s)", use(abc);

            char buf[4];
            sql << "select str from soci_test", into(buf);
            assert(buf[0] == 'A');
            assert(buf[1] == 'B');
            assert(buf[2] == 'C');
            assert(buf[3] == '\0');
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::string hello("Hello");
            sql << "insert into soci_test(str) values(:s)", use(hello);

            char buf[4];
            sql << "select str from soci_test", into(buf);
            assert(buf[0] == 'H');
            assert(buf[1] == 'e');
            assert(buf[2] == 'l');
            assert(buf[3] == '\0');
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
        
            std::string helloSOCI("Hello, SOCI!");
            sql << "insert into soci_test(str) values(:s)", use(helloSOCI);
            std::string str;
            sql << "select str from soci_test", into(str);
            assert(str == "Hello, SOCI!");
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            
            short three(3);
            sql << "insert into soci_test(sh) values(:id)", use(three);
            short sh(0);
            sql << "select sh from soci_test", into(sh);
            assert(sh == 3);
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
         
            int five(5);
            sql << "insert into soci_test(id) values(:id)", use(five);
            int i(0);
            sql << "select id from soci_test", into(i);
            assert(i == 5);
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            unsigned long seven(7);
            sql << "insert into soci_test(ul) values(:ul)", use(seven);
            unsigned long ul(0);
            sql << "select ul from soci_test", into(ul);
            assert(ul == 7);
        }

        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
        
            double pi(3.14159265);
            sql << "insert into soci_test(d) values(:d)", use(pi);
            double d(0.0);
            sql << "select d from soci_test", into(d);
            assert(std::fabs(d - 3.14159265) < 0.001);
        }
        
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::tm nov15;
            nov15.tm_year = 105;
            nov15.tm_mon = 10;
            nov15.tm_mday = 15;
            nov15.tm_hour = 0;
            nov15.tm_min = 0;
            nov15.tm_sec = 0;
        
            sql << "insert into soci_test(tm) values(:tm)", use(nov15);

            std::tm t;
            sql << "select tm from soci_test", into(t);
            assert(t.tm_year == 105);
            assert(t.tm_mon  == 10);
            assert(t.tm_mday == 15);
            assert(t.tm_hour == 0);
            assert(t.tm_min  == 0);
            assert(t.tm_sec  == 0);
        }
        
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::tm nov15;
            nov15.tm_year = 105;
            nov15.tm_mon = 10;
            nov15.tm_mday = 15;
            nov15.tm_hour = 22;
            nov15.tm_min = 14;
            nov15.tm_sec = 17;

            sql << "insert into soci_test(tm) values(:tm)", use(nov15);
            
            std::tm t;
            sql << "select tm from soci_test", into(t);
            assert(t.tm_year == 105);
            assert(t.tm_mon  == 10);
            assert(t.tm_mday == 15);
            assert(t.tm_hour == 22);
            assert(t.tm_min  == 14);
            assert(t.tm_sec  == 17);
        }

        // test indicators
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            int id(1);
            std::string str("Hello");
            sql << "insert into soci_test(id, str) values(:id, :str)",
                use(id), use(str); 

            int i;
            eIndicator ind;
            sql << "select id from soci_test", into(i, ind);
            assert(ind == eOK);

            char buf[4];
            sql << "select str from soci_test", into(buf, ind);
            assert(ind == eTruncated);
        }

        // more indicator tests, NULL values
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            sql << "insert into soci_test(id,tm) values(NULL,NULL)";
            int i;
            eIndicator ind;
            sql << "select id from soci_test", into(i, ind);
            assert(ind == eNull);

            // additional test for NULL with std::tm
            std::tm t;
            sql << "select tm from soci_test", into(t, ind);
            assert(ind == eNull);

            try
            {
                // expect error
                sql << "select id from soci_test", into(i);
                assert(false);
            }
            catch (SOCIError const &e)
            {
                std::string error = e.what();
                assert(error ==
                    "Null value fetched and no indicator defined.");
            }

            sql << "select id from soci_test where id = 1000", into(i, ind);
            assert(ind == eNoData);

            try
            {
                // expect error
                sql << "select id from soci_test where id = 1000", into(i);
                assert(false);
            }
            catch (SOCIError const &e)
            {
                std::string error = e.what();
                assert(error ==
                "No data fetched and no indicator defined.");
            }
        }
    }

    std::cout << "test 2 passed" << std::endl;
}

// repeated fetch and bulk fetch
void test3()
{
    {
        Session sql(backEndFactory_, connectString_);

        // repeated fetch and bulk fetch of char
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            char c;
            for (c = 'a'; c <= 'z'; ++c)
            {
                sql << "insert into soci_test(c) values(\'" << c << "\')";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == 'z' - 'a' + 1);

            {
                char c2 = 'a';

                Statement st = (sql.prepare <<
                    "select c from soci_test order by c", into(c));

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
                    "select c from soci_test order by c", into(vec));
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

            {
                // verify an exception is thrown when empty vector is used
                std::vector<char> vec;
                try
                {
                    sql << "select c from soci_test", into(vec);
                    assert(false);
                }
                catch (SOCIError const &e)
                {
                     std::string msg = e.what();
                     assert(msg == "Vectors of size 0 are not allowed.");
                }
            }

        }

        // repeated fetch and bulk fetch of std::string
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            
            int const rowsToTest = 10;
            for (int i = 0; i != rowsToTest; ++i)
            {
                std::ostringstream ss;
                ss << "Hello_" << i;

                sql << "insert into soci_test(str) values(\'"
                    << ss.str() << "\')";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == rowsToTest);

            {
                int i = 0;
                std::string s;
                Statement st = (sql.prepare <<
                    "select str from soci_test order by str", into(s));

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
                    "select str from soci_test order by str", into(vec));
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
        }

        // repeated fetch and bulk fetch of short
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            short const rowsToTest = 100;
            short sh;
            for (sh = 0; sh != rowsToTest; ++sh)
            {
                sql << "insert into soci_test(sh) values(\'" << sh << "\')";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == rowsToTest);

            {
                short sh2 = 0;

                Statement st = (sql.prepare <<
                    "select sh from soci_test order by sh", into(sh));

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
                    "select sh from soci_test order by sh", into(vec));
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
        }

        // repeated fetch and bulk fetch of int
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            int const rowsToTest = 100;
            int i;
            for (i = 0; i != rowsToTest; ++i)
            {
                sql << "insert into soci_test(id) values(\'" << i << "\')";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == rowsToTest);

            {
                int i2 = 0;

                Statement st = (sql.prepare <<
                    "select id from soci_test order by id", into(i));

                st.execute();
                while (st.fetch())
                {
                    assert(i == i2);
                    ++i2;
                }
                assert(i2 == rowsToTest);
            }
            {
                // additional test with the use element

                int i2 = 0;
                int cond = 0; // this condition is always true

                Statement st = (sql.prepare <<
                    "select id from soci_test where id >= :cond order by id",
                    use(cond), into(i));

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
                    "select id from soci_test order by id", into(vec));
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
        }

        // repeated fetch and bulk fetch of unsigned long
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));   

            unsigned long const rowsToTest = 100;
            unsigned long ul;
            for (ul = 0; ul != rowsToTest; ++ul)
            {
                sql << "insert into soci_test(ul) values(\'" << ul << "\')";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == static_cast<int>(rowsToTest));

            {
                unsigned long ul2 = 0;

                Statement st = (sql.prepare <<
                    "select ul from soci_test order by ul", into(ul));

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
                    "select ul from soci_test order by ul", into(vec));
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
        }

        // repeated fetch and bulk fetch of double
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            int const rowsToTest = 100;
            double d = 0.0;
            for (int i = 0; i != rowsToTest; ++i)
            {
                sql << "insert into soci_test(d) values(\'" << d << "\')";
                d += 0.6;
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == rowsToTest);

            {
                double d2 = 0.0;
                int i = 0;

                Statement st = (sql.prepare <<
                    "select d from soci_test order by d", into(d));

                st.execute();
                while (st.fetch())
                {
                    assert(std::fabs(d - d2) < 0.001);
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
                    "select d from soci_test order by d", into(vec));
                st.execute();
                while (st.fetch())
                {
                    for (std::size_t j = 0; j != vec.size(); ++j)
                    {
                        assert(std::fabs(d2 - vec[j]) < 0.001);
                        d2 += 0.6;
                        ++i;
                    }

                    vec.resize(8);
                }
                assert(i == rowsToTest);
            }
        }

        // repeated fetch and bulk fetch of std::tm
        {
            // create and populate the test table
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            int const rowsToTest = 8;
            for (int i = 0; i != rowsToTest; ++i)
            {
                std::ostringstream ss;
                ss << 2000 + i << "-0" << 1 + i << '-' << 20 - i << ' '
                    << 15 + i << ':' << 50 - i << ':' << 40 + i;

                sql << "insert into soci_test(id, tm) values(" << i
                << ", " << tc_.toDateTime(ss.str()) << ")";
            }

            int count;
            sql << "select count(*) from soci_test", into(count);
            assert(count == rowsToTest);

            {
                std::tm t;
                int i = 0;

                Statement st = (sql.prepare <<
                    "select tm from soci_test order by id", into(t));

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
                    "select tm from soci_test order by id", into(vec));
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
        }
    }

    std::cout << "test 3 passed" << std::endl;
}
    
// test for indicators (repeated fetch and bulk)
void test4()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(id, val) values(1, 10)";
        sql << "insert into soci_test(id, val) values(2, 11)";
        sql << "insert into soci_test(id, val) values(3, NULL)";
        sql << "insert into soci_test(id, val) values(4, NULL)";
        sql << "insert into soci_test(id, val) values(5, 12)";

        {
            int val;
            eIndicator ind;

            Statement st = (sql.prepare <<
                "select val from soci_test order by id", into(val, ind));

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
                "select val from soci_test order by id", into(vals, inds));

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
                "select val from soci_test where 0 = 1", into(vals, inds));

            assert(!st.execute(true));
        }
    }

    std::cout << "test 4 passed" << std::endl;
}

// test for different sizes of data vector and indicators vector
// (library should force ind. vector to have same size as data vector)
void test5()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(id, val) values(1, 10)";
        sql << "insert into soci_test(id, val) values(2, 11)";
        sql << "insert into soci_test(id, val) values(3, NULL)";
        sql << "insert into soci_test(id, val) values(4, NULL)";
        sql << "insert into soci_test(id, val) values(5, 12)";

        {
            std::vector<int> vals(4);
            std::vector<eIndicator> inds;

            Statement st = (sql.prepare <<
                "select val from soci_test order by id", into(vals, inds));

            st.execute();
            st.fetch();
            assert(vals.size() == 4);
            assert(inds.size() == 4);
            vals.resize(3);
            st.fetch();
            assert(vals.size() == 1);
            assert(inds.size() == 1);
        }
    }

    std::cout << "test 5 passed" << std::endl;
}

// "use" tests, type conversions, etc.
void test6()
{
// Note: this functionality is not available with older PostgreSQL
#ifndef SOCI_PGSQL_NOPARAMS
    {
        Session sql(backEndFactory_, connectString_);

        // test for char
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            char c('a');
            sql << "insert into soci_test(c) values(:c)", use(c);

            c = 'b';
            sql << "select c from soci_test", into(c);
            assert(c == 'a');

        }

        // test for char[]
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            char s[] = "Hello";
            sql << "insert into soci_test(str) values(:s)", use(s);

            std::string str;
            sql << "select str from soci_test", into(str);

            assert(str == "Hello");
        }

        // test for std::string
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            std::string s = "Hello SOCI!";
            sql << "insert into soci_test(str) values(:s)", use(s);

            std::string str;
            sql << "select str from soci_test", into(str);

            assert(str == "Hello SOCI!");
        }

        // test for short
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            short s = 123;
            sql << "insert into soci_test(id) values(:id)", use(s);

            short s2 = 0;
            sql << "select id from soci_test", into(s2);

            assert(s2 == 123);
        }

        // test for int
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            int i = -12345678;
            sql << "insert into soci_test(id) values(:i)", use(i);

            int i2 = 0;
            sql << "select id from soci_test", into(i2);

            assert(i2 == -12345678);
        }

        // test for unsigned long
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            unsigned long ul = 4000000000ul;
            sql << "insert into soci_test(ul) values(:num)", use(ul);

            std::string s;
            sql << "select ul from soci_test", into(s);

            assert(s == "4000000000");
        }

        // test for double
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            double d = 3.14159265;
            sql << "insert into soci_test(d) values(:d)", use(d);

            double d2 = 0;
            sql << "select d from soci_test", into(d2);

            assert(std::fabs(d2 - d) < 0.0001);
        }

        // test for std::tm
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            std::tm t;
            t.tm_year = 105;
            t.tm_mon = 10;
            t.tm_mday = 19;
            t.tm_hour = 21;
            t.tm_min = 39;
            t.tm_sec = 57;
            sql << "insert into soci_test(tm) values(:t)", use(t);

            std::tm t2;
            t2.tm_year = 0;
            t2.tm_mon = 0;
            t2.tm_mday = 0;
            t2.tm_hour = 0;
            t2.tm_min = 0;
            t2.tm_sec = 0;

            sql << "select tm from soci_test", into(t2);

            assert(t.tm_year == 105);
            assert(t.tm_mon  == 10);
            assert(t.tm_mday == 19);
            assert(t.tm_hour == 21);
            assert(t.tm_min  == 39);
            assert(t.tm_sec  == 57);
        }

        // test for repeated use
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));
            int i;
            Statement st = (sql.prepare
                << "insert into soci_test(id) values(:id)", use(i));

            i = 5;
            st.execute(true);
            i = 6;
            st.execute(true);
            i = 7;
            st.execute(true);

            std::vector<int> v(5);
            sql << "select id from soci_test order by id", into(v);

            assert(v.size() == 3);
            assert(v[0] == 5);
            assert(v[1] == 6);
            assert(v[2] == 7);
        }

    }

    std::cout << "test 6 passed" << std::endl;
#endif // SOCI_PGSQL_NOPARAMS
}

// test for multiple use (and into) elements
void test7()
{
    {
        Session sql(backEndFactory_, connectString_);
        AutoTableCreator tableCreator(tc_.tableCreator1(sql));
        
        {
            int i1 = 5;
            int i2 = 6;
            int i3 = 7;

#ifndef SOCI_PGSQL_NOPARAMS

            sql << "insert into soci_test(i1, i2, i3) values(:i1, :i2, :i3)",
                use(i1), use(i2), use(i3);

#else
            // Older PostgreSQL does not support use elements.

            sql << "insert into test7(i1, i2, i3) values(5, 6, 7)";

#endif // SOCI_PGSQL_NOPARAMS

            i1 = 0;
            i2 = 0;
            i3 = 0;
            sql << "select i1, i2, i3 from soci_test",
                into(i1), into(i2), into(i3);

            assert(i1 == 5);
            assert(i2 == 6);
            assert(i3 == 7);

            // same for vectors
            sql << "delete from soci_test";

            i1 = 0;
            i2 = 0;
            i3 = 0;

#ifndef SOCI_PGSQL_NOPARAMS

            Statement st = (sql.prepare
                << "insert into soci_test(i1, i2, i3) values(:i1, :i2, :i3)",
                use(i1), use(i2), use(i3));

            i1 = 1;
            i2 = 2;
            i3 = 3;
            st.execute(true);
            i1 = 4;
            i2 = 5;
            i3 = 6;
            st.execute(true);
            i1 = 7;
            i2 = 8;
            i3 = 9;
            st.execute(true);

#else
            // Older PostgreSQL does not support use elements.

            sql << "insert into soci_test(i1, i2, i3) values(1, 2, 3)";
            sql << "insert into soci_test(i1, i2, i3) values(4, 5, 6)";
            sql << "insert into soci_test(i1, i2, i3) values(7, 8, 9)";

#endif // SOCI_PGSQL_NOPARAMS

            std::vector<int> v1(5);
            std::vector<int> v2(5);
            std::vector<int> v3(5);

            sql << "select i1, i2, i3 from soci_test order by i1",
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
        }
    }

    std::cout << "test 7 passed" << std::endl;
}

// use vector elements
void test8()
{
// Not supported with older PostgreSQL
#ifndef SOCI_PGSQL_NOPARAMS

    {
        Session sql(backEndFactory_, connectString_);

        // test for char
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<char> v;
            v.push_back('a');
            v.push_back('b');
            v.push_back('c');
            v.push_back('d');

            sql << "insert into soci_test(c) values(:c)", use(v);

            std::vector<char> v2(4);

            sql << "select c from soci_test order by c", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == 'a');
            assert(v2[1] == 'b');
            assert(v2[2] == 'c');
            assert(v2[3] == 'd');
        }

        // test for std::string
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<std::string> v;
            v.push_back("ala");
            v.push_back("ma");
            v.push_back("kota");

            sql << "insert into soci_test(str) values(:s)", use(v);

            std::vector<std::string> v2(4);

            sql << "select str from soci_test order by str", into(v2);
            assert(v2.size() == 3);
            assert(v2[0] == "ala");
            assert(v2[1] == "kota");
            assert(v2[2] == "ma");
        }

        // test for short
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<short> v;
            v.push_back(-5);
            v.push_back(6);
            v.push_back(7);
            v.push_back(123);

            sql << "insert into soci_test(sh) values(:sh)", use(v);

            std::vector<short> v2(4);

            sql << "select sh from soci_test order by sh", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == -5);
            assert(v2[1] == 6);
            assert(v2[2] == 7);
            assert(v2[3] == 123);
        }

        // test for int
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<int> v;
            v.push_back(-2000000000);
            v.push_back(0);
            v.push_back(1);
            v.push_back(2000000000);

            sql << "insert into soci_test(id) values(:i)", use(v);

            std::vector<int> v2(4);

            sql << "select id from soci_test order by id", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == -2000000000);
            assert(v2[1] == 0);
            assert(v2[2] == 1);
            assert(v2[3] == 2000000000);
        }

        // test for unsigned long
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<unsigned long> v;
            v.push_back(0);
            v.push_back(1);
            v.push_back(123);
            v.push_back(1000);

            sql << "insert into soci_test(ul) values(:ul)", use(v);

            std::vector<unsigned long> v2(4);

            sql << "select ul from soci_test order by ul", into(v2);
            assert(v2.size() == 4);
            assert(v2[0] == 0);
            assert(v2[1] == 1);
            assert(v2[2] == 123);
            assert(v2[3] == 1000);
        }

        // test for char
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            std::vector<double> v;
            v.push_back(0);
            v.push_back(-0.0001);
            v.push_back(0.0001);
            v.push_back(3.1415926);

            sql << "insert into soci_test(d) values(:d)", use(v);

            std::vector<double> v2(4);

            sql << "select d from soci_test order by d", into(v2);
            assert(v2.size() == 4);
            assert(std::fabs(v2[0] + 0.0001) < 0.00001);
            assert(std::fabs(v2[1]) < 0.0001);
            assert(std::fabs(v2[2] - 0.0001) < 0.00001);
            assert(std::fabs(double(v2[3] - 3.1415926 < 0.0001)));
        }

        // test for std::tm
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

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

            sql << "insert into soci_test(tm) values(:t)", use(v);

            std::vector<std::tm> v2(4);

            sql << "select tm from soci_test order by tm", into(v2);
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
        }
    }

    std::cout << "test 8 passed" << std::endl;

#endif // SOCI_PGSQL_NOPARAMS

}

// test for named binding
void test9()
{
// Not supported with older PostgreSQL
#ifndef SOCI_PGSQL_NOPARAMS

    {
        Session sql(backEndFactory_, connectString_);
        {
            AutoTableCreator tableCreator(tc_.tableCreator1(sql));

            int i1 = 7;
            int i2 = 8;

            // verify the exception is thrown if both by position
            // and by name use elements are specified
            try
            {
                sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
                    use(i1, "i1"), use(i2);

                assert(false);
            }
            catch (SOCIError const &e)
            {
                assert(std::string(e.what()) ==
                    "Binding for use elements must be either by position "
                    "or by name.");
            }

            // normal test
            sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
                use(i1, "i1"), use(i2, "i2");

            i1 = 0;
            i2 = 0;
            sql << "select i1, i2 from soci_test", into(i1), into(i2);
            assert(i1 == 7);
            assert(i2 == 8);

            i2 = 0;
            sql << "select i2 from soci_test where i1 = :i1", into(i2), use(i1);
            assert(i2 == 8);

            sql << "delete from soci_test";

            // test vectors

            std::vector<int> v1;
            v1.push_back(1);
            v1.push_back(2);
            v1.push_back(3);

            std::vector<int> v2;
            v2.push_back(4);
            v2.push_back(5);
            v2.push_back(6);

            sql << "insert into soci_test(i1, i2) values(:i1, :i2)",
                use(v1, "i1"), use(v2, "i2");

            sql << "select i2, i1 from soci_test order by i1 desc",
                into(v1), into(v2);
            assert(v1.size() == 3);
            assert(v2.size() == 3);
            assert(v1[0] == 6);
            assert(v1[1] == 5);
            assert(v1[2] == 4);
            assert(v2[0] == 3);
            assert(v2[1] == 2);
            assert(v2[2] == 1);
        }
    }

    std::cout << "test 9 passed" << std::endl;

#endif // SOCI_PGSQL_NOPARAMS

}

// transaction test
void test10()
{
    {
        Session sql(backEndFactory_, connectString_);

        AutoTableCreator tableCreator(tc_.tableCreator1(sql));

        int count;
        sql << "select count(*) from soci_test", into(count);
        assert(count == 0);

        {
            sql.begin();

#ifndef SOCI_PGSQL_NOPARAMS

            int id;
            std::string name;

            Statement st1 = (sql.prepare <<
                "insert into soci_test (id, name) values (:id, :name)",
                use(id), use(name));

            id = 1; name = "John"; st1.execute(true);
            id = 2; name = "Anna"; st1.execute(true);
            id = 3; name = "Mike"; st1.execute(true);

#else
            // Older PostgreSQL does not support use elements

            sql << "insert into soci_test (id, name) values(1, 'John')";
            sql << "insert into soci_test (id, name) values(2, 'Anna')";
            sql << "insert into soci_test (id, name) values(3, 'Mike')";

#endif // SOCI_PGSQL_NOPARAMS

            sql.commit();
            sql.begin();

            sql << "select count(*) from soci_test", into(count);
            assert(count == 3);

#ifndef SOCI_PGSQL_NOPARAMS
            id = 4; name = "Stan"; st1.execute(true);
#else
            sql << "insert into soci_test (id, name) values(4, 'Stan')";
#endif // SOCI_PGSQL_NOPARAMS

            sql << "select count(*) from soci_test", into(count);
            assert(count == 4);
            sql.rollback();

            sql << "select count(*) from soci_test", into(count);
            assert(count == 3);
        }
        {
            sql.begin();

            sql << "delete from soci_test";

            sql << "select count(*) from soci_test", into(count);
            assert(count == 0);
            sql.rollback();
            sql << "select count(*) from soci_test", into(count);
            assert(count == 3);
        }
    }

    std::cout << "test 10 passed" << std::endl;
}

// test of use elements with indicators
void test11()
{
#ifndef SOCI_PGSQL_NOPARAMS
    {
        Session sql(backEndFactory_, connectString_);

        AutoTableCreator tableCreator(tc_.tableCreator1(sql));

        eIndicator ind1 = eOK;
        eIndicator ind2 = eOK;

        int id = 1;
        int val = 10;

        sql << "insert into soci_test(id, val) values(:id, :val)",
            use(id, ind1), use(val, ind2);

        id = 2;
        val = 11;
        ind2 = eNull;
        sql << "insert into soci_test(id, val) values(:id, :val)",
            use(id, ind1), use(val, ind2);

        sql << "select val from soci_test where id = 1", into(val, ind2);
        assert(ind2 == eOK);
        assert(val == 10);
        sql << "select val from soci_test where id = 2", into(val, ind2);
        assert(ind2 == eNull);

        std::vector<int> ids;
        ids.push_back(3);
        ids.push_back(4);
        ids.push_back(5);
        std::vector<int> vals;
        vals.push_back(12);
        vals.push_back(13);
        vals.push_back(14);
        std::vector<eIndicator> inds;
        inds.push_back(eOK);
        inds.push_back(eNull);
        inds.push_back(eOK);

        sql << "insert into soci_test(id, val) values(:id, :val)",
            use(ids), use(vals, inds);

        ids.resize(5);
        vals.resize(5);
        sql << "select id, val from soci_test order by id desc",
            into(ids), into(vals, inds);

        assert(ids.size() == 5);
        assert(ids[0] == 5);
        assert(ids[1] == 4);
        assert(ids[2] == 3);
        assert(ids[3] == 2);
        assert(ids[4] == 1);
        assert(inds.size() == 5);
        assert(inds[0] == eOK);
        assert(inds[1] == eNull);
        assert(inds[2] == eOK);
        assert(inds[3] == eNull);
        assert(inds[4] == eOK);
        assert(vals.size() == 5);
        assert(vals[0] == 14);
        assert(vals[2] == 12);
        assert(vals[4] == 10);
    }

    std::cout << "test 11 passed" << std::endl;

#endif // SOCI_PGSQL_NOPARAMS
}

// Dynamic binding to Row objects
void test12()
{
    {
        Session sql(backEndFactory_, connectString_);

        AutoTableCreator tableCreator(tc_.tableCreator2(sql));

        Row r;
        sql << "select * from soci_test", into(r);
        assert(r.indicator(0) ==  eNoData);

        sql << "insert into soci_test"
            " values(3.14, 123, \'Johny\',"
            << tc_.toDateTime("2005-12-19 22:14:17")
            << ", 'a')";

        // select into a Row
        {
            Row r;
            Statement st = (sql.prepare <<
                "select * from soci_test", into(r));
            st.execute(true);
            assert(r.size() == 5);

            assert(r.getProperties(0).getDataType() == eDouble);
            assert(r.getProperties(1).getDataType() == eInteger);
            assert(r.getProperties(2).getDataType() == eString);
            assert(r.getProperties(3).getDataType() == eDate);

            // type char is visible as string
            // - to comply with the implementation for Oracle
            assert(r.getProperties(4).getDataType() == eString);

            assert(r.getProperties("num_int").getDataType() == eInteger);

            assert(r.getProperties(0).getName() == "num_float");
            assert(r.getProperties(1).getName() == "num_int");
            assert(r.getProperties(2).getName() == "name");
            assert(r.getProperties(3).getName() == "sometime");
            assert(r.getProperties(4).getName() == "chr");

            assert(std::fabs(r.get<double>(0) - 3.14) < 0.001);
            assert(r.get<int>(1) == 123);
            assert(r.get<std::string>(2) == "Johny");
            std::tm t = r.get<std::tm>(3);
            assert(t.tm_year == 105);

            // again, type char is visible as string
            assert(r.get<std::string>(4) == "a");

            assert(std::fabs(r.get<double>("num_float") - 3.14) < 0.001);
            assert(r.get<int>("num_int") == 123);
            assert(r.get<std::string>("name") == "Johny");
            assert(r.get<std::string>("chr") == "a");

            assert(r.indicator(0) == eOK);

            // verify exception thrown on invalid get<>
            bool caught = false;
            try
            {
                r.get<std::string>(0);
            }
            catch (std::bad_cast const &)
            {
                caught = true;
            }
            assert(caught);

            // additional test for stream-like extraction
            {
                double d;
                int i;
                std::string s;
                std::tm t;
                std::string c;

                r >> d >> i >> s >> t >> c;

                assert(std::fabs(d - 3.14) < 0.001);
                assert(i == 123);
                assert(s == "Johny");
                assert(t.tm_year == 105);
                assert(t.tm_mon == 11);
                assert(t.tm_mday == 19);
                assert(t.tm_hour == 22);
                assert(t.tm_min == 14);
                assert(t.tm_sec == 17);
                assert(c == "a");
            }

        }
    }

    std::cout << "test 12 passed" << std::endl;
}

// more dynamic bindings
void test13()
{
    Session sql(backEndFactory_, connectString_);

    AutoTableCreator tableCreator(tc_.tableCreator1(sql));

    sql << "insert into soci_test(id, val) values(1, 10)";
    sql << "insert into soci_test(id, val) values(2, 20)";
    sql << "insert into soci_test(id, val) values(3, 30)";

#ifndef SOCI_PGSQL_NOPARAMS
    {
        int id = 2;
        Row r;
        sql << "select val from soci_test where id = :id", use(id), into(r);

        assert(r.size() == 1);
        assert(r.getProperties(0).getDataType() == eInteger);
        assert(r.get<int>(0) == 20);
    }
    {
        int id;
        Row r;
        Statement st = (sql.prepare <<
            "select val from soci_test where id = :id", use(id), into(r));

        id = 2;
        st.execute(true);
        assert(r.size() == 1);
        assert(r.getProperties(0).getDataType() == eInteger);
        assert(r.get<int>(0) == 20);
        
        id = 3;
        st.execute(true);
        assert(r.size() == 1);
        assert(r.getProperties(0).getDataType() == eInteger);
        assert(r.get<int>(0) == 30);

        id = 1;
        st.execute(true);
        assert(r.size() == 1);
        assert(r.getProperties(0).getDataType() == eInteger);
        assert(r.get<int>(0) == 10);
    }
#else
    {
        Row r;
        sql << "select val from soci_test where id = 2", into(r);

        assert(r.size() == 1);
        assert(r.getProperties(0).getDataType() == eInteger);
        assert(r.get<int>(0) == 20);
    }
#endif // SOCI_PGSQL_NOPARAMS

    std::cout << "test 13 passed" << std::endl;
}

// More Dynamic binding to Row objects
void test14()
{
    {
        Session sql(backEndFactory_, connectString_);
        AutoTableCreator tableCreator(tc_.tableCreator3(sql));

        Row r1;
        sql << "select * from soci_test", into(r1);
        assert(r1.indicator(0) ==  eNoData);

        sql << "insert into soci_test values('david', '(404)123-4567')";
        sql << "insert into soci_test values('john', '(404)123-4567')";
        sql << "insert into soci_test values('doe', '(404)123-4567')";

        Row r2;
        Statement st = (sql.prepare << "select * from soci_test", into(r2));
        st.execute();
        
        assert(r2.size() == 2); 
        
        int count = 0;
        while(st.fetch())
        {
            ++count;
            assert(r2.get<std::string>("phone") == "(404)123-4567");
        }
        assert(count == 3);
    }
    std::cout << "test 14 passed" << std::endl;
}

// test15 is like test14 but with a TypeConversion instead of a row
void test15()
{   
    Session sql(backEndFactory_, connectString_);
    
    {
        AutoTableCreator tableCreator(tc_.tableCreator3(sql));

        PhonebookEntry p1;
        sql << "select * from soci_test", into(p1);
        assert(p1.name ==  "");
        assert(p1.phone == "");

        p1.name = "david";

        sql << "insert into soci_test values(:name, :phone)", use(p1);
        sql << "insert into soci_test values('john', '(404)123-4567')";
        sql << "insert into soci_test values('doe', '(404)123-4567')";

        PhonebookEntry p2;
        Statement st = (sql.prepare << "select * from soci_test", into(p2));
        st.execute();
        
        int count = 0;
        while(st.fetch())
        {
            ++count;
            if (p2.name == "david")
            {
                // see TypeConversion<PhonebookEntry>
                assert(p2.phone =="<NULL>");
            }
            else
            {
                assert(p2.phone == "(404)123-4567");
            }
        }
        assert(count == 3);        
    }

    {   // Use the PhonebookEntry2 type conversion, to test
        // calls to Values::indicator()
        AutoTableCreator tableCreator(tc_.tableCreator3(sql));

        PhonebookEntry2 p1;
        sql << "select * from soci_test", into(p1);
        assert(p1.name ==  "");
        assert(p1.phone == "");
        p1.name = "david";

        sql << "insert into soci_test values(:name, :phone)", use(p1);
        sql << "insert into soci_test values('john', '(404)123-4567')";
        sql << "insert into soci_test values('doe', '(404)123-4567')";

        PhonebookEntry2 p2;
        Statement st = (sql.prepare << "select * from soci_test", into(p2));
        st.execute();
        
        int count = 0;
        while(st.fetch())
        {
            ++count;
            if (p2.name == "david")
            {
                // see TypeConversion<PhonebookEntry2>
                assert(p2.phone =="<NULL>");
            }
            else
            {
                assert(p2.phone == "(404)123-4567");
            }
        }
        assert(count == 3);        
    }

    std::cout << "test 15 passed" << std::endl;
}

// test for bulk fetch with single use
void test16()
{
#ifndef SOCI_PGSQL_NOPARAMS
    {
        Session sql(backEndFactory_, connectString_);

        AutoTableCreator tableCreator(tc_.tableCreator1(sql));

        sql << "insert into soci_test(name, id) values('john', 1)";
        sql << "insert into soci_test(name, id) values('george', 2)";
        sql << "insert into soci_test(name, id) values('anthony', 1)";
        sql << "insert into soci_test(name, id) values('marc', 3)";
        sql << "insert into soci_test(name, id) values('julian', 1)";

        int code = 1;
        std::vector<std::string> names(10);
        sql << "select name from soci_test where id = :id order by name",
             into(names), use(code);

        assert(names.size() == 3);
        assert(names[0] == "anthony");
        assert(names[1] == "john");
        assert(names[2] == "julian");
    }
#endif // SOCI_PGSQL_NOPARAMS

    std::cout << "test 16 passed" << std::endl;
}

// test for basic logging support
void test17()
{
    Session sql(backEndFactory_, connectString_);

    std::ostringstream log;
    sql.setLogStream(&log);

    try
    {
        sql << "drop table soci_test1";
    }
    catch (...) {}

    assert(sql.getLastQuery() == "drop table soci_test1");

    sql.setLogStream(NULL);

    try
    {
        sql << "drop table soci_test2";
    }
    catch (...) {}

    assert(sql.getLastQuery() == "drop table soci_test2");

    sql.setLogStream(&log);

    try
    {
        sql << "drop table soci_test3";
    }
    catch (...) {}

    assert(sql.getLastQuery() == "drop table soci_test3");
    assert(log.str() ==
        "drop table soci_test1\n"
        "drop table soci_test3\n");

    std::cout << "test 17 passed\n";
}

// test for Rowset creation and copying
void test18()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        // Open empty rowset
        Rowset<Row> rs1 = (sql.prepare << "select * from soci_test");

        // Copy by assignment
        Rowset<Row> rs2 = rs1;
        Rowset<Row> rs3(rs2);

        // TODO - mloskot:
        // Fix issue with rs1.begin() == rs2.begin()
    }

    std::cout << "test 18 passed" << std::endl;
}

// test for simple iterating using Rowset iterator (without reading data)
void test19()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(id, val) values(1, 10)";
        sql << "insert into soci_test(id, val) values(2, 11)";
        sql << "insert into soci_test(id, val) values(3, NULL)";
        sql << "insert into soci_test(id, val) values(4, NULL)";
        sql << "insert into soci_test(id, val) values(5, 12)";
        {
            Rowset<Row> rs = (sql.prepare << "select * from soci_test");

            assert(5 == std::distance(rs.begin(), rs.end()));
        }
    }

    std::cout << "test 19 passed" << std::endl;
}

// test for reading Rowset<Row> using iterator
void test20()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator2(sql));
    {
        {
            // Empty rowset
            Rowset<Row> rs = (sql.prepare << "select * from soci_test order by num_float asc");
            assert(0 == std::distance(rs.begin(), rs.end()));
        }

        {
            // Non-empty rowset
            sql << "insert into soci_test values(3.14, 123, \'Johny\',"
                << tc_.toDateTime("2005-12-19 22:14:17")
                << ", 'a')";
            sql << "insert into soci_test values(6.28, 246, \'Robert\',"
                << tc_.toDateTime("2004-10-01 18:44:10")
                << ", 'b')";

            Rowset<Row> rs = (sql.prepare << "select * from soci_test");

            Rowset<Row>::const_iterator it = rs.begin(); 
            assert(it != rs.end());
                
            //
            // First row
            // 
            Row const& r1 = (*it);

            // Properties
            assert(r1.size() == 5);
            assert(r1.getProperties(0).getDataType() == eDouble);
            assert(r1.getProperties(1).getDataType() == eInteger);
            assert(r1.getProperties(2).getDataType() == eString);
            assert(r1.getProperties(3).getDataType() == eDate);
            assert(r1.getProperties(4).getDataType() == eString);
            assert(r1.getProperties("num_int").getDataType() == eInteger);

            // Data
            assert(std::fabs(r1.get<double>(0) - 3.14) < 0.001);
            assert(r1.get<int>(1) == 123);
            assert(r1.get<std::string>(2) == "Johny");
            std::tm t1 = r1.get<std::tm>(3);
            assert(t1.tm_year == 105);
            assert(r1.get<std::string>(4) == "a");
            assert(std::fabs(r1.get<double>("num_float") - 3.14) < 0.001);
            assert(r1.get<int>("num_int") == 123);
            assert(r1.get<std::string>("name") == "Johny");
            assert(r1.get<std::string>("chr") == "a");

            // 
            // Iterate to second row
            //
            ++it;
            assert(it != rs.end());

            //
            // Second row
            //
            Row const& r2 = (*it);

            // Properties
            assert(r2.size() == 5);
            assert(r2.getProperties(0).getDataType() == eDouble);
            assert(r2.getProperties(1).getDataType() == eInteger);
            assert(r2.getProperties(2).getDataType() == eString);
            assert(r2.getProperties(3).getDataType() == eDate);
            assert(r2.getProperties(4).getDataType() == eString);
            assert(r2.getProperties("num_int").getDataType() == eInteger);

            // Data
            assert(std::fabs(r2.get<double>(0) - 6.28) < 0.001);
            assert(r2.get<int>(1) == 246);
            assert(r2.get<std::string>(2) == "Robert");
            std::tm t2 = r2.get<std::tm>(3);
            assert(t2.tm_year == 104);
            assert(r2.get<std::string>(4) == "b");
            assert(std::fabs(r2.get<double>("num_float") - 6.28) < 0.001);
            assert(r2.get<int>("num_int") == 246);
            assert(r2.get<std::string>("name") == "Robert");
            assert(r2.get<std::string>("chr") == "b");
          
        }
    }

    std::cout << "test 20 passed" << std::endl;
}

// test for reading Rowset<int> using iterator
void test21()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(id) values(1)";
        sql << "insert into soci_test(id) values(2)";
        sql << "insert into soci_test(id) values(3)";
        sql << "insert into soci_test(id) values(4)";
        sql << "insert into soci_test(id) values(5)";
        {
            Rowset<int> rs = (sql.prepare << "select id from soci_test order by id asc");

            // 1st row
            Rowset<int>::const_iterator pos = rs.begin();
            assert(1 == (*pos));

            // 3rd row
            std::advance(pos, 2);
            assert(3 == (*pos));

            // 5th row
            std::advance(pos, 2);
            assert(5 == (*pos));

            // The End
            ++pos;
            assert(pos == rs.end());

            // XXX - advancing with negative value throws segfault
        }
    }

    std::cout << "test 21 passed" << std::endl;
}

// test for handling 'use' and reading Rowset<std::string> using iterator
void test22()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(str) values('abc')";
        sql << "insert into soci_test(str) values('def')";
        sql << "insert into soci_test(str) values('ghi')";
        sql << "insert into soci_test(str) values('jkl')";
        {
            // Expected result in numbers
            std::string idle("def");
            Rowset<std::string> rs1 = (sql.prepare
                    << "select str from soci_test where str = :idle",
                    use(idle));

            assert(1 == std::distance(rs1.begin(), rs1.end()));

            // Expected result in value
            idle = "jkl";
            Rowset<std::string> rs2 = (sql.prepare
                    << "select str from soci_test where str = :idle",
                    use(idle));

            assert(idle == *(rs2.begin()));
        }
    }

    std::cout << "test 22 passed" << std::endl;
}

// test for handling troublemaker
void test23()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(str) values('abc')";
        {
            // verify exception thrown
            bool caught = false;
            try
            {
                std::string troublemaker;
                Rowset<std::string> rs1 = (sql.prepare << "select str from soci_test",
                        into(troublemaker));
            }
            catch(SOCIError const& e)
            {
                caught = true;
            }
            assert(caught);
        }
        std::cout << "test 23 passed" << std::endl;
    }

}

// test for handling NULL values with expected exception:
// "Null value fetched and no indicator defined."
void test24()
{
    Session sql(backEndFactory_, connectString_);
    
    // create and populate the test table
    AutoTableCreator tableCreator(tc_.tableCreator1(sql));
    {
        sql << "insert into soci_test(val) values(1)";
        sql << "insert into soci_test(val) values(2)";
        sql << "insert into soci_test(val) values(NULL)";
        sql << "insert into soci_test(val) values(3)";
        {
            // verify exception thrown
            bool caught = false;
            try
            {
                std::string troublemaker;
                Rowset<int> rs = (sql.prepare << "select val from soci_test order by val asc");

                int tester = 0;
                for (Rowset<int>::const_iterator it = rs.begin(); it != rs.end(); ++it)
                {
                    tester = *it;
                }

                // Never should get here
                assert(false);
            }
            catch(SOCIError const& e)
            {
                caught = true;
            }
            assert(caught);
        }
        std::cout << "test 24 passed" << std::endl;
    }

}

// test25 is like test15 but with Rowset and iterators use
void test25()
{   
    Session sql(backEndFactory_, connectString_);
    
    {
        AutoTableCreator tableCreator(tc_.tableCreator3(sql));

        PhonebookEntry p1;
        sql << "select * from soci_test", into(p1);
        assert(p1.name ==  "");
        assert(p1.phone == "");

        p1.name = "david";

        sql << "insert into soci_test values(:name, :phone)", use(p1);
        sql << "insert into soci_test values('john', '(404)123-4567')";
        sql << "insert into soci_test values('doe', '(404)123-4567')";

        Rowset<PhonebookEntry> rs = (sql.prepare << "select * from soci_test");
        
        int count = 0;
        for (Rowset<PhonebookEntry>::const_iterator it = rs.begin(); it != rs.end(); ++it)
        {
            ++count;
            PhonebookEntry const& p2 = (*it);
            if (p2.name == "david")
            {
                // see TypeConversion<PhonebookEntry>
                assert(p2.phone =="<NULL>");
            }
            else
            {
                assert(p2.phone == "(404)123-4567");
            }
        }

        assert(3 == count);        
    }
    std::cout << "test 25 passed" << std::endl;
}

}; // class CommonTests

} // namespace tests
} // namespace SOCI

#endif // COMMON_TESTS_H_INCLUDED

