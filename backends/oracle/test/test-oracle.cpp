//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-oracle.h"
#include "common-tests.h"
#include <iostream>
#include <string>
#include <cassert>
#include <ctime>

using namespace SOCI;
using namespace SOCI::tests;

std::string connectString;
BackEndFactory const &backEnd = oracle;

// Extra tests for date/time
void test1()
{
    Session sql(backEnd, connectString);

    {
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

    std::cout << "test 1 passed" << std::endl;
}

// explicit calls test
void test2()
{
    Session sql(backEnd, connectString);

    Statement st(sql);
    st.alloc();
    int i = 0;
    st.exchange(into(i));
    st.prepare("select 7 from dual");
    st.defineAndBind();
    st.execute(1);
    assert(i == 7);

    std::cout << "test 2 passed" << std::endl;
}

// DDL + BLOB test

struct BlobTableCreator : public TableCreatorBase
{
    BlobTableCreator(Session& session)
    : TableCreatorBase(session)
    {
        session <<
            "create table soci_test ("
            "    id number(10) not null,"
            "    img blob"
            ")";
    }
};

void test3()
{
    Session sql(backEnd, connectString);
    
    BlobTableCreator tableCreator(sql);

    char buf[] = "abcdefghijklmnopqrstuvwxyz";
    sql << "insert into soci_test (id, img) values (7, empty_blob())";

    {
        BLOB b(sql);

        OracleSessionBackEnd *sessionBackEnd
            = static_cast<OracleSessionBackEnd *>(sql.getBackEnd());

        OracleBLOBBackEnd *blobBackEnd
            = static_cast<OracleBLOBBackEnd *>(b.getBackEnd());

        OCILobDisableBuffering(sessionBackEnd->svchp_,
            sessionBackEnd->errhp_, blobBackEnd->lobp_);

        sql << "select img from soci_test where id = 7", into(b);
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
        sql << "select img from soci_test where id = 7", into(b);
        //assert(b.getLen() == sizeof(buf) + 10);
        assert(b.getLen() == 10);
        char buf2[100];
        b.read(1, buf2, 10);
        assert(strncmp(buf2, "abcdefghij", 10) == 0);
    }

    std::cout << "test 3 passed" << std::endl;
}

// nested statement test
// (the same syntax is used for output cursors in PL/SQL)

struct BasicTableCreator : public TableCreatorBase
{
    BasicTableCreator(Session& session) 
        : TableCreatorBase(session)
    {
        session << 
                    "create table soci_test ("
                    "    id number(5) not null,"
                    "    name varchar2(100),"
                    "    code number(5)"
                    ")";
    }
};

void test4()
{
    Session sql(backEnd, connectString);
    BasicTableCreator tableCreator(sql);

    int id;
    std::string name;
    {
        Statement st1 = (sql.prepare <<
            "insert into soci_test (id, name) values (:id, :name)",
            use(id), use(name));

        id = 1; name = "John"; st1.execute(1);
        id = 2; name = "Anna"; st1.execute(1);
        id = 3; name = "Mike"; st1.execute(1);
    }

    Statement stInner(sql);
    Statement stOuter = (sql.prepare <<
        "select cursor(select name from soci_test order by id)"
        " from soci_test where id = 1",
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

    std::cout << "test 4 passed" << std::endl;
}

// ROWID test
void test5()
{
    Session sql(backEnd, connectString);
    BasicTableCreator tableCreator(sql);

    sql << "insert into soci_test(id, name) values(7, \'John\')";

    RowID rid(sql);
    sql << "select rowid from soci_test where id = 7", into(rid);

    int id;
    std::string name;
    sql << "select id, name from soci_test where rowid = :rid",
        into(id), into(name), use(rid);

    assert(id == 7);
    assert(name == "John");

    std::cout << "test 5 passed" << std::endl;
}

// Stored Procedures
struct ProcedureCreator : ProcedureCreatorBase
{
    ProcedureCreator(Session& session) 
        : ProcedureCreatorBase(session)
    {
        session << 
             "create or replace procedure soci_test(output out varchar2,"
             "input in varchar2) as "
             "begin output := input; end;";
    }
};
  
void test6()
{
    {
        Session sql(backEnd, connectString);
        ProcedureCreator procedureCreator(sql);

        std::string in("my message");
        std::string out;
        Statement st = (sql.prepare <<
            "begin soci_test(:output, :input); end;",
            use(out, "output"),
            use(in, "input"));
        st.execute(1);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;
            Procedure proc = (sql.prepare <<
                "soci_test(:output, :input)",
                use(out, "output"), use(in, "input"));
            proc.execute(1);
            assert(out == in);
        }
    }

    std::cout << "test 6 passed" << std::endl;
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

struct InOutProcedureCreator : public ProcedureCreatorBase
{
    InOutProcedureCreator(Session& session) 
        : ProcedureCreatorBase(session)
    {
        session << "create or replace procedure soci_test(s in out varchar2)"
                " as begin s := s || s; end;";
    }
};

struct ReturnsNullProcedureCreator : public ProcedureCreatorBase
{
    ReturnsNullProcedureCreator(Session& session) 
        : ProcedureCreatorBase(session)
    {
        session << "create or replace procedure soci_test(s in out varchar2)"
            " as begin s := NULL; end;"; 
    }
};

void test7()
{
    Session sql(backEnd, connectString);
    {
        BasicTableCreator tableCreator(sql);

        int id(1);
        StringHolder in("my string");
        sql << "insert into soci_test(id, name) values(:id, :name)", use(id), use(in);

        StringHolder out;
        sql << "select name from soci_test", into(out);
        assert(out.get() == "my string");

        Row r;
        sql << "select * from soci_test", into(r);
        StringHolder dynamicOut = r.get<StringHolder>(1);
        assert(dynamicOut.get() == "my string");
    }

    // test procedure with user-defined type as in-out parameter    
    {
        InOutProcedureCreator procedureCreator(sql);

        StringHolder sh("test");
        Procedure proc = (sql.prepare << "soci_test(:s)", use(sh));
        proc.execute(1);
        assert(sh.get() == "testtest");
    }

    // test procedure which returns null
    {
         ReturnsNullProcedureCreator procedureCreator(sql);

         StringHolder sh;           
         eIndicator ind = eOK;
         Procedure proc = (sql.prepare << "soci_test(:s)", use(sh, ind));
         proc.execute(1);
         assert(ind == eNull);
    }

    std::cout << "test 7 passed" << std::endl;
}

// test bulk insert features
void test8()
{
    Session sql(backEnd, connectString);

    BasicTableCreator tableCreator(sql);

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
            sql << "insert into soci_test(id,code) values(:id,:code)",
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
            sql << "select from soci_test", into(ids), into(codes);
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
            sql << "insert into soci_test (id) values(:id)", use(ids, "id");
        }
        catch (SOCIError const &e)
        {
            error = e.what();
            //TODO e could be made to tell which row(s) failed
        }
        sql.commit();
        assert(error.find("ORA-01438") != std::string::npos);
        int count(7);
        sql << "select count(*) from soci_test", into(count);
        assert(count == 1);
        sql << "delete from soci_test";
    }

    // test insert
    {
        std::vector<int> ids;
        for (int i = 0; i != 3; ++i)
        {
            ids.push_back(i+10);
        }

        Statement st = (sql.prepare << "insert into soci_test(id) values(:id)",
                            use(ids));
        st.execute(1);
        int count;
        sql << "select count(*) from soci_test", into(count);
        assert(count == 3);
    }

    //verify an exception is thrown if into vector is zero length
    {
        std::vector<int> ids;
        bool caught(false);
        try
        {
            sql << "select id from soci_test", into(ids);
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
            sql << "insert into soci_test(id) values(:id)", use(ids);
        }
        catch (SOCIError const &)
        {
            caught = true;
        }
        assert(caught);
    }

    // test "no data" condition
    {
        std::vector<eIndicator> inds(3);
        std::vector<int> ids_out(3);
        Statement st = (sql.prepare << "select id from soci_test where 1=0",
                        into(ids_out, inds));

        // false return value means "no data"
        assert(!st.execute(1));

        // that's it - nothing else is guaranteed
        // and nothing else is to be tested here
    }

    // test NULL indicators
    {
        std::vector<int> ids(3);
        sql << "select id from soci_test", into(ids);

        std::vector<eIndicator> inds_in;
        inds_in.push_back(eOK);
        inds_in.push_back(eNull);
        inds_in.push_back(eOK);

        std::vector<int> new_codes;
        new_codes.push_back(10);
        new_codes.push_back(11);
        new_codes.push_back(10);

        sql << "update soci_test set code = :code where id = :id",
                use(new_codes, inds_in), use(ids);

        std::vector<eIndicator> inds_out(3);
        std::vector<int> codes(3);

        sql << "select code from soci_test", into(codes, inds_out);
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
            sql << "select code from soci_test", into(intos);
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
        Statement st = (sql.prepare << "select id from soci_test",
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
        Statement st = (sql.prepare << "select id from soci_test",
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
        Statement st2 = (sql.prepare << "select id from soci_test",
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
        sql << "insert into soci_test(id) values(:id)", use(more);

        std::vector<int> ids(2);
        Statement st3 = (sql.prepare << "select id from soci_test", into(ids));
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

    std::cout << "test 8 passed" << std::endl;
}

// more tests for bulk fetch
void test9()
{
    Session sql(backEnd, connectString);

    BasicTableCreator tableCreator(sql);

    std::vector<int> in;
    for (int i = 1; i <= 10; ++i)
    {
        in.push_back(i);
    }

    sql << "insert into soci_test (id) values(:id)", use(in);

    int count(0);
    sql << "select count(*) from soci_test", into(count);
    assert(count == 10);

    // verify that the exception is thrown when trying to resize
    // the output vector to the size that is bigger than that
    // at the time of binding
    {
        std::vector<int> out(4);
        Statement st = (sql.prepare <<
            "select id from soci_test", into(out));

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
            "select id from soci_test", into(out));

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

    std::cout << "test 9 passed" << std::endl;
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

struct PersonTableCreator : public TableCreatorBase 
{
    PersonTableCreator(Session& session) 
        : TableCreatorBase(session) 
    {
        session << "create table soci_test(id numeric(5,0) NOT NULL,"
             << " last_name varchar2(20), first_name varchar2(20), "
                " gender varchar2(10))";
    }
};

struct Times100ProcedureCreator : public ProcedureCreatorBase
{
    Times100ProcedureCreator(Session& session) 
        : ProcedureCreatorBase(session)
    {
        session << "create or replace procedure soci_test(id in out number)"
               " as begin id := id * 100; end;"; 
    }
};

void test10()
{
    Session sql(backEnd, connectString);

    {
        PersonTableCreator tableCreator(sql);

        Person p;
        p.id = 1;
        p.lastName = "Smith";
        p.firstName = "Pat";
        sql << "insert into soci_test(id, first_name, last_name, gender) "
            << "values(:ID, :FIRST_NAME, :LAST_NAME, :GENDER)", use(p);
    
        // p should be unchanged
        assert(p.id == 1);
        assert(p.firstName == "Pat");
        assert(p.lastName.get() == "Smith");

        Person p1;
        sql << "select * from soci_test", into(p1);
        assert(p1.id == 1);
        assert(p1.firstName + p1.lastName.get() == "PatSmith");
        assert(p1.gender == "unknown");

        p.firstName = "Patricia";
        sql << "update soci_test set first_name = :FIRST_NAME "
               "where id = :ID", use(p);

        // p should be unchanged
        assert(p.id == 1);
        assert(p.firstName == "Patricia");
        assert(p.lastName.get() == "Smith");
        // Note: gender is now "unknown" because of the mapping, not ""
        assert(p.gender == "unknown"); 

        Person p2;
        sql << "select * from soci_test", into(p2);
        assert(p2.id == 1);
        assert(p2.firstName + p2.lastName.get() == "PatriciaSmith");

        // insert a second row so we can test fetching
        Person p3;
        p3.id = 2;
        p3.firstName = "Joe";
        p3.lastName = "Smith";
        sql << "insert into soci_test(id, first_name, last_name, gender) "
            << "values(:ID, :FIRST_NAME, :LAST_NAME, :GENDER)", use(p3);

        Person p4;
        Statement st = (sql.prepare << "select * from soci_test order by id",
                    into(p4));

        st.execute();
        assert(st.fetch());
        assert(p4.id == 1);
        assert(p4.firstName == "Patricia");

        assert(st.fetch());
        assert(p4.id == 2);
        assert(p4.firstName == "Joe");
        assert(!st.fetch());
    }

    // test with stored procedure
    {
        Times100ProcedureCreator procedureCreator(sql);

        Person p;
        p.id = 1;
        p.firstName = "Pat";
        p.lastName = "Smith";
        Procedure proc = (sql.prepare << "soci_test(:ID)", use(p));
        proc.execute(1);
        assert(p.id == 100);
        assert(p.firstName == "Pat");
        assert(p.lastName.get() == "Smith");
    }

    // test with stored procedure which returns null
    {
        ReturnsNullProcedureCreator procedureCreator(sql);

        std::string msg;
        Person p;
        try
        {
            Procedure proc = (sql.prepare << "soci_test(:FIRST_NAME)", 
                                use(p));
            proc.execute(1);
        }
        catch (SOCIError& e)
        {
            msg = e.what();
        }
        assert(msg == "Column FIRST_NAME contains NULL value and"
                      " no default was provided");

        Procedure proc = (sql.prepare << "soci_test(:GENDER)", 
                                use(p));
        proc.execute(1);
        assert(p.gender == "unknown");        

    }
    std::cout << "test 10 passed" << std::endl;
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
}

void test11()
{
    Session sql(backEnd, connectString);

    PersonTableCreator tableCreator(sql);

    Person p;
    p.id = 1;
    p.lastName = "Smith";
    p.firstName = "Patricia";
    sql << "insert into soci_test(id, first_name, last_name, gender) "
        << "values(:ID, :FIRST_NAME, :LAST_NAME, :GENDER)", use(p);

    //  test position-based conversion
    Person2 p3;
    sql << "select id, first_name, last_name, gender from soci_test", into(p3);
    assert(p3.id == 1);
    assert(p3.firstName + p3.lastName == "PatriciaSmith");
    assert(p3.gender == "whoknows");

    sql << "update soci_test set gender = 'F' where id = 1";

    // additional test for stream-like conversion
    Person3 p4;
    sql << "select id, first_name, last_name, gender from soci_test", into(p4);
    assert(p4.id == 1);
    assert(p4.firstName + p4.lastName == "PatriciaSmith");
    assert(p4.gender == "F");

    std::cout << "test 11 passed" << std::endl;
}

//
// Support for SOCI Common Tests
//

struct TableCreator1 : public TableCreatorBase
{
    TableCreator1(Session& session)
        : TableCreatorBase(session) 
    {
        session << "create table soci_test(id number, val number(4,0), c char, "
                 "str varchar2(20), sh number, ul number, d number, "
                 "tm date, i1 number, i2 number, i3 number, name varchar2(20))";
    }
};

struct TableCreator2 : public TableCreatorBase
{
    TableCreator2(Session& session)
        : TableCreatorBase(session)
    {
        session  << "create table soci_test(\"num_float\" number, \"num_int\" numeric(4,0),"
                     " \"name\" varchar2(20), \"sometime\" date, \"chr\" char)";
    }
};

struct TableCreator3 : public TableCreatorBase
{
    TableCreator3(Session& session)
        : TableCreatorBase(session)
    {
        session << "create table soci_test(\"name\" varchar2(100) not null, "
            "\"phone\" varchar2(15))";
    }
};

class TestContext :public TestContextBase
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
        return "to_date('" + dateString + "', 'YYYY-MM-DD HH24:MI:SS')";
    }
};
    
int main(int argc, char** argv)
{
#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run asser()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

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
        TestContext tc(backEnd, connectString);
        CommonTests tests(tc);
        tests.run();

        std::cout << "\nSOCI Oracle tests:\n\n"; 
     
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
 
        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
    return 0;
}
