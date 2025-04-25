//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ODBC_H_INCLUDED
#define SOCI_ODBC_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_ODBC_SOURCE
# define SOCI_ODBC_DECL SOCI_DECL_EXPORT
#else
# define SOCI_ODBC_DECL SOCI_DECL_IMPORT
#endif

#include <vector>
#include <soci/soci-backend.h>
#include <sstream>
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#endif
#include <sqlext.h> // ODBC
#include <string.h> // strcpy()

namespace soci
{

namespace details
{
    // TODO: Do we want to make it a part of public interface? --mloskot
    std::size_t const odbc_max_buffer_length = 100 * 1024 * 1024;

    // select max size from following MSDN article
    // https://msdn.microsoft.com/en-us/library/ms130896.aspx
    SQLLEN const ODBC_MAX_COL_SIZE = 8000;

    // This cast is only used to avoid compiler warnings when passing strings
    // to ODBC functions, the returned string may *not* be really modified.
    inline SQLCHAR* sqlchar_cast(std::string const& s)
    {
      return reinterpret_cast<SQLCHAR*>(const_cast<char*>(s.c_str()));
    }

    inline SQLWCHAR* sqlchar_cast(std::wstring const& s)
    {
      return reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(s.c_str()));
    }

    inline SQLWCHAR* sqlchar_cast(std::u16string const& s)
    {
      return reinterpret_cast<SQLWCHAR*>(const_cast<char16_t*>(s.c_str()));
    }
}

// Option allowing to specify the "driver completion" parameter of
// SQLDriverConnect(). Its possible values are the same as the allowed values
// for this parameter in the official ODBC, i.e. one of SQL_DRIVER_XXX (in
// string form as all options are strings currently).
extern SOCI_ODBC_DECL char const * odbc_option_driver_complete;

struct odbc_statement_backend;

// Helper of into and use backends.
class odbc_standard_type_backend_base
{
protected:
    odbc_standard_type_backend_base(odbc_statement_backend &st)
        : statement_(st) {}

    // Check if we need to pass 64 bit integers as strings to the database as
    // some drivers don't support them directly.
    inline bool use_string_for_bigint() const;

    // If we do need to use strings for 64 bit integers, this constant defines
    // the maximal string length needed.
    enum
    {
        // This is the length of decimal representation of UINT64_MAX + 1.
        max_bigint_length = 21
    };

    // IBM DB2 driver is not compliant to ODBC spec for indicators in 64bit
    // SQLLEN is still defined 32bit (int) but spec requires 64bit (long)
    inline bool requires_noncompliant_32bit_sqllen() const;
    inline SQLLEN get_sqllen_from_value(const SQLLEN val) const;
    inline void set_sqllen_from_value(SQLLEN &target, const SQLLEN val) const;

    inline bool supports_negative_tinyint() const;
    inline bool can_convert_to_unsigned_sql_type() const;

    odbc_statement_backend &statement_;
private:
    SOCI_NOT_COPYABLE(odbc_standard_type_backend_base)
};

struct odbc_standard_into_type_backend : details::standard_into_type_backend,
                                         private odbc_standard_type_backend_base
{
    odbc_standard_into_type_backend(odbc_statement_backend &st)
        : odbc_standard_type_backend_base(st), buf_(0)
    {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
        indicator *ind) override;

    void clean_up() override;

    char *buf_;        // generic buffer
    void *data_;
    details::exchange_type type_;
    int position_;
    SQLSMALLINT odbcType_;
    SQLLEN valueLen_;
private:
    SOCI_NOT_COPYABLE(odbc_standard_into_type_backend)
};

struct odbc_vector_into_type_backend : details::vector_into_type_backend,
                                       private odbc_standard_type_backend_base
{
    odbc_vector_into_type_backend(odbc_statement_backend &st)
        : odbc_standard_type_backend_base(st),
          data_(NULL), buf_(NULL), position_(0) {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator *ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() const override;

    void clean_up() override;

    // Normally data retrieved from the database is handled in post_fetch(),
    // however we may need to call SQLFetch() multiple times, so we call this
    // function instead after each call to it to retrieve the given range of
    // rows.
    void do_post_fetch_rows(std::size_t beginRow, std::size_t endRow);

    // IBM DB2 driver is not compliant to ODBC spec for indicators in 64bit
    // SQLLEN is still defined 32bit (int) but spec requires 64bit (long)
    inline SQLLEN get_sqllen_from_vector_at(std::size_t idx) const;

    // Rebind the single vector value at the given index to the first row.
    // Used when vector values are fetched by single row.
    void rebind_row(std::size_t rowInd);

    std::vector<SQLLEN> indHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::exchange_type type_;
    std::size_t colSize_;    // size of the string column (used for strings)
    SQLSMALLINT odbcType_;
    int position_;
};

struct odbc_standard_use_type_backend : details::standard_use_type_backend,
                                        private odbc_standard_type_backend_base
{
    odbc_standard_use_type_backend(odbc_statement_backend &st)
        : odbc_standard_type_backend_base(st),
          position_(-1), data_(0), buf_(0), indHolder_(0) {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const *ind) override;
    void post_use(bool gotData, indicator *ind) override;

    void clean_up() override;

    // Return the pointer to the buffer containing data to be used by ODBC.
    // This can be either data_ itself or buf_, that is allocated by this
    // function if necessary.
    //
    // Also fill in the size of the data and SQL and C types of it.
    void* prepare_for_bind(SQLLEN &size,
       SQLSMALLINT &sqlType, SQLSMALLINT &cType);

    int position_;
    void *data_;
    details::exchange_type type_;
    char *buf_;
    SQLLEN indHolder_;

private:
    // Copy string data to buf_ and set size, sqlType and cType to the values
    // appropriate for strings.
    void copy_from_string(std::string const& s,
                          SQLLEN& size,
                          SQLSMALLINT& sqlType,
                          SQLSMALLINT& cType);

    void copy_from_string(const std::wstring& s,
                          SQLLEN& size,
                          SQLSMALLINT& sqlType,
                          SQLSMALLINT& cType);
};

struct odbc_vector_use_type_backend : details::vector_use_type_backend,
                                      private odbc_standard_type_backend_base
{
    odbc_vector_use_type_backend(odbc_statement_backend &st)
        : odbc_standard_type_backend_base(st),
          data_(NULL), buf_(NULL) {}

    // helper function for preparing indicators
    // (as part of the define_by_pos)
    void prepare_indicators(std::size_t size);

    // helper of pre_use(), return the pointer to the data to be used by ODBC.
    void* prepare_for_bind(SQLUINTEGER &size, SQLSMALLINT &sqlType, SQLSMALLINT &cType);

    void bind_by_pos(int &position,
        void *data, details::exchange_type type) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type) override;

    void pre_use(indicator const *ind) override;

    std::size_t size() const override;

    void clean_up() override;

    // IBM DB2 driver is not compliant to ODBC spec for indicators in 64bit
    // SQLLEN is still defined 32bit (int) but spec requires 64bit (long)
    inline void set_sqllen_from_vector_at(const std::size_t idx, const SQLLEN val);

    std::vector<SQLLEN> indHolderVec_;
    void *data_;
    details::exchange_type type_;
    int position_;
    char *buf_;              // generic buffer
    std::size_t colSize_;    // size of the string column (used for strings)
    // used for strings only
    std::size_t maxSize_;
};

struct odbc_session_backend;
struct SOCI_ODBC_DECL odbc_statement_backend : details::statement_backend
{
    odbc_statement_backend(odbc_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const &query,
        details::statement_type eType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;
    int get_row_to_dump() const override { return error_row_; }

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum,
        db_type &dbtype,
        std::string &columnName) override;
    data_type to_data_type(db_type dbt) const override;

    // helper for defining into vector<string>
    std::size_t column_size(int position);

    odbc_standard_into_type_backend * make_into_type_backend() override;
    odbc_standard_use_type_backend * make_use_type_backend() override;
    odbc_vector_into_type_backend * make_vector_into_type_backend() override;
    odbc_vector_use_type_backend * make_vector_use_type_backend() override;

    odbc_session_backend &session_;
    SQLHSTMT hstmt_;
    SQLULEN numRowsFetched_;
    bool fetchVectorByRows_;
    bool boundByName_;
    bool boundByPos_;

    long long rowsAffected_; // number of rows affected by the last operation

    std::string query_;
    std::vector<std::string> names_; // list of names for named binds

    // This vector, containing non-owning non-null pointers, can be empty if
    // we're not using any vector "intos".
    std::vector<odbc_vector_into_type_backend*> intos_;

private:
    // fetch() helper wrapping SQLFetch() call for the given range of rows.
    exec_fetch_result do_fetch(int beginRow, int endRow);

    // First row with the error for bulk operations or -1.
    int error_row_ = -1;
};

struct SOCI_ODBC_DECL odbc_rowid_backend : details::rowid_backend
{
    odbc_rowid_backend(odbc_session_backend &session);

    ~odbc_rowid_backend() override;
};

struct SOCI_ODBC_DECL odbc_blob_backend : details::blob_backend
{
    odbc_blob_backend(odbc_session_backend &session);

    ~odbc_blob_backend() override;

    std::size_t get_len() override;
    std::size_t read_from_start(void *buf, std::size_t toRead, std::size_t offset = 0) override;
    std::size_t write_from_start(const void *buf, std::size_t toWrite, std::size_t offset = 0) override;
    std::size_t append(const void *buf, std::size_t toWrite) override;
    void trim(std::size_t newLen) override;
    details::session_backend &get_session_backend() override;

    odbc_session_backend &session_;
};

struct SOCI_ODBC_DECL odbc_session_backend : details::session_backend
{
    odbc_session_backend(connection_parameters const & parameters);

    ~odbc_session_backend() override;

    bool is_connected() override;

    void begin() override;
    void commit() override;
    void rollback() override;

    bool get_next_sequence_value(session & s,
        std::string const & sequence, long long & value) override;
    bool get_last_insert_id(session & s,
        std::string const & table, long long & value) override;

    std::string get_dummy_from_table() const override;

    std::string get_backend_name() const override { return "odbc"; }

    void configure_connection();
    void reset_transaction();

    void clean_up();

    odbc_statement_backend * make_statement_backend() override;
    odbc_rowid_backend * make_rowid_backend() override;
    odbc_blob_backend * make_blob_backend() override;

    enum database_product
    {
      prod_uninitialized, // Never returned by get_database_product().
      prod_db2,
      prod_firebird,
      prod_mssql,
      prod_mysql,
      prod_oracle,
      prod_postgresql,
      prod_sqlite,
      prod_unknown = -1
    };

    // Determine the type of the database we're connected to.
    database_product get_database_product() const;

    // Return full ODBC connection string.
    std::string get_connection_string() const { return connection_string_; }

    SQLHENV henv_;
    SQLHDBC hdbc_;

    std::string connection_string_;

private:
    mutable database_product product_;
};

class SOCI_ODBC_DECL odbc_soci_error : public soci_error
{
    SQLCHAR message_[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate_[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode_;

public:
    odbc_soci_error(SQLSMALLINT htype, SQLHANDLE hndl, std::string const & msg);

    error_category get_error_category() const override;

    std::string get_backend_name() const override { return "odbc"; }
    int get_backend_error_code() const override { return sqlcode_; }
    std::string get_sqlstate() const override { return (char const*)sqlstate_; }

    SQLCHAR const * odbc_error_code() const
    {
        return sqlstate_;
    }
    SQLINTEGER native_error_code() const
    {
        return sqlcode_;
    }
    SQLCHAR const * odbc_error_message() const
    {
        return message_;
    }

private:
    // Initialize the member variables and return the full error message
    // corresponding to the last error on the given ODBC object (connection or
    // statement handle depending on htype value).
    std::string
    interpret_odbc_error(SQLSMALLINT htype, SQLHANDLE hndl, std::string const& msg);
};

inline bool is_odbc_error(SQLRETURN rc)
{
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO && rc != SQL_NO_DATA)
    {
        return true;
    }
    else
    {
        return false;
    }
}

inline bool odbc_standard_type_backend_base::use_string_for_bigint() const
{
    // Oracle ODBC driver doesn't support SQL_C_[SU]BIGINT data types
    // (see appendix G.1 of Oracle Database Administrator's reference at
    // https://docs.oracle.com/cd/B19306_01/server.102/b15658/app_odbc.htm),
    // so we need a special workaround for this case and we represent 64
    // bit integers as strings and rely on ODBC driver for transforming
    // them to SQL_NUMERIC.
    return statement_.session_.get_database_product()
            == odbc_session_backend::prod_oracle;
}

inline bool odbc_standard_type_backend_base::requires_noncompliant_32bit_sqllen() const
{
    // IBM DB2 did not implement the ODBC specification for indicator sizes in 64bit.
    // They still use SQLLEN as 32bit even if the driver is compiled 64bit
    // This breaks the backend in terms of using indicators
    // see: https://bugs.php.net/bug.php?id=54007
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
    return statement_.session_.get_database_product()
            == odbc_session_backend::prod_db2;
#else
    return false;
#endif
}

inline SQLLEN odbc_standard_type_backend_base::get_sqllen_from_value(const SQLLEN val) const
{
    if (requires_noncompliant_32bit_sqllen())
    {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-aliasing"
#endif
        return *reinterpret_cast<const int*>(&val);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
    }
    return val;
}

inline void odbc_standard_type_backend_base::set_sqllen_from_value(SQLLEN &target, const SQLLEN val) const
{
    if (requires_noncompliant_32bit_sqllen())
    {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-aliasing"
#endif
        reinterpret_cast<int*>(&target)[0] =  *reinterpret_cast<const int*>(&val);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
    }
    else
    {
        target = val;
    }
}

inline bool odbc_standard_type_backend_base::supports_negative_tinyint() const
{
    // MSSQL ODBC driver only supports a range of [0..255] for tinyint.
    return statement_.session_.get_database_product()
            != odbc_session_backend::prod_mssql;
}

inline bool odbc_standard_type_backend_base::can_convert_to_unsigned_sql_type() const
{
    // MSSQL ODBC driver seemingly can't handle the conversion of unsigned C
    // types to their respective unsigned SQL type because they are out of
    // range for their supported signed types. This results in the error
    // "Numeric value out of range (SQL state 22003)".
    // The only place it works is with tinyint values as their range is
    // [0..255], i.e. they have enough space for unsigned values anyway.
    return statement_.session_.get_database_product()
            != odbc_session_backend::prod_mssql;
}

inline SQLLEN odbc_vector_into_type_backend::get_sqllen_from_vector_at(std::size_t idx) const
{
    if (requires_noncompliant_32bit_sqllen())
    {
        return reinterpret_cast<const int*>(&indHolderVec_[0])[idx];
    }
    return indHolderVec_[idx];
}

inline void odbc_vector_use_type_backend::set_sqllen_from_vector_at(const std::size_t idx, const SQLLEN val)
{
    if (requires_noncompliant_32bit_sqllen())
    {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-aliasing"
#endif
        reinterpret_cast<int*>(&indHolderVec_[0])[idx] = *reinterpret_cast<const int*>(&val);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
    }
    else
    {
        indHolderVec_[idx] = val;
    }
}

struct odbc_backend_factory : backend_factory
{
    odbc_backend_factory() {}
    odbc_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_ODBC_DECL odbc_backend_factory const odbc;

extern "C"
{

// for dynamic backend loading
SOCI_ODBC_DECL backend_factory const * factory_odbc();
SOCI_ODBC_DECL void register_factory_odbc();

} // extern "C"

} // namespace soci

#endif // SOCI_EMPTY_H_INCLUDED
