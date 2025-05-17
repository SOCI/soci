//
// Copyright (C) 2024 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_CONTEXT_H_INCLUDED
#define SOCI_TEST_CONTEXT_H_INCLUDED

#include "soci/soci.h"

#include <cassert>
#include <clocale>
#include <cstdlib>
#include <memory>
#include <string>

// These variables are defined in the existing tests code and could be removed
// later.
extern std::string connectString;
extern soci::backend_factory const &backEnd;

namespace soci
{

namespace tests
{

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

using auto_table_creator = std::unique_ptr<table_creator_base>;


// This is a singleton class, at any given time there is at most one test
// context alive and common_tests fixture class uses it.
class test_context_base
{
public:
    test_context_base()
        : backEndFactory_(backEnd)
    {
        // This can't be a CHECK() because the test context is constructed
        // outside of any test.
        assert(!the_test_context_);

        the_test_context_ = this;
    }

    static test_context_base const& get_instance()
    {
        assert(the_test_context_);

        return *the_test_context_;
    }

    // This accessor is used by main() to allow it to initialize the test
    // context. It shouldn't be used anywhere else.
    static test_context_base* get_instance_pointer()
    {
        return the_test_context_;
    }

    // This function is called from main() with the value passed on the command
    // line, if any.
    //
    // If it returns false, an error message about the connection string being
    // required is printed and the tests exits immediately with an error, so
    // the backends that don't require a connection string should override it.
    virtual bool initialize_connect_string(std::string argFromCommandLine)
    {
        assert(connectString_.empty()); // Shouldn't be called more than once.

        if (argFromCommandLine.empty())
            return false;

        connectString_ = argFromCommandLine;

        // For compatibility with the existings tests, also set the global
        // variable which they sometimes use instead of the one here.
        ::connectString = argFromCommandLine;

        return true;
    }

    // This is a purely informational function returning an example of a DSN
    // which can be used with this backend.
    //
    // It should be overridden if initialize_connect_string() is _not_
    // overridden to improve the error message given when the connection string
    // is not specified on the command line.
    virtual std::string get_example_connection_string() const { return {}; }

    // This function is called before starting to run the test suite.
    //
    // If it returns false, the process exits with error code 1 immediately,
    // without running any tests. The function should print the reason for the
    // error in this case.
    virtual bool start_testing()
    {
        // To allow running tests in non-default ("C") locale, the following
        // environment variable can be set and then the current default locale
        // (which can itself be changed by setting LC_ALL environment variable)
        // will then be used.
        if (std::getenv("SOCI_TEST_USE_LC_ALL"))
            std::setlocale(LC_ALL, "");

        return true;
    }

    backend_factory const & get_backend_factory() const
    {
        return backEndFactory_;
    }

    std::string get_connect_string() const
    {
        return connectString_;
    }

    // Return the name of the backend as used in the dynamic library name.
    virtual std::string get_backend_name() const = 0;

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

    // Override this if the backend doesn't handle partial success for
    // operations using array parameters correctly.
    virtual bool has_partial_update_bug() const { return false; }

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
        the_test_context_ = NULL;
    }

private:
    backend_factory const &backEndFactory_;
    std::string connectString_;

    static test_context_base* the_test_context_;

    SOCI_NOT_COPYABLE(test_context_base)
};

// Base class for all "normal" tests, i.e. those that run common tests.
class test_context_common : public test_context_base
{
public:
    // This is implemented in test-common.cpp.
    test_context_common();
};

// Fixture class for tests that need to use the database.
class common_tests
{
public:
    common_tests()
    : tc_(test_context_base::get_instance()),
      backEndFactory_(tc_.get_backend_factory()),
      connectString_(tc_.get_connect_string())
    {}

protected:
    test_context_base const & tc_;
    backend_factory const &backEndFactory_;
    std::string const connectString_;

    SOCI_NOT_COPYABLE(common_tests)
};

} // namespace tests

} // namespace soci

#endif // SOCI_TEST_CONTEXT_H_INCLUDED
