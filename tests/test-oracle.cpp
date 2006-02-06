//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-oracle.h"
#include <iostream>
#include <string>
#include <cassert>
#include <ctime>

using namespace SOCI;

std::string connectString;
std::string backEndName = "oracle";

// fundamental tests
void test1()
{
    {
        Session session(backEndName, connectString);

        int x = -5;

        try
        {
            session.once << "select null from dual", into(x);

            // exception expected (null value and no indicator)
            assert(false);
        }
        catch (std::exception const &e)
        {
            std::string msg(e.what());
            assert(msg == "Null value fetched and no indicator defined.");
        }

        try
        {
            session.once << "select 7 from dual where 0 = 1", into(x);

            // exception expected (no data and no indicator)
            assert(false);
        }
        catch (std::exception const &e)
        {
            std::string msg(e.what());
            assert(msg == "No data fetched and no indicator defined.");
        }

        eIndicator xind;
        session.once << "select 7 from dual where 0 = 1", into(x, xind);
        assert(xind == eNoData);

        session.once << "select 7 from dual where 1 = 1", into(x, xind);
        assert(xind == eOK && x == 7);

        session.once << "select null from dual where 1 = 1", into(x, xind);
        assert(xind == eNull && x == 7);

        int y = 0;
        session.once << "select 3, 4 from dual where 1 = 1", into(x), into(y);
        assert(x == 3 && y == 4);
    }

    {
        Session sql(backEndName, connectString);

        int a = 0;
        int b = 5;
        sql.once <<
            "select a from (select :b as a from dual)",
            into(a), use(b);

        assert(a == 5);
        eIndicator aind = eOK;
        eIndicator bind = eNull;
        sql.once << "select a from (select :b as a from dual)",
            into(a, aind), use(b, bind);

        assert(aind == eNull);

        int c = 10;
        int d = 11;
        sql.once << "select a, b from (select :c as a, :d as b from dual)",
            into(a), into(b), use(d, "d"), use(c, "c");
        assert(a == 10 && b == 11);
    }

    std::cout << "test 1 passed" << std::endl;
}

// type test
void test2()
{
    Session sql(backEndName, connectString);

    {
        double d1 = 0.0, d2 = 3.14;
        sql <<
            "select d from (select :d as d from dual)", into(d1), use(d2);

        // beware: this test is prone to typical floating-point issues
        // if it fails, it can mean that there are rounding errors
        // it works fine on my system, so I leave it as it is
        assert(d1 == d2 && d1 == 3.14);
    }

    {
        int i1 = 0, i2 = 12345678;
        sql <<
            "select i from (select :i as i from dual)", into(i1), use(i2);
        assert(i1 == i2 && i1 == 12345678);
    }

    {
        short s1 = 0, s2 = 12345;
        sql <<
            "select s from (select :s as s from dual)", into(s1), use(s2);
        assert(s1 == s2 && s1 == 12345);
    }

    {
        char c1 = 'a', c2 = 'x';
        sql <<
            "select c from (select :c as c from dual)", into(c1), use(c2);
        assert(c1 == c2 && c1 == 'x');
    }

    {
        unsigned long u1 = 4, u2 = 4000000000ul;
        sql <<
            "select u from (select :u as u from dual)", into(u1), use(u2);
        assert(u1 == u2 && u1 == 4000000000ul);
    }

    {
        char msg[] = "Hello, Oracle!";
        char buf1[100], buf2[100];
        char *b1 = buf1, *b2 = buf2;
        strcpy(b2, msg);
        sql << "select s from (select :s as s from dual)",
            into(b1, 100), use(b2, 100);
        assert(strcmp(b1, b2) == 0);
    }
    {
        char msg[] = "Hello, Oracle!";
        char buf1[100], buf2[100];
        strcpy(buf2, msg);
        sql << "select s from (select :s as s from dual)",
            into(buf1), use(buf2);
        assert(strcmp(buf1, buf2) == 0);
    }

    {
        std::string s1, s2("Hello, Oracle!");
        sql << "select s from (select :s as s from dual)",
            into(s1), use(s2);
        assert(s1 == s2);
    }

    {
        // date and time
        std::time_t now = std::time(NULL);
        std::tm t1, t2;
        t2 = *std::localtime(&now);

        sql << "select t from (select :t as t from dual)",
            into(t1), use(t2);

        assert(memcmp(&t1, &t2, sizeof(std::tm)) == 0);

        // make sure the date is stored properly in Oracle
        char buf[25];
        strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M:%S", &t2);

        std::string t_out;
        std::string format("MM-DD-YYYY HH24:MI:SS");
        sql << "select to_char(t, :format) from (select :t as t from dual)",
            into(t_out), use(format), use(t2);

        assert(t_out == std::string(buf));
    }

    {
        // date and time - before year 2000
        std::time_t then = std::time(NULL) - 17*365*24*60*60;
        std::tm t1, t2;
        t2 = *std::localtime(&then);

        sql << "select t from (select :t as t from dual)",
             into(t1), use(t2);

        assert(memcmp(&t1, &t2, sizeof(std::tm)) == 0);
        
        // make sure the date is stored properly in Oracle
        char buf[25];
        strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M:%S", &t2);

        std::string t_out;
        std::string format("MM-DD-YYYY HH24:MI:SS");
        sql << "select to_char(t, :format) from (select :t as t from dual)",
            into(t_out), use(format), use(t2);

        assert(t_out == std::string(buf));
    }


//     {
//         std::time_t now = std::time(NULL);
//         std::time_t t;
// 
//         sql << "select t from (select :t as t from dual)",
//             into(t), use(now);
//         assert(t == now);
//     }

    std::cout << "test 2 passed" << std::endl;
}

// indicator test
void test3()
{
    Session sql(backEndName, connectString);

    {
        // test for eOK
        int i1 = 0, i2 = 12345678;
        eIndicator ind1;
        sql << "select i from (select :i as i from dual)",
            into(i1, ind1), use(i2);
        assert(ind1 == eOK);
    }

    {
        // test for eNull
        int i1 = 0, i2 = 12345678;
        eIndicator ind1 = eOK, ind2 = eNull;
        sql << "select i from (select :i as i from dual)",
            into(i1, ind1), use(i2, ind2);
        assert(ind1 == eNull);
    }

    {
        // test for truncation
        char buf1[6], buf2[100];
        eIndicator ind1;
        strcpy(buf2, "Hello, Oracle!");
        sql << "select s from (select :s as s from dual)",
            into(buf1, ind1), use(buf2);
        assert(ind1 == eTruncated);
        assert(strcmp(buf1, "Hello") == 0);
    }

    try
    {
        // test for overflow
        short s1;
        int i2 = 12345678;
        eIndicator ind1 = eOK;
        sql << "select s from (select :i as s from dual)",
            into(s1, ind1), use(i2);

        // exception expected
        assert(false);
    }
    catch (OracleSOCIError const &e)
    {
        // ORA-01455 happens here (overflow on conversion)
        assert(e.errNum_ == 1455);
    }

    std::cout << "test 3 passed" << std::endl;
}

// explicit calls test
void test4()
{
    Session sql(backEndName, connectString);

    Statement st(sql);
    st.alloc();
    int i = 0;
    st.exchange(into(i));
    st.prepare("select 7 from dual");
    st.defineAndBind();
    st.execute(1);
    assert(i == 7);

    std::cout << "test 4 passed" << std::endl;
}

// DDL + insert and retrieval tests
void test5()
{
    Session sql(backEndName, connectString);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    name varchar2(100)"
        ")";

    int count;
    sql << "select count(*) from some_table", into(count);
    assert(count == 0);

    int id;
    std::string name;
    Statement st1 = (sql.prepare <<
        "insert into some_table (id, name) values (:id, :name)",
        use(id), use(name));

    id = 1; name = "John"; st1.execute(1);
    id = 2; name = "Anna"; st1.execute(1);
    id = 3; name = "Mike"; st1.execute(1);

    sql.commit();

    sql << "select count(*) from some_table", into(count);
    assert(count == 3);

    Statement st2 = (sql.prepare <<
        "select id, name from some_table order by id",
        into(id), into(name));
    st2.execute();
    std::vector<int> ids;
    std::vector<std::string> names;
    while (st2.fetch())
    {
        ids.push_back(id);
        names.push_back(name);
    }

    assert(ids.size() == 3 && names.size() == 3);
    assert(ids[0] == 1 && names[0] == "John");
    assert(ids[1] == 2 && names[1] == "Anna");
    assert(ids[2] == 3 && names[2] == "Mike");

    sql << "drop table some_table";

    std::cout << "test 5 passed" << std::endl;
}

// DDL + BLOB test
void test6()
{
    Session sql(backEndName, connectString);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    img blob"
        ")";

    char buf[] = "abcdefghijklmnopqrstuvwxyz";

    sql << "insert into some_table (id, img) values (7, empty_blob())";

    {
        BLOB b(sql);

        OracleSessionBackEnd *sessionBackEnd
            = static_cast<OracleSessionBackEnd *>(sql.getBackEnd());

        OracleBLOBBackEnd *blobBackEnd
            = static_cast<OracleBLOBBackEnd *>(b.getBackEnd());

        OCILobDisableBuffering(sessionBackEnd->svchp_,
            sessionBackEnd->errhp_, blobBackEnd->lobp_);

        sql << "select img from some_table where id = 7", into(b);
        assert(b.getLen() == 0);

        // note: BLOB offsets start from 1
        b.write(1, buf, sizeof(buf));
        assert(b.getLen() == sizeof(buf));
        b.trim(10);
        assert(b.getLen() == 10);

        // append does not work (Oracle bug #886191 ?)
        //b.append(buf, sizeof(buf));
        //assert(b.getLen() == sizeof(buf) + 10);
        sql.commit();
    }

    {
        BLOB b(sql);
        sql << "select img from some_table where id = 7", into(b);
        //assert(b.getLen() == sizeof(buf) + 10);
        assert(b.getLen() == 10);
        char buf2[100];
        b.read(1, buf2, 10);
        assert(strncmp(buf2, "abcdefghij", 10) == 0);
    }

    sql << "drop table some_table";

    std::cout << "test 6 passed" << std::endl;
}

// rollback test
void test7()
{
    Session sql(backEndName, connectString);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    name varchar2(100)"
        ")";

    int count;
    sql << "select count(*) from some_table", into(count);
    assert(count == 0);

    int id;
    std::string name;
    Statement st1 = (sql.prepare <<
        "insert into some_table (id, name) values (:id, :name)",
        use(id), use(name));

    id = 1; name = "John"; st1.execute(1);
    id = 2; name = "Anna"; st1.execute(1);
    id = 3; name = "Mike"; st1.execute(1);

    sql.commit();

    sql << "select count(*) from some_table", into(count);
    assert(count == 3);

    id = 4; name = "Stan"; st1.execute(1);

    sql << "select count(*) from some_table", into(count);
    assert(count == 4);

    sql.rollback();

    sql << "select count(*) from some_table", into(count);
    assert(count == 3);

    sql << "delete from some_table";

    sql << "select count(*) from some_table", into(count);
    assert(count == 0);

    sql.rollback();

    sql << "select count(*) from some_table", into(count);
    assert(count == 3);

    sql << "drop table some_table";

    std::cout << "test 7 passed" << std::endl;
}

// nested statement test
// (the same syntax is used for output cursors in PL/SQL)
void test8()
{
    Session sql(backEndName, connectString);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    name varchar2(100)"
        ")";

    int id;
    std::string name;
    {
        Statement st1 = (sql.prepare <<
            "insert into some_table (id, name) values (:id, :name)",
            use(id), use(name));

        id = 1; name = "John"; st1.execute(1);
        id = 2; name = "Anna"; st1.execute(1);
        id = 3; name = "Mike"; st1.execute(1);
    }

    Statement stInner(sql);
    Statement stOuter = (sql.prepare <<
        "select cursor(select name from some_table order by id)"
        " from some_table where id = 1",
        into(stInner));
    stInner.exchange(into(name));
    stOuter.execute();
    stOuter.fetch();

    std::vector<std::string> names;
    while (stInner.fetch())    { names.push_back(name); }

    assert(names.size() == 3);
    assert(names[0] == "John");
    assert(names[1] == "Anna");
    assert(names[2] == "Mike");

    sql << "drop table some_table";

    std::cout << "test 8 passed" << std::endl;
}

// ROWID test
void test9()
{
    Session sql(backEndName, connectString);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    name varchar2(100)"
        ")";

    sql << "insert into some_table(id, name) values(7, \'John\')";

    RowID rid(sql);
    sql << "select rowid from some_table where id = 7", into(rid);

    int id;
    std::string name;
    sql << "select id, name from some_table where rowid = :rid",
        into(id), into(name), use(rid);

    assert(id == 7);
    assert(name == "John");

    sql << "drop table some_table";

    std::cout << "test 9 passed" << std::endl;
}

// Stored Procedures
void test10()
{
    {
        Session sql(backEndName, connectString);
        sql <<
            "create or replace procedure echo(output out varchar2,"
            "input in varchar2) as "
            "begin output := input; end;";

        std::string in("my message");
        std::string out;
        Statement st = (sql.prepare <<
            "begin echo(:output, :input); end;",
            use(out, "output"),
            use(in, "input"));
        st.execute(1);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;
            Procedure proc = (sql.prepare <<
                "echo(:output, :input)",
                use(out, "output"), use(in, "input"));
            proc.execute(1);
            assert(out == in);
        }
        sql << "drop procedure echo";
    }

    std::cout << "test 10 passed" << std::endl;
}

// Dynamic binding to Row objects
void test11()
{
    {
        Session sql(backEndName, connectString);

        try { sql << "drop table test11"; }
        catch (SOCIError const &) {} //ignore error if table doesn't exist

        sql << "create table test11(num_float numeric(7,2) NOT NULL,"
            << " name varchar2(20), when date, large numeric(10,0), "
            << " chr1 char(1), small numeric(4,0), vc varchar(10), "
            << " fl float, ntest char(1))";

        Row r;
        sql << "select * from test11", into(r);
        assert(r.indicator(0) ==  eNoData);

        for (int i = 1; i != 4; ++i)
        {
            std::ostringstream namestr;
            namestr << "name" << i;
            std::string name = namestr.str();

            std::time_t now = std::time(0);
            std::tm when = *gmtime(&now);
            when.tm_year = 104;
            when.tm_mon = 11;
            when.tm_mday = i;
            mktime(&when);

            double d = i + .25;
            unsigned long l = i + 100000;
            char c[] = "X";
            char v[] = "varchar";
            double f = i + .33;
            std::string s;
            eIndicator nullIndicator = eNull;

            sql << "insert into test11 values(:num_float, :name, :when, "
                << ":large, :chr1, :small, :vc, :fl, :ntest)",
                use(d,"num_float"),
                use(name, "name"),
                use(when, "when"),
                use(l, "large"),
                use(c, "chr1"),
                use(i, "small"),
                use(v, "vc"),
                use(f, "fl"),
                use(s, nullIndicator, "ntest");
        }

        // select into a Row
        {
            Row r;
            Statement st = (sql.prepare <<
                "select * from test11 order by num_float", into(r));
            st.execute(1);
            assert(r.size() == 9);

            assert(r.getProperties(0).getDataType() == eDouble);
            assert(r.getProperties(1).getDataType() == eString);
            assert(r.getProperties(2).getDataType() == eDate);
            assert(r.getProperties(3).getDataType() == eUnsignedLong);
            assert(r.getProperties(4).getDataType() == eString);
            assert(r.getProperties(5).getDataType() == eInteger);
            assert(r.getProperties("VC").getDataType() == eString);

            assert(r.getProperties(0).getName() == "NUM_FLOAT");
            assert(r.getProperties(1).getName() == "NAME");
            assert(r.getProperties(2).getName() == "WHEN");
            assert(r.getProperties(3).getName() == "LARGE");
            assert(r.getProperties(4).getName() == "CHR1");
            assert(r.getProperties(5).getName() == "SMALL");

            st.fetch();
            assert(r.get<double>(0) == 2.25);
            assert(r.get<std::string>(1) == "name2");
            assert(r.get<unsigned long>(3) == 100002);
            assert(r.get<std::string>(4) == "X");
            assert(r.get<int>(5) == 2);
            assert(r.get<std::string>(6) == "varchar");

            assert(r.get<double>("NUM_FLOAT") == 2.25);
            assert(r.get<int>("SMALL") == 2);
            assert(r.get<double>("FL") == 2.33);

            assert(r.get<std::string>("NTEST", "null") == "null");
            bool caught = false;
            try
            {
                std::string s = r.get<std::string>("NTEST");
            }
            catch(SOCIError&)
            {
                caught = true;
            }
            assert(caught);
    
            std::tm t = r.get<std::tm>(2);
            assert(t.tm_year == 104);
            assert(t.tm_mon == 11);
            assert(t.tm_mday == 2);

            assert(r.indicator(0) == eOK);

            // verify exception thrown on invalid get<>
            caught = false;
            try
            {
                r.get<std::string>(0);
            }
            catch (std::bad_cast const &)
            {
                caught = true;
            }
            assert(caught);
        }
    }

    std::cout << "test 11 passed" << std::endl;
}

// bind into user-defined objects
struct StringHolder
{
    StringHolder() {}
    StringHolder(const char* s) : s_(s) {}
    StringHolder(std::string s) : s_(s) {}
    std::string get() { return s_; }
private:
    std::string s_;
};

namespace SOCI
{
    template<> 
    struct TypeConversion<StringHolder>
    {
        typedef std::string base_type;
        static StringHolder from(std::string& s) { return StringHolder(s); }
        static std::string to(StringHolder& sh) { return sh.get(); }
    };
}

void test12()
{
    Session sql(backEndName, connectString);
    {
        try
        {
            sql << "drop table test12";
        }
        catch (SOCIError const &) {} // ignore error if table doesn't exist
        sql << "create table test12(name varchar2(20))";

        StringHolder in("my string");
        sql << "insert into test12(name) values(:name)", use(in);

        StringHolder out;
        sql << "select name from test12", into(out);
        assert(out.get() == "my string");

        Row r;
        sql << "select * from test12", into(r);
        StringHolder dynamicOut = r.get<StringHolder>(0);
        assert(dynamicOut.get() == "my string");
    }

    // test procedure with user-defined type as in-out parameter    
    {
        sql << "create or replace procedure doubleString(s in out varchar2)"
            " as begin s := s || s; end;";

        StringHolder sh("test");
  
        Procedure proc = (sql.prepare << "doubleString(:s)", use(sh));
        proc.execute(1);
        assert(sh.get() == "testtest");

        sql << "drop procedure doubleString";
    }

    // test procedure which returns null
    {
         sql << "create or replace procedure returnsNull(s in out varchar2)"
            " as begin s := NULL; end;";

         StringHolder sh;           
         eIndicator ind = eOK;
         Procedure proc = (sql.prepare << "returnsNull(:s)", use(sh, ind));
         proc.execute(1);
         assert(ind == eNull);

        sql << "drop procedure returnsNull";
    }

    std::cout << "test 12 passed" << std::endl;
}

// test multiple use types of the same underlying type
void test13()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test13"; } catch (SOCIError const &) {} // ignore

    sql << "create table test13 ("
        "id number(10) not null,"
        "idtest number(10) not null,"
        "name varchar2(100),"
        "nametest varchar2(100))";

    int id_in = 1;
    int idtest_in = 2;

    std::string name_in("my name");
    std::string nametest_in("my name test");

    Statement st1 = (sql.prepare
                     << "insert into test13(id,idtest,name,nametest)"
                     << " values (:id,:idtest,:name,:nametest)",
                     use(id_in,"id"),
                     use(idtest_in,"idtest"),
                     use(name_in,"name"),
                     use(nametest_in,"nametest"));
    st1.execute(1);

    int id_out;
    int idtest_out;
    std::string name_out;
    std::string nametest_out;

    sql << "select * from test13",
        into(id_out),
        into(idtest_out),
        into(name_out),
        into(nametest_out);
    assert(id_in == id_out);
    assert(idtest_in == idtest_out);
    assert(name_in == name_out);
    assert(nametest_in == nametest_out);
    std::cout << "test 13 passed" << std::endl;
}

// test dbtype CHAR
void test14()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test14"; } catch (SOCIError const &) {} // ignore

    sql << "create table test14(chr1 char(1))";

    char c_in = 'Z';
    sql << "insert into test14(chr1) values(:C)", use(c_in);

    char c_out = ' ';
    sql << "select chr1 from test14", into(c_out);
    assert(c_out == 'Z');

    std::cout << "test 14 passed" << std::endl;
}

// test bulk insert features
void test15()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test15"; } catch (SOCIError const &) {} // ignore

    sql << "create table test15 (id number(5), code number(2))";

    // verify exception if thrown if vectors of unequal size are passed in
    {
        std::vector<int> ids;
        ids.push_back(1);
        ids.push_back(2);
        std::vector<int> codes;
        codes.push_back(1);
        std::string error;

        try
        {
            sql << "insert into test15(id,code) values(:id,:code)",
                use(ids), use(codes);
        }
        catch (SOCIError const &e)
        {
            error = e.what();
        }
        assert(error.find("Bind variable size mismatch")
            != std::string::npos);

        try
        {
            sql << "select from test15", into(ids), into(codes);
        }
        catch (std::exception const &e)
        {
            error = e.what();
        }
        assert(error.find("Bind variable size mismatch")
            != std::string::npos);
    }

    // verify partial insert occurs when one of the records is bad
    {
        std::vector<int> ids;
        ids.push_back(100);
        ids.push_back(1000000); // too big for column

        std::string error;
        try
        {
            sql << "insert into test15 (id) values(:id)", use(ids, "id");
        }
        catch (SOCIError const &e)
        {
            error = e.what();
            //TODO e could be made to tell which row(s) failed
        }
        sql.commit();
        assert(error.find("ORA-01438") != std::string::npos);
        int count(7);
        sql << "select count(*) from test15", into(count);
        assert(count == 1);
        sql << "delete from test15";
    }

    // test insert
    {
        std::vector<int> ids;
        for (int i = 0; i != 3; ++i)
        {
            ids.push_back(i+10);
        }

        Statement st = (sql.prepare << "insert into test15(id) values(:id)",
                            use(ids));
        st.execute(1);
        int count;
        sql << "select count(*) from test15", into(count);
        assert(count == 3);
    }

    //verify an exception is thrown if into vector is zero length
    {
        std::vector<int> ids;
        bool caught(false);
        try
        {
            sql << "select id from test15", into(ids);
        }
        catch (SOCIError const &)
        {
            caught = true;
        }
        assert(caught);
    }

    // verify an exception is thrown if use vector is zero length
    {
        std::vector<int> ids;
        bool caught(false);
        try
        {
            sql << "insert into test15(id) values(:id)", use(ids);
        }
        catch (SOCIError const &)
        {
            caught = true;
        }
        assert(caught);
    }

// Note: This test was removed, because eNoData is probably meaningless
// when used in vectors. The repeated fetch() idiom with while loop
// works based on the assumption that the false return value from
// fetch means "no more data". In such a scheme, the only possible
// indicators in the vector are eOK and eNull and the user need not
// check the vector for any other possibilities.
// (eTruncated is ruled out, because we do not support vector<char*>.)
// For consistency, the execute(1) should provide the same semantics.
// Please see the test case just below for the valid part.
// 
//     // test eNoData indicator
//     {
//         std::vector<eIndicator> inds(3);
//         std::vector<int> ids_out(3);
//         Statement st = (sql.prepare << "select id from test15 where 1=0",
//                         into(ids_out, inds));

//         assert(!st.execute(1));
//         assert(ids_out.size() == 0);
//         assert(inds.size() == 3 && inds[0] == eNoData
//             && inds[1] == eNoData && inds[2] == eNoData);
//     }

    // test "no data" condition
    {
        std::vector<eIndicator> inds(3);
        std::vector<int> ids_out(3);
        Statement st = (sql.prepare << "select id from test15 where 1=0",
                        into(ids_out, inds));

        // false return value means "no data"
        assert(!st.execute(1));

        // that's it - nothing else is guaranteed
        // and nothing else is to be tested here
    }

    // test NULL indicators
    {
        std::vector<int> ids(3);
        sql << "select id from test15", into(ids);

        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);
        inds_in.push_back(eOK);

        std::vector<int> new_codes;
        new_codes.push_back(10);
        new_codes.push_back(11);
        new_codes.push_back(10);

        sql << "update test15 set code = :code where id = :id",
                use(new_codes, inds_in), use(ids);

        std::vector<eIndicator> inds_out(3);
        std::vector<int> codes(3);

        sql << "select code from test15", into(codes, inds_out);
        assert(codes.size() == 3 && inds_out.size() == 3);
        assert(codes[0] == 10 && codes[2] == 10);
        assert(inds_out[0] == eOK && inds_out[1] == eNull
            && inds_out[2] == eOK);
    }

    // verify an exception is thrown if null is selected
    //  and no indicator was provided
    {
        std::string msg;
        std::vector<int> intos(3);
        try
        {
            sql << "select code from test15", into(intos);
        }
        catch (SOCIError const &e)
        {
            msg = e.what();
        }
        assert(msg == "Null value fetched and no indicator defined." );
    }

    // test basic select
    {
        const size_t sz = 3;
        std::vector<eIndicator> inds(sz);
        std::vector<int> ids_out(sz);
        Statement st = (sql.prepare << "select id from test15",
            into(ids_out, inds));
        assert(st.execute(1));
        assert(ids_out.size() == sz);
        assert(ids_out[0] == 10);
        assert(ids_out[2] == 12);
        assert(inds.size() == 3 && inds[0] == eOK
            && inds[1] == eOK && inds[2] == eOK);
    }

    // verify execute(0)
    {
        std::vector<int> ids_out(2);
        Statement st = (sql.prepare << "select id from test15",
            into(ids_out));

        st.execute(0);
        assert(ids_out.size() == 2);
        assert(st.fetch());
        assert(ids_out.size() == 2 && ids_out[0] == 10 && ids_out[1] == 11);
        assert(st.fetch());
        assert(ids_out.size() == 1 && ids_out[0] == 12);
        assert(!st.fetch());
    }

    // verify resizing happens if vector is larger
    // than number of rows returned
    {
        std::vector<int> ids_out(4); // one too many
        Statement st2 = (sql.prepare << "select id from test15",
            into(ids_out));
        assert(st2.execute(1));
        assert(ids_out.size() == 3);
        assert(ids_out[0] == 10);
        assert(ids_out[2] == 12);
    }

    // verify resizing happens properly during fetch()
    {
        std::vector<int> more;
        more.push_back(13);
        more.push_back(14);
        sql << "insert into test15(id) values(:id)", use(more);

        std::vector<int> ids(2);
        Statement st3 = (sql.prepare << "select id from test15", into(ids));
        assert(st3.execute(1));
        assert(ids[0] == 10);
        assert(ids[1] == 11);

        assert(st3.fetch());
        assert(ids[0] == 12);
        assert(ids[1] == 13);

        assert(st3.fetch());
        assert(ids.size() == 1);
        assert(ids[0] == 14);

        assert(!st3.fetch());
    }

    std::cout << "test 15 passed" << std::endl;
}

//test bulk operations for std::string
void test16()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test16"; } catch (SOCIError const &) {} // ignore

    sql << "create table test16(name varchar2(10), code varchar2(3))";

    // verify an exception is thrown if string size exceeds column size
    {
        std::vector<std::string> codes;
        codes.push_back("abc");
        codes.push_back("defg"); // too big for column

        std::string error;
        try
        {
            sql << "insert into test16(code) values(:code)", use(codes);
            assert(false);
        }
        catch (SOCIError const &e)
        {
            error = e.what();
        }
        assert(error.find("ORA-12899") != std::string::npos);
        int count(0);
        sql << "select count(*) from test16", into(count);
        assert(count == 1);
        sql << "delete from test16";
    }

    std::vector<std::string> names;
    std::vector<std::string> codes;
    for (int i = 0; i != 3; ++i)
    {
        std::ostringstream nStr;
        nStr << "name" << i;
        if (i==1)
            nStr << "extra";
        names.push_back(nStr.str());

        std::ostringstream cStr;
        cStr << i*100;
        codes.push_back(cStr.str());
    }

    Statement st = (sql.prepare
        << "insert into test16(name,code) values(:name, :code)",
        use(names), use(codes));
    st.execute(1);

    // test select, multiple string columns
    {
        std::vector<std::string> names_out(2);
        std::vector<std::string> codes_out(2);
        Statement st = (sql.prepare << "select name, code from test16",
            into(names_out), into(codes_out));
        st.execute(1);
        assert(names_out.size() == 2);
        assert(names_out[0] == "name0");
        assert(names_out[1] == "name1extra");
        assert(codes_out[0] == "0");
        assert(codes_out[1] == "100");
        st.fetch();
        assert(names_out.size()==1 && names_out[0] == "name2");
        assert(codes_out.size()==1 && codes_out[0] == "200");
    }

    // test simple syntax
    {
        std::vector<std::string> simple;
        simple.push_back("simple1");
        simple.push_back("simple2");
        sql << "insert into test16(name) values(:names)",
            use(simple, "names");

        std::vector<std::string> simple_out(2);
        sql << "select name from test16 where name like 'simple%'",
            into(simple_out);
        assert(simple_out.size()==2 && simple_out[0] == "simple1"
            && simple_out[1] == "simple2");
    }

    // test indicators
    {
        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);
        inds_in.push_back(eOK);

        std::vector<std::string> names_in;
        names_in.push_back("indtest1");
        names_in.push_back("indtest2");
        names_in.push_back("indtest3");

        std::vector<std::string> codes;
        codes.push_back("99");
        codes.push_back("99");
        codes.push_back("99");
        sql << "insert into test16(name,code) values (:name, :code)",
            use(names_in, inds_in), use(codes);
        sql.commit();

        std::vector<eIndicator> inds_out(3);
        inds_out[0] = eNoData;
        inds_out[1] = eNoData;
        inds_out[2] = eNoData;
        std::vector<std::string> names_out(3);

        sql << "select name from test16 where code = :code",
            use(codes[0]), into(names_out, inds_out);

        assert(names_out[0] == "indtest1" && names_out[2] == "indtest3");
        assert(inds_out[0] == eOK && inds_out[1] == eNull
            && inds_out[2] == eOK);
        assert(names_out[0] == "indtest1" && names_out[2] == "indtest3");
    }

    //verify bad sql statement causes exception
    {
        std::vector<std::string> names;
        std::string msg;
        try
        {
            sql << "select bogus from test16\n", into(names);
        }
        catch (SOCIError const &e)
        {
            msg = e.what();
        }
        assert(msg.find("invalid identifier") != std::string::npos);
    }
    std::cout << "test 16 passed" << std::endl;
}

// test bulk operations for unsigned long and double
void test17()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test17"; } catch (SOCIError const &) {} // ignore

    sql << "create table test17 (nums number(10), amts number(8,2))";

    std::vector<unsigned long> nums;
    nums.push_back(100 * 1000);
    nums.push_back(200 * 1000);
    nums.push_back(300 * 1000);

    std::vector<double> amts;
    amts.push_back(1.11);
    amts.push_back(1.12);
    amts.push_back(1.13);

    sql << "insert into test17(nums, amts) values(:nums, :amts)",
        use(nums, "nums"), use(amts, "amts");
    int count(0);

    sql << "select count(*) from test17", into(count);
    assert(count == 3);

    std::vector<unsigned long> nums_out(2);
    std::vector<double> amts_out(2);

    sql << "select nums, amts from test17", into(nums_out), into(amts_out);
    assert(nums_out.size()==2 && nums_out[0] == 100*1000
        && nums_out[1] == 200*1000);
    assert(amts_out.size()==2 && amts_out[0] == 1.11 && amts_out[1] == 1.12);

    // test indicators
    {
        std::vector<eIndicator> indsd_in;
        indsd_in.push_back(eOK);
        indsd_in.push_back(eNull);

        std::vector<double> vd;
        vd.push_back(99.99);
        vd.push_back(100.1);

        std::vector<eIndicator> indsl_in;
        indsl_in.push_back(eNull);
        indsl_in.push_back(eOK);

        std::vector<unsigned long> vl;
        vl.push_back(9990);
        vl.push_back(1001);

        sql << "insert into test17(nums, amts) values(:num, :amt)",
            use(vl, indsl_in), use(vd, indsd_in);

        std::vector<double> d_out(2);
        std::vector<unsigned long> l_out(2);
        std::vector<eIndicator> indd_out(2);
        std::vector<eIndicator> indl_out(2);

        sql << "select nums, amts from test17 "
            "where nums is null or amts is null",
            into(l_out, indl_out), into(d_out, indd_out);

        assert(l_out.size() == 2);

        assert(l_out[1] == vl[1] && indl_out[1] == eOK);
        assert(indl_out[0] == eNull);

        assert(indd_out[1] == eNull);
        assert(d_out[0] < 100 && d_out[0] > 99.98);
        assert(indd_out[0] == eOK);
    }

    std::cout << "test 17 passed" << std::endl;
}

// test bulk operations for std::tm
void test18()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test18"; } catch (SOCIError const &) {} // ignore

    sql << "create table test18 (d1 date, d2 date)";

    std::vector<std::tm> times;
    for (int i = 0; i != 4; ++i)
    {
        std::time_t now = std::time(0);
        std::tm when = *gmtime(&now);
        when.tm_year = 104;
        when.tm_mon = 11;
        when.tm_mday = i+10;
        mktime(&when);
        times.push_back(when);
    }

    sql << "insert into test18(d1) values(:times)", use(times);

    int count(0);
    sql << "select count(*) from test18", into(count);
    assert(count == 4);

    std::vector<std::tm> times_out(3);
    sql << "select d1 from test18", into(times_out);

    assert(times_out.size() == 3);
    for (size_t i = 0; i != times_out.size(); ++i)
    {
        assert(mktime(&times_out[i]) == mktime(&times[i]));
        std::tm t = times_out[i];
        assert(t.tm_year == 104);
        assert(t.tm_mon == 11);
        assert(t.tm_mday == static_cast<int>(i+10));
    }

    std::vector<std::tm> times_in;
    // test indicators
    {
        for (int i = 0; i != 2; ++i)
        {
            std::time_t now = std::time(0);
            std::tm when = *gmtime(&now);
            when.tm_year = 125;
            when.tm_mon = 2;
            when.tm_mday = i + 1;
            mktime(&when);
            times_in.push_back(when);
        }
        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);
        sql << "insert into test18 (d1) values (:d1)",
            use(times_in, inds_in, "d1");
        sql.commit();
        std::vector<std::tm> times_out(2);
        std::vector<eIndicator> inds_out(2);

        sql << "select d1 from test18 where d1 > sysdate or d1 is null",
            into(times_out, inds_out);
        assert(times_out.size() == 2);
        assert(memcmp(&times_out[0], &times_in[0], sizeof(std::tm)) == 0);
        assert(inds_out[0] == eOK);
        assert(inds_out[1] == eNull);
    }

    // test resizing
    {
        std::vector<std::tm> out(2);
        Statement st = (sql.prepare
            << "select d1 from test18 where d1 is not null", into(out));

        assert(st.execute(1));
        assert(out.size() == 2);

        assert(mktime(&out[0]) == mktime(&times[0]));
        assert(mktime(&out[1]) == mktime(&times[1]));

        assert(st.fetch());
        assert(out.size() == 2);
        assert(mktime(&out[0]) == mktime(&times[2]));
        assert(mktime(&out[1]) == mktime(&times[3]));

        assert(st.fetch());
        assert(out.size() == 1);
        assert(mktime(&out[0]) == mktime(&times_in[0]));

        assert(!st.fetch());
    }

    std::cout << "test 18 passed" << std::endl;
}

// test bulk operations for std::time_t
void test19()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test19"; } catch (SOCIError const &) {} // ignore

    sql << "create table test19 (d1 date)";

// The following tests require the existence of TypeConversion<std::time_t>
// which cannot be used in all environments (where std::time_t is an alias
// to int). You can try it out by uncommenting the test and the specialization
// of TypeConversion in soci.h.
// 
//     time_t t1 = std::time(0) + 60*60*24;
//     time_t t2 = t1 + 60*60*24*2;
//     time_t t3 = t1 + 60*60*24*4;
//     std::vector<time_t> times_in1;
//     {
//         times_in1.push_back(t1);
//         times_in1.push_back(t2);
//         times_in1.push_back(t3);
// 
//         sql << "insert into test19(d1) values(:d1)", use(times_in1);
// 
//         int count(0);
//         sql << "select count(*) from test19", into(count);
//         assert(count == 3);
//     }
// 
//     // test resizing from exec
//     {
//         std::vector<time_t> times_out(3); // one too many
//         sql << "select d1 from test19", into(times_out);
// 
//         assert(times_out.size() == 3);
//         assert(times_out[1] == t2);
//     }
// 
//     //test indicators
//     std::vector<std::time_t> times_in2;
//     {
//         time_t today = time(0);
//         time_t yesterday = today - 60*60*24;
//         time_t dayBefore = yesterday - 60*60*24;
//         times_in2.push_back(yesterday);
//         times_in2.push_back(dayBefore);
// 
//         std::vector<eIndicator> inds_in;
//         inds_in.push_back(eOK);
//         inds_in.push_back(eNull);
//         sql << "insert into test19 (d1) values(:d1)",
//             use(times_in2, inds_in, "d1");
//         assert(times_in2.size() == 2);
//         assert(times_in2[0] == yesterday);
//         assert(times_in2[1] == dayBefore);
// 
//         std::vector<std::time_t> times_out(2);
//         std::vector<eIndicator> inds_out(2);
//         sql << "select d1 from test19 where d1 < sysdate or d1 is null",
//             into(times_out, inds_out);
// 
//         assert(times_out.size() == 2);
//         assert(times_out[0] == times_in2[0]);
//         assert(inds_out[0] == eOK);
//         assert(inds_out[1] == eNull);
//     }
// 
//     //test resizing
//     {
//         std::vector<std::time_t> times_in3;
//         time_t now = time(0);
//         times_in3.push_back(now);
// 
//         sql << "insert into test19 (d1) values(:d1)", use(times_in3);
// 
//         std::vector<std::time_t> times_out(2);
//         Statement st = (sql.prepare
//             << "select d1 from test19 where d1 is not null",
//             into(times_out));
//         assert(st.execute(1));
//         assert(times_out.size() == 2 && times_out[0] == times_in1[0]
//             && times_out[1] == times_in1[1]);
// 
//         assert(st.fetch());
//         assert(times_out.size() == 2);
//         assert(times_out[0] == times_in1[2]);
//         assert(times_out[1] == times_in2[0]);
// 
//         assert(st.fetch());
//         assert(times_out.size() == 1 && times_out[0] == times_in3[0]);
//         assert(!st.fetch());
//     }

    std::cout << "test 19 passed" << std::endl;
}

// test bulk operations for char
void test20()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test20"; } catch (SOCIError const &) {} // ignore

    sql << "create table test20(code char(1))";

    std::vector<char> in;
    in.push_back('A');
    in.push_back('B');

    sql << "insert into test20 (code) values(:code)", use(in);

    int count(0);
    sql << "select count(*) from test20", into(count);
    assert(count == 2);

    std::vector<char> out(2);
    sql << "select code from test20", into(out);
    assert(out.size() == 2 && out[0] == 'A' && out[1] == 'B');

    // test indicators
    {
        std::vector<char> in2;
        in2.push_back('C');
        in2.push_back('D');

        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);

        sql << "insert into test20 (code) values (:code)",
            use(in2, inds_in, "code");

        std::vector<char> out2(4);
        std::vector<eIndicator> inds_out(4);

        sql << "select code from test20", into(out2, inds_out);
        assert(out2[2] == 'C' && inds_out[2] == eOK);
        assert(inds_out[3] == eNull);

    }
    std::cout << "test 20 passed" << std::endl;
}

// test bulk operations for short
void test21()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test21"; } catch (SOCIError const &) {} // ignore

    sql << "create table test21(id number(2))";

    std::vector<short> in;
    in.push_back(1);
    in.push_back(2);

    sql << "insert into test21 (id) values(:id)", use(in);

    int count(0);
    sql << "select count(*) from test21", into(count);
    assert(count == 2);

    std::vector<short> out(2);
    sql << "select id from test21", into(out);
    assert(out.size() == 2 && out[0] == 1 && out[1] == 2);

    // test indicators
    {
        std::vector<short> in2;
        in2.push_back(3);
        in2.push_back(4);

        assert(in2[0] == 3);
        assert(in2[1] == 4);

        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);

        sql << "insert into test21 (id) values (:id)",
            use(in2, inds_in, "id");

        std::vector<short> out2(4);
        std::vector<eIndicator> inds_out(4);

        sql << "select id from test21", into(out2, inds_out);

        assert(in2[0] == 3);

        assert(out2[2] == 3);
        assert(inds_out[2] == eOK);
        assert(inds_out[3] == eNull);
    }

    std::cout << "test 21 passed" << std::endl;
}

// more tests for bulk fetch
void test22()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table test22"; } catch (SOCIError const &) {} // ignore

    sql << "create table test22 (id number(2))";

    std::vector<int> in;
    for (int i = 1; i <= 10; ++i)
    {
        in.push_back(i);
    }

    sql << "insert into test22 (id) values(:id)", use(in);

    int count(0);
    sql << "select count(*) from test22", into(count);
    assert(count == 10);

    // verify that the exception is thrown when trying to resize
    // the output vector to the size that is bigger than that
    // at the time of binding
    {
        std::vector<int> out(4);
        Statement st = (sql.prepare <<
            "select id from test22", into(out));

        st.execute();

        st.fetch();
        assert(out.size() == 4);
        assert(out[0] == 1);
        assert(out[1] == 2);
        assert(out[2] == 3);
        assert(out[3] == 4);
        out.resize(5); // this should be detected as error
        try
        {
            st.fetch();
            assert(false); // should never reach here
        }
        catch (SOCIError const &e)
        {
            assert(std::string(e.what()) ==
                "Increasing the size of the output vector is not supported.");
        }
    }

    // on the other hand, downsizing is OK
    {
        std::vector<int> out(4);
        Statement st = (sql.prepare <<
            "select id from test22", into(out));

        st.execute();

        st.fetch();
        assert(out.size() == 4);
        assert(out[0] == 1);
        assert(out[1] == 2);
        assert(out[2] == 3);
        assert(out[3] == 4);
        out.resize(3); // ok
        st.fetch();
        assert(out.size() == 3);
        assert(out[0] == 5);
        assert(out[1] == 6);
        assert(out[2] == 7);
        out.resize(4); // ok, not bigger than initially
        st.fetch();
        assert(out.size() == 3); // downsized because of end of data
        assert(out[0] == 8);
        assert(out[1] == 9);
        assert(out[2] == 10);
        assert(st.fetch() == false); // end of data
    }

    std::cout << "test 22 passed" << std::endl;
}


struct Person
{
    int id;
    std::string firstName;
    StringHolder lastName; //test mapping of TypeConversion-based types
    std::string gender; 
};

// Object-Relational Mapping
// Note: Use the Values class as shown below in TypeConversions
// to achieve object relational mapping.  The Values class should
// not be used directly in any other fashion.
namespace SOCI
{
    // name-based conversion
    template<> struct TypeConversion<Person>
    {
        typedef Values base_type;

        static Person from(Values const &v)
        {
            Person p;
            p.id = v.get<int>("ID");
            p.firstName = v.get<std::string>("FIRST_NAME");
            p.lastName = v.get<StringHolder>("LAST_NAME");
            p.gender = v.get<std::string>("GENDER", "unknown");
            return p;
        }

        static Values to(Person &p)
        {
            Values v;
            v.set("ID", p.id);
            v.set("FIRST_NAME", p.firstName);
            v.set("LAST_NAME", p.lastName);
            v.set("GENDER", p.gender, p.gender.empty() ? eNull : eOK);
            return v;
        }
    };
}

void test23()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table person"; }
        catch (SOCIError const &) {} //ignore error if table doesn't exist

    sql << "create table person(id numeric(5,0) NOT NULL,"
        << " last_name varchar2(20), first_name varchar2(20), "
           " gender varchar2(10))";

    Person p;
    p.id = 1;
    p.lastName = "Smith";
    p.firstName = "Pat";
    sql << "insert into person(id, first_name, last_name, gender) "
        << "values(:ID, :FIRST_NAME, :LAST_NAME, :GENDER)", use(p);
    
    // p should be unchanged
    assert(p.id == 1);
    assert(p.firstName == "Pat");
    assert(p.lastName.get() == "Smith");

    Person p1;
    sql << "select * from person", into(p1);
    assert(p1.id == 1);
    assert(p1.firstName + p1.lastName.get() == "PatSmith");
    assert(p1.gender == "unknown");

    p.firstName = "Patricia";
    sql << "update person set first_name = :FIRST_NAME "
           "where id = :ID", use(p);

    // p should be unchanged
    assert(p.id == 1);
    assert(p.firstName == "Patricia");
    assert(p.lastName.get() == "Smith");
    //TODO p.gender is now "unknown" because of the mapping, not ""
    //Is this ok? assert(p.gender == ""); 

    Person p2;
    sql << "select * from person", into(p2);
    assert(p2.id == 1);
    assert(p2.firstName + p2.lastName.get() == "PatriciaSmith");

    // test with stored procedure
    {
        sql << "create or replace procedure getNewID(id in out number)"
               " as begin id := id * 100; end;"; 
        Person p;
        p.id = 1;
        p.firstName = "Pat";
        p.lastName = "Smith";
        Procedure proc = (sql.prepare << "getNewID(:ID)", use(p));
        proc.execute(1);
        assert(p.id == 100);
        assert(p.firstName == "Pat");
        assert(p.lastName.get() == "Smith");

        sql << "drop procedure getNewID";
    }

    // test with stored procedure which returns null
    {
        sql << "create or replace procedure returnsNull(s in out varchar2)"
               " as begin s := NULL; end;"; 
        
        std::string msg;
        Person p;
        try
        {
            Procedure proc = (sql.prepare << "returnsNull(:FIRST_NAME)", 
                                use(p));
            proc.execute(1);
        }
        catch (SOCIError& e)
        {
            msg = e.what();
        }

        assert(msg == "Column FIRST_NAME contains NULL value and"
                      " no default was provided");

        Procedure proc = (sql.prepare << "returnsNull(:GENDER)", 
                                use(p));
        proc.execute(1);
        assert(p.gender == "unknown");        

        sql << "drop procedure returnsNull";
    }
    std::cout << "test 23 passed" << std::endl;
}

// Experimental support for position based O/R Mapping

// additional type for position-based test
struct Person2
{
    int id;
    std::string firstName;
    std::string lastName;
    std::string gender;
};

// additional type for stream-like test
struct Person3 : Person2 {};

namespace SOCI
{
    // position-based conversion
    template<> struct TypeConversion<Person2>
    {
        typedef Values base_type;

        static Person2 from(Values const &v)
        {
            Person2 p;
            p.id = v.get<int>(0);
            p.firstName = v.get<std::string>(1);
            p.lastName = v.get<std::string>(2);
            p.gender = v.get<std::string>(3, "whoknows");
            return p;
        }

        // What about the "to" part? Does it make any sense to have it?
    };

    // stream-like conversion
    template<> struct TypeConversion<Person3>
    {
        typedef Values base_type;

        static Person3 from(Values const &v)
        {
            Person3 p;
            v >> p.id >> p.firstName >> p.lastName >> p.gender;
            return p;
        }

        // TODO: The "to" part is certainly needed.
    };
};

void test24()
{
    Session sql(backEndName, connectString);

    try { sql << "drop table person"; }
        catch (SOCIError const &) {} //ignore error if table doesn't exist

    sql << "create table person(id numeric(5,0) NOT NULL,"
        << " last_name varchar2(20), first_name varchar2(20), "
           " gender varchar2(10))";
    
    Person p;
    p.id = 1;
    p.lastName = "Smith";
    p.firstName = "Patricia";
    sql << "insert into person(id, first_name, last_name, gender) "
        << "values(:ID, :FIRST_NAME, :LAST_NAME, :GENDER)", use(p);

    //  test position-based conversion
    Person2 p3;
    sql << "select id, first_name, last_name, gender from person", into(p3);
    assert(p3.id == 1);
    assert(p3.firstName + p3.lastName == "PatriciaSmith");
    assert(p3.gender == "whoknows");

    sql << "update person set gender = 'F' where id = 1";

    // additional test for stream-like conversion
    Person3 p4;
    sql << "select id, first_name, last_name, gender from person", into(p4);
    assert(p4.id == 1);
    assert(p4.firstName + p4.lastName == "PatriciaSmith");
    assert(p4.gender == "F");

    std::cout << "test 24 passed" << std::endl;
}

// additional test for statement preparation with indicators (non-bulk)
void test25()
{
    Session sql(backEndName, connectString);

    try{ sql << "drop table test25"; }
    catch (SOCIError const &) {} // ignore error if table doesn't exist

    sql << "create table test25(id numeric(2))";

    sql << "insert into test25(id) values(1)";
    sql << "insert into test25(id) values(NULL)";
    sql << "insert into test25(id) values(NULL)";
    sql << "insert into test25(id) values(2)";

    int id(7);
    eIndicator ind(eNoData);

    Statement st = (sql.prepare << "select id from test25", into(id, ind));

    st.execute();
    assert(st.fetch());
    assert(ind == eOK);
    assert(id  == 1);
    assert(st.fetch());
    assert(ind == eNull);
    assert(st.fetch());
    assert(ind == eNull);
    assert(st.fetch());
    assert(ind == eOK);
    assert(id  == 2);
    assert(st.fetch() == false); // end of rowset expected

    std::cout << "test 25 passed" << std::endl;
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
            << " \'service=orcl user=scott password=tiger\'\n";
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
        test9();
        test10(); 
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

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
