#include "soci.h"
#include <iostream>
#include <cassert>
#include <ctime>

using namespace SOCI;

char serviceName[25]; 
char userName[25]; 
char password[25];

// fundamental tests
void test1()
{
    {
        Session session(serviceName, userName, password);

        int x = -5;

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
        Session sql(serviceName, userName, password);

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
    Session sql(serviceName, userName, password);

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
        strftime(buf, 25, "%m-%d-%C%y %H:%M:%S", &t2);

        std::string t_out;
        std::string format("MM-DD-YYYY HH24:MI:SS");
        sql << "select to_char(t, :format) from (select :t as t from dual)",
            into(t_out), use(format), use(t2);

        assert(t_out == std::string(buf));
    }
    {
        std::time_t now = std::time(NULL);
        std::time_t t;

        sql << "select t from (select :t as t from dual)",
            into(t), use(now);
        assert(t == now);
    }

    std::cout << "test 2 passed" << std::endl;
}

// indicator test
void test3()
{
    Session sql(serviceName, userName, password);

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
    catch (SOCIError const &e)
    {
        // ORA-01455 happens here (overflow on conversion)
        assert(e.errNum_ == 1455);
    }

    std::cout << "test 3 passed" << std::endl;
}

// explicit calls test
void test4()
{
    Session sql(serviceName, userName, password);

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
    Session sql(serviceName, userName, password);

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
    Session sql(serviceName, userName, password);

    sql <<
        "create table some_table ("
        "    id number(10) not null,"
        "    img blob"
        ")";

    char buf[] = "abcdefghijklmnopqrstuvwxyz";

    sql << "insert into some_table (id, img) values (7, empty_blob())";

    {
        BLOB b(sql);

        OCILobDisableBuffering(sql.svchp_, sql.errhp_, b.lobp_);

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
    Session sql(serviceName, userName, password);

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

// nested statement test (the same syntax is used for output cursors in PL/SQL)
void test8()
{
    Session sql(serviceName, userName, password);

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
    Session sql(serviceName, userName, password);

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

//Stored Procedures
void test10()
{
    {
        Session sql(serviceName, userName, password);
        sql <<
            "create or replace procedure echo(output out varchar2,"
            "input in varchar2) as "
            "begin output := input; end;";

        std::string in("my message");
        std::string out;
        Statement st = (sql.prepare <<"begin echo(:output, :input); end;", 
                                       use(out, "output"), 
                                       use(in, "input"));
        st.execute(1);
        assert(out == in);

        // explicit procedure syntax
        {
            std::string in("my message2");
            std::string out;
            Procedure proc = (sql.prepare << "echo(:output, :input)", 
                                            use(out, "output"),
                                            use(in, "input"));
            proc.execute(1);
            assert(out == in);
        }
    }
    std::cout << "test 10 passed" << std::endl;
    
}

// Dynamic binding to Row objects
void test11()
{
    {
        Session sql(serviceName, userName, password);
        try
        {
            sql << "drop table test11";
        }
        catch(const SOCIError& e){}//ignore error if table doesn't exist

        sql << "create table test11(num_float numeric(7,2) NOT NULL,"
            << " name varchar2(20), when date, large numeric(10,0), "
            << " chr1 char(1), small numeric(4,0), vc varchar(10), fl float)";

        Row r;
        sql << "select * from test11", into(r);
        assert(r.indicator(0) ==  eNoData);

        for (int i=1; i<4; ++i)
        {
            std::ostringstream namestr;
            namestr << "name"<<i;
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

            sql << "insert into test11 values(:num_float, :name, :when, "
                << ":large, :chr1, :small, :vc, :fl)",
                use(d,"num_float"), 
                use(name, "name"),
                use(when, "when"),
                use(l, "large"),
                use(c, "chr1"),
                use(i, "small"),
                use(v, "vc"),
                use(f, "fl");

            sql.commit();
        }

        // select into a Row
        {
            Row r;
            Statement st = (sql.prepare << "select * from test11 order by num_float", into(r));
            st.execute(1);
            assert(r.size() == 8);
        
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

            assert(r.getProperties(0).getSize() == 22);
            assert(r.getProperties(0).getScale() == 2);
            assert(r.getProperties(0).getPrecision() == 7);
            assert(r.getProperties(0).getNullOK() == false);
            assert(r.getProperties(1).getNullOK() == true);

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

            std::tm t = r.get<std::tm>(2);
            assert(t.tm_year == 104);
            assert(t.tm_mon == 11);
            assert(t.tm_mday == 2);
            
            assert(r.indicator(0) == eOK);

            // verify exception thrown on invalid get<>
            bool cought = false;
            try{ r.get<std::string>(0); }catch(std::bad_cast& e)
            {
              cought = true;
            }
            assert(cought);
        }
    }
    std::cout << "test 11 passed" << std::endl;
}

// bind into user-defined objects
struct StringHolder
{
    StringHolder(){}
    StringHolder(const char* s):s_(s){}
    StringHolder(std::string s):s_(s){}
    std::string get(){return s_;}
private:
    std::string s_;
};
namespace SOCI
{
    template<> class TypeConversion<StringHolder>
    {
    public:
        typedef std::string base_type;
        static StringHolder from(std::string& s){return StringHolder(s);}
        static std::string to(StringHolder& sh){return sh.get();}
    };
}
void test12()
{
    {
        Session sql(serviceName, userName, password);
        try
        { sql << "drop table test12";
        }
        catch(const SOCIError& e){}//ignore error if table doesn't exist
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
    std::cout << "test 12 passed" << std::endl;
}

// test multiple use types of the same underlying type
void test13()
{
    Session sql(serviceName, userName, password);
    try { sql<<"drop table test13";}catch(SOCIError& e){}//ignore

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
                     <<"insert into test13(id,idtest,name,nametest)"
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
   Session sql(serviceName, userName, password);
   try{sql << "drop table test14";} catch(const SOCIError& e){}//ignore error 

   sql << "create table test14(chr1 char(1))";

   char c_in = 'Z';
   sql << "insert into test14(chr1) values(:C)", use(c_in);
   sql.commit();

   char c_out = ' ';
   sql << "select chr1 from test14", into(c_out);
   assert(c_out == 'Z');
   
   std::cout << "test 14 passed" <<std::endl; 
}

int main(int argc, char** argv)
{
    if (argc == 4)
    {
        strcpy(userName, argv[1]);
        strcpy(password, argv[2]);
        strcpy(serviceName, argv[3]);
    }
    else 
    {
        std::cout<<"usage: "<<argv[0]<<" [user] [password] [serviceName]\n";
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

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
