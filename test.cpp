#include "soci.h"
#include <iostream>
#include <cassert>
#include <ctime>

using namespace SOCI;

const char *serviceName = "SERVICENAME";
const char *userName = "username";
const char *password = "password";

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

int main()
{
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

        std::cout << "\nOK, all tests passed.\n\n";
    }
    catch (std::exception const & e)
    {
        std::cout << e.what() << '\n';
    }
}
