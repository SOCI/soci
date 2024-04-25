#ifndef SOCI_COMMON_TESTS_H_
#define SOCI_COMMON_TESTS_H_

#include <catch.hpp>

#include <soci/soci.h>
#include <soci-compiler.h>

#include <string>
#include <cassert>
#include <memory>
#include <iomanip>
#include <iostream>
#include <clocale>

#ifdef SOCI_HAVE_BOOST
// explicitly pull conversions for Boost's optional, tuple and fusion:
#include <boost/version.hpp>
#include "soci/boost-optional.h"
#include "soci/boost-tuple.h"
#include "soci/boost-gregorian-date.h"
#if defined(BOOST_VERSION) && BOOST_VERSION >= 103500
#include "soci/boost-fusion.h"
#endif // BOOST_VERSION
#endif // SOCI_HAVE_BOOST
#ifdef SOCI_HAVE_CXX17
#include "soci/std-optional.h"
#endif


#if defined(_MSC_VER) && (_MSC_VER < 1500)
#undef SECTION
#define SECTION(name) INTERNAL_CATCH_SECTION(name, "dummy-for-vc8")
#endif

namespace soci
{

// Helper used by test_roundtrip() below which collects all round trip test
// data and allows to define a type conversion for it.
template<typename T>
struct Roundtrip
{
    typedef T val_type;
    Roundtrip(soci::db_type type, T val)
        : inType(type), inVal(val) {}

    soci::db_type inType;
    T inVal;

    soci::db_type outType;
    T outVal;
};

// Test a rountrip insertion data to the current database for the arithmetic type T
// This test specifically use the dynamic bindings and the DDL creation statements.
template<typename T>
struct type_conversion<Roundtrip<T>>
{
    static_assert(std::is_arithmetic<T>::value, "Roundtrip currently supported only for numeric types");
    typedef soci::values base_type;
    static void from_base(soci::values const &v, soci::indicator, Roundtrip<T> &t)
    {
        t.outType = v.get_properties(0).get_db_type();
        switch (t.outType)
        {
            case soci::db_int8:   t.outVal = static_cast<T>(v.get<std::int8_t>(0));   break;
            case soci::db_uint8:  t.outVal = static_cast<T>(v.get<std::uint8_t>(0));  break;
            case soci::db_int16:  t.outVal = static_cast<T>(v.get<std::int16_t>(0));  break;
            case soci::db_uint16: t.outVal = static_cast<T>(v.get<std::uint16_t>(0)); break;
            case soci::db_int32:  t.outVal = static_cast<T>(v.get<std::int32_t>(0));  break;
            case soci::db_uint32: t.outVal = static_cast<T>(v.get<std::uint32_t>(0)); break;
            case soci::db_int64:  t.outVal = static_cast<T>(v.get<std::int64_t>(0));  break;
            case soci::db_uint64: t.outVal = static_cast<T>(v.get<std::uint64_t>(0)); break;
            case soci::db_double: t.outVal = static_cast<T>(v.get<double>(0));        break;
            default: FAIL_CHECK("Unsupported type mapped to db_type"); break;
        }
    }
    static void to_base(Roundtrip<T> const &t, soci::values &v, soci::indicator&)
    {
        v.set("VAL", t.inVal);
    }
};

namespace tests
{

template<typename T>
void check(soci::Roundtrip<T> const &val)
{
    CHECK(val.inType == val.outType);
    CHECK(val.inVal == val.outVal);
}

template<>
void check(soci::Roundtrip<double> const &val);

template<typename T>
void test_roundtrip(soci::session &sql, soci::db_type inputType, T inputVal)
{
    try
    {
        Roundtrip<T> tester(inputType, inputVal);

        const std::string table = "TEST_ROUNDTRIP";
        sql.create_table(table).column("VAL", tester.inType);
        struct table_dropper
        {
            table_dropper(soci::session& sql, std::string const& table)
                : sql_(sql), table_(table) {}
            ~table_dropper() { sql_ << "DROP TABLE " << table_; }

            soci::session& sql_;
            const std::string table_;
        } dropper(sql, table);

        sql << "INSERT INTO " << table << "(VAL) VALUES (:VAL)", soci::use(const_cast<const Roundtrip<T>&>(tester));
        soci::statement stmt = (sql.prepare << "SELECT * FROM " << table);
        stmt.exchange(soci::into(tester));
        stmt.define_and_bind();
        stmt.execute();
        stmt.fetch();
        check(tester);
    }
    catch (const std::exception& e)
    {
        FAIL_CHECK(e.what());
    }
}

// TODO: improve cleanup capabilities by subtypes, soci_test name may be omitted --mloskot
//       i.e. optional ctor param accepting custom table name
class table_creator_base
{
public:
    table_creator_base(session& sql)
        : msession(sql) { drop(); }

    virtual ~table_creator_base() { drop();}
private:
    void drop()
    {
        try
        {
            msession << "drop table soci_test";
        }
        catch (soci_error const& e)
        {
            //std::cerr << e.what() << std::endl;
            e.what();
        }
    }
    session& msession;

    SOCI_NOT_COPYABLE(table_creator_base)
};

class procedure_creator_base
{
public:
    procedure_creator_base(session& sql)
        : msession(sql) { drop(); }

    virtual ~procedure_creator_base() { drop();}
private:
    void drop()
    {
        try { msession << "drop procedure soci_test"; } catch (soci_error&) {}
    }
    session& msession;

    SOCI_NOT_COPYABLE(procedure_creator_base)
};

class function_creator_base
{
public:
    function_creator_base(session& sql)
        : msession(sql) { drop(); }

    virtual ~function_creator_base() { drop();}

protected:
    virtual std::string dropstatement()
    {
        return "drop function soci_test";
    }

private:
    void drop()
    {
        try { msession << dropstatement(); } catch (soci_error&) {}
    }
    session& msession;

    SOCI_NOT_COPYABLE(function_creator_base)
};

// This is a singleton class, at any given time there is at most one test
// context alive and common_tests fixture class uses it.
class test_context_base
{
public:
    test_context_base(backend_factory const &backEnd,
                    std::string const &connectString)
        : backEndFactory_(backEnd),
          connectString_(connectString)
    {
        // This can't be a CHECK() because the test context is constructed
        // outside of any test.
        assert(!the_test_context_);

        the_test_context_ = this;

        // To allow running tests in non-default ("C") locale, the following
        // environment variable can be set and then the current default locale
        // (which can itself be changed by setting LC_ALL environment variable)
        // will then be used.
        if (std::getenv("SOCI_TEST_USE_LC_ALL"))
            std::setlocale(LC_ALL, "");
    }

    static test_context_base const& get_instance()
    {
        REQUIRE(the_test_context_);

        return *the_test_context_;
    }

    backend_factory const & get_backend_factory() const
    {
        return backEndFactory_;
    }

    std::string get_connect_string() const
    {
        return connectString_;
    }

    virtual std::string to_date_time(std::string const &dateTime) const = 0;

    virtual table_creator_base* table_creator_1(session&) const = 0;
    virtual table_creator_base* table_creator_2(session&) const = 0;
    virtual table_creator_base* table_creator_3(session&) const = 0;
    virtual table_creator_base* table_creator_4(session&) const = 0;

    // Override this to return the table creator for a simple table containing
    // an integer "id" column and CLOB "s" one.
    //
    // Returns null by default to indicate that CLOB is not supported.
    virtual table_creator_base* table_creator_clob(session&) const { return NULL; }

    // Override this to return the table creator for a simple table containing
    // an integer "id" column and BLOB "b" one.
    //
    // Returns null by default to indicate that BLOB is not supported.
    virtual table_creator_base* table_creator_blob(session&) const { return NULL; }

    // Override this to return the table creator for a simple table containing
    // an integer "id" column and XML "x" one.
    //
    // Returns null by default to indicate that XML is not supported.
    virtual table_creator_base* table_creator_xml(session&) const { return NULL; }

    // Override this to return the table creator for a simple table containing
    // an identity integer "id" and a simple integer "val" columns.
    //
    // Returns null by default to indicate that identity is not supported.
    virtual table_creator_base* table_creator_get_last_insert_id(session&) const { return NULL; }

    // Return the casts that must be used to convert the between the database
    // XML type and the query parameters.
    //
    // By default no special casts are done.
    virtual std::string to_xml(std::string const& x) const { return x; }
    virtual std::string from_xml(std::string const& x) const { return x; }

    // Override this if the backend not only supports working with XML values
    // (and so returns a non-null value from table_creator_xml()), but the
    // database itself has real XML support instead of just allowing to store
    // and retrieve XML as text. "Real" support means at least preventing the
    // application from storing malformed XML in the database.
    virtual bool has_real_xml_support() const { return false; }

    // Override this if the backend doesn't handle floating point values
    // correctly, i.e. writing a value and reading it back doesn't return
    // *exactly* the same value.
    virtual bool has_fp_bug() const { return false; }

    // Override this if the backend wrongly returns CR LF when reading a string
    // with just LFs from the database to strip the unwanted CRs.
    virtual std::string fix_crlf_if_necessary(std::string const& s) const { return s; }

    // Override this if the backend doesn't handle multiple active select
    // statements at the same time, i.e. a result set must be entirely consumed
    // before creating a new one (this is the case of MS SQL without MARS).
    virtual bool has_multiple_select_bug() const { return false; }

    // Override this if the backend may not have transactions support.
    virtual bool has_transactions_support(session&) const { return true; }

    // Override this if the backend silently truncates string values too long
    // to fit by default.
    virtual bool has_silent_truncate_bug(session&) const { return false; }

    // Override this if the backend doesn't distinguish between empty and null
    // strings (Oracle does this).
    virtual bool treats_empty_strings_as_null() const { return false; }

    // Override this if the backend does not store values bigger than INT64_MAX
    // correctly. This can lead to an unexpected ordering of values as larger
    // values might be stored as overflown and therefore negative integer.
    virtual bool has_uint64_storage_bug() const { return false; }

    // Override this if the backend truncates integer values bigger than INT64_MAX.
    virtual bool truncates_uint64_to_int64() const { return false; }

    // Override this to call commit() if it's necessary for the DDL statements
    // to be taken into account (currently this is only the case for Firebird).
    virtual void on_after_ddl(session&) const { }

    // Put the database in SQL-complient mode for CHAR(N) values, return false
    // if it's impossible, i.e. if the database doesn't behave correctly
    // whatever we do.
    virtual bool enable_std_char_padding(session&) const { return true; }

    // Return the SQL expression giving the length of the specified string,
    // i.e. "char_length(s)" in standard SQL but often "len(s)" or "length(s)"
    // in practice and sometimes even worse (thanks Oracle).
    virtual std::string sql_length(std::string const& s) const = 0;

    virtual ~test_context_base()
    {
        the_test_context_ = nullptr;
    }

private:
    backend_factory const &backEndFactory_;
    std::string const connectString_;

    static test_context_base* the_test_context_;

    SOCI_NOT_COPYABLE(test_context_base)
};

std::unique_ptr<test_context_base> instantiate_test_context(const soci::backend_factory &backend, const std::string &connection_string);
const backend_factory &create_backend_factory();

// Although SQL standard mandates right padding CHAR(N) values to their length
// with spaces, some backends don't confirm to it:
//
//  - Firebird does pad the string but to the byte-size (not character size) of
//  the column (i.e. CHAR(10) NONE is padded to 10 bytes but CHAR(10) UTF8 --
//  to 40).
//  - For MySql PAD_CHAR_TO_FULL_LENGTH option must be set, otherwise the value
//  is trimmed.
//  - SQLite never behaves correctly at all.
//
// This method will check result string from column defined as fixed char It
// will check only bytes up to the original string size. If padded string is
// bigger than expected string then all remaining chars must be spaces so if
// any non-space character is found it will fail.
void checkEqualPadded(const std::string& padded_str, const std::string& expected_str);

#define CHECK_EQUAL_PADDED(padded_str, expected_str) \
    CHECK_NOTHROW(checkEqualPadded(padded_str, expected_str));

// Objects used later in tests 14,15
struct PhonebookEntry
{
    std::string name;
    std::string phone;
};

struct PhonebookEntry2 : public PhonebookEntry
{
};

class PhonebookEntry3
{
public:
    void setName(std::string const & n) { name_ = n; }
    std::string getName() const { return name_; }

    void setPhone(std::string const & p) { phone_ = p; }
    std::string getPhone() const { return phone_; }

public:
    std::string name_;
    std::string phone_;
};

// user-defined object for test26 and test28
class MyInt
{
public:
    MyInt() : i_() {}
    MyInt(int i) : i_(i) {}
    void set(int i) { i_ = i; }
    int get() const { return i_; }
private:
    int i_;
};

// user-defined object for the "vector of custom type objects" tests.
class MyOptionalString
{
public:
    MyOptionalString() : valid_(false) {}
    MyOptionalString(const std::string& str) : valid_(true), str_(str) {}
    void set(const std::string& str) { valid_ = true; str_ = str; }
    void reset() { valid_ = false; str_.clear(); }
    bool is_valid() const { return valid_; }
    const std::string &get() const { return str_; }
private:
    bool valid_;
    std::string str_;
};

std::ostream& operator<<(std::ostream& ostr, const MyOptionalString& optstr);

std::ostream& operator<<(std::ostream& ostr, const std::vector<MyOptionalString>& vec);

// Compare doubles for approximate equality. This has to be used everywhere
// where we write "3.14" (or "6.28") to the database as a string and then
// compare the value read back with the literal 3.14 floating point constant
// because they are not the same.
//
// It is also used for the backends which currently don't handle doubles
// correctly.
//
// Notice that this function is normally not used directly but rather from the
// macro below.
bool are_doubles_approx_equal(double const a, double const b);

// This is a macro to ensure we use the correct line numbers. The weird
// do/while construction is used to make this a statement and the even weirder
// condition in while ensures that the loop is executed exactly once without
// triggering warnings from MSVC about the condition being always false.
#define ASSERT_EQUAL_APPROX(a, b) \
    do { \
      if (!are_doubles_approx_equal((a), (b))) { \
        FAIL( "Approximate equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )


// Exact double comparison function. We need one, instead of writing "a == b",
// only in order to have some place to put the pragmas disabling gcc warnings.
bool are_doubles_exactly_equal(double a, double b);

#define ASSERT_EQUAL_EXACT(a, b) \
    do { \
      if (!are_doubles_exactly_equal((a), (b))) { \
        FAIL( "Exact equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )


// Compare two floating point numbers either exactly or approximately depending
// on test_context::has_fp_bug() return value.
bool are_doubles_equal(test_context_base const& tc, double a, double b);

// This macro should be used when where we don't have any problems with string
// literals vs floating point literals mismatches described above and would
// ideally compare the numbers exactly but, unfortunately, currently can't do
// this unconditionally because at least some backends are currently buggy and
// don't handle the floating point values correctly.
//
// This can be only used from inside the common_tests class as it relies on
// having an accessible "tc_" variable to determine whether exact or
// approximate comparison should be used.
#define ASSERT_EQUAL(a, b) \
    do { \
      if (!are_doubles_equal(tc_, (a), (b))) { \
        FAIL( "Equality check failed: " \
                  << std::fixed \
                  << std::setprecision(std::numeric_limits<double>::digits10 + 1) \
                  << (a) << " != " << (b) ); \
      } \
    } while ( (void)0, 0 )

}
}

namespace soci
{

// basic type conversion for user-defined type with single base value
template<> struct type_conversion<soci::tests::MyInt>
{
    typedef int base_type;

    static void from_base(int i, indicator ind, soci::tests::MyInt &mi)
    {
        if (ind == i_ok)
        {
            mi.set(i);
        }
    }

    static void to_base(soci::tests::MyInt const &mi, int &i, indicator &ind)
    {
        i = mi.get();
        ind = i_ok;
    }
};

// basic type conversion for string based user-defined type which can be null
template<> struct type_conversion<soci::tests::MyOptionalString>
{
    typedef std::string base_type;

    static void from_base(const base_type& in, indicator ind, soci::tests::MyOptionalString& out)
    {
        if (ind == i_null)
        {
            out.reset();
        }
        else
        {
            out.set(in);
        }
    }

    static void to_base(const soci::tests::MyOptionalString& in, base_type& out, indicator& ind)
    {
        if (in.is_valid())
        {
            out = in.get();
            ind = i_ok;
        }
        else
        {
            ind = i_null;
        }
    }
};

// basic type conversion on many values (ORM)
template<> struct type_conversion<soci::tests::PhonebookEntry>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, soci::tests::PhonebookEntry &pe)
    {
        // here we ignore the possibility the the whole object might be NULL
        pe.name = v.get<std::string>("NAME");
        pe.phone = v.get<std::string>("PHONE", "<NULL>");
    }

    static void to_base(soci::tests::PhonebookEntry const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.name);
        v.set("PHONE", pe.phone, pe.phone.empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

// type conversion which directly calls values::get_indicator()
template<> struct type_conversion<soci::tests::PhonebookEntry2>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, soci::tests::PhonebookEntry2 &pe)
    {
        // here we ignore the possibility the the whole object might be NULL

        pe.name = v.get<std::string>("NAME");
        indicator ind = v.get_indicator("PHONE"); //another way to test for null
        pe.phone = ind == i_null ? "<NULL>" : v.get<std::string>("PHONE");
    }

    static void to_base(soci::tests::PhonebookEntry2 const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.name);
        v.set("PHONE", pe.phone, pe.phone.empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

template<> struct type_conversion<soci::tests::PhonebookEntry3>
{
    typedef soci::values base_type;

    static void from_base(values const &v, indicator /* ind */, soci::tests::PhonebookEntry3 &pe)
    {
        // here we ignore the possibility the the whole object might be NULL

        pe.setName(v.get<std::string>("NAME"));
        pe.setPhone(v.get<std::string>("PHONE", "<NULL>"));
    }

    static void to_base(soci::tests::PhonebookEntry3 const &pe, values &v, indicator &ind)
    {
        v.set("NAME", pe.getName());
        v.set("PHONE", pe.getPhone(), pe.getPhone().empty() ? i_null : i_ok);
        ind = i_ok;
    }
};

}

#endif // SOCI_COMMON_TESTS_H_
