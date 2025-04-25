//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_SQLITE3_H_INCLUDED
#define SOCI_SQLITE3_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_SQLITE3_SOURCE
# define SOCI_SQLITE3_DECL SOCI_DECL_EXPORT
#else
# define SOCI_SQLITE3_DECL SOCI_DECL_IMPORT
#endif

#include <cstdarg>
#include <cstdint>
#include <vector>
#include <soci/soci-backend.h>
#include <soci/trivial-blob-backend.h>

// Disable flood of nonsense warnings generated for SQLite
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4510 4512 4610)
#endif

namespace sqlite_api
{

// Don't include sqlite3.h from outside SOCI: this header might not be
// available when using built-in SQLite3 as we don't install it in this case.
#ifdef SOCI_SQLITE3_SOURCE
    #include <sqlite3.h>
#else // !SOCI_SQLITE3_SOURCE
// We need just a couple of forward declarations to make this header itself
// compile.
struct sqlite3;
struct sqlite3_stmt;

#if defined(_MSC_VER)
  typedef __int64 sqlite3_int64;
  typedef unsigned __int64 sqlite3_uint64;
#else
  typedef long long int sqlite3_int64;
  typedef unsigned long long int sqlite3_uint64;
#endif
#endif // SOCI_SQLITE3_SOURCE/!SOCI_SQLITE3_SOURCE

} // namespace sqlite_api

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace soci
{

class SOCI_SQLITE3_DECL sqlite3_soci_error : public soci_error
{
public:
    // Create an error object corresponding to the last SQLite error on this
    // connection.
    //
    // If prefix is not empty, it is prepended to the SQLite error message.
    //
    // If the error message is non-null, it is used, otherwise sqlite3_errmsg()
    // is called to get it.
    sqlite3_soci_error(sqlite_api::sqlite3* conn,
      std::string const& prefix,
      char const* errmsg = nullptr);

    // Broad category of the error, SQLite-specific functions below can be used
    // for more precise classification.
    error_category get_error_category() const override;

    // This is the primary SQLite error code.
    int result() const;

    // Return the extended SQLite error code, providing more details.
    int extended_result() const;

    // Implement generic functions from the base class.
    std::string get_backend_name() const override { return "sqlite3"; }
    int get_backend_error_code() const override { return extended_result(); }

private:
    // We store the extended result only, primary result is obtained from it.
    int extended_result_;
};

struct sqlite3_statement_backend;
struct sqlite3_standard_into_type_backend : details::standard_into_type_backend
{
    sqlite3_standard_into_type_backend(sqlite3_statement_backend &st)
        : statement_(st), data_(0), type_(), position_(0)
    {
    }

    void define_by_pos(int &position,
                             void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
                           indicator *ind) override;

    void clean_up() override;

    sqlite3_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
};

struct sqlite3_vector_into_type_backend : details::vector_into_type_backend
{
    sqlite3_vector_into_type_backend(sqlite3_statement_backend &st)
        : statement_(st), data_(0), type_(), position_(0)
    {
    }

    void define_by_pos(int& position, void* data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator* ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() const override;

    void clean_up() override;

    sqlite3_statement_backend& statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
};

struct sqlite3_standard_use_type_backend : details::standard_use_type_backend
{
    sqlite3_standard_use_type_backend(sqlite3_statement_backend &st);

    void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const *ind) override;
    void post_use(bool gotData, indicator *ind) override;

    void clean_up() override;

    sqlite3_statement_backend &statement_;

    void *data_;                    // pointer to used data: soci::use(myvariable) --> data_ = &myvariable
    details::exchange_type type_;   // type of data_
    int position_;                  // binding position
    std::string name_;              // binding name
};

struct sqlite3_vector_use_type_backend : details::vector_use_type_backend
{
    sqlite3_vector_use_type_backend(sqlite3_statement_backend &st)
        : statement_(st), data_(0), type_(), position_(0)
    {
    }

    void bind_by_pos(int &position,
                           void *data, details::exchange_type type) override;
    void bind_by_name(std::string const &name,
                            void *data, details::exchange_type type) override;

    void pre_use(indicator const *ind) override;

    std::size_t size() const override;

    void clean_up() override;

    sqlite3_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
    std::string name_;
};

struct sqlite3_column_buffer
{
    std::size_t size_;
    union
    {
        const char *constData_;
        char *data_;
    };
};

struct sqlite3_column
{
    bool isNull_;
    // DEPRECATED. USE dataType_ INSTEAD.
    data_type type_;
    db_type dataType_;

    union
    {
        sqlite3_column_buffer buffer_;
        int8_t int8_;
        uint8_t uint8_;
        int16_t int16_;
        uint16_t uint16_;
        int32_t int32_;
        uint32_t uint32_;
        sqlite_api::sqlite3_int64 int64_;
        sqlite_api::sqlite3_uint64 uint64_;
        double double_;
    };
};

typedef std::vector<sqlite3_column> sqlite3_row;
typedef std::vector<sqlite3_row> sqlite3_recordset;


struct sqlite3_column_info
{
    // DEPRECATED. USE dataType_ INSTEAD.
    data_type type_;
    db_type dataType_;
    std::string name_;
};
typedef std::vector<sqlite3_column_info> sqlite3_column_info_list;

struct sqlite3_session_backend;
struct SOCI_SQLITE3_DECL sqlite3_statement_backend : details::statement_backend
{
    sqlite3_statement_backend(sqlite3_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const &query,
        details::statement_type eType) override;
    void reset_if_needed();
    void reset();

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;
    int get_row_to_dump() const override { return current_row_; }

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum,
                                db_type &dbtype,
                                std::string &columnName) override;

    sqlite3_standard_into_type_backend * make_into_type_backend() override;
    sqlite3_standard_use_type_backend * make_use_type_backend() override;
    sqlite3_vector_into_type_backend * make_vector_into_type_backend() override;
    sqlite3_vector_use_type_backend * make_vector_use_type_backend() override;

    db_type exchange_dbtype_for(db_type type) const override;

    sqlite3_session_backend &session_;
    sqlite_api::sqlite3_stmt *stmt_;
    sqlite3_recordset dataCache_;
    sqlite3_recordset useData_;
    bool databaseReady_;
    bool boundByName_;
    bool boundByPos_;
    sqlite3_column_info_list columns_;


    long long rowsAffectedBulk_; // number of rows affected by the last bulk operation

private:
    // This is used in bind_and_execute(), called by execute() for bulk
    // operations and allows to show the values corresponding to the last
    // processed row in the query context generated by calling dump_value()
    // using this row number if an error occurs.
    int current_row_ = -1;

    exec_fetch_result load_rowset(int totalRows);
    exec_fetch_result load_one();
    exec_fetch_result bind_and_execute(int number);
};

struct SOCI_SQLITE3_DECL sqlite3_rowid_backend : details::rowid_backend
{
    sqlite3_rowid_backend(sqlite3_session_backend &session);

    ~sqlite3_rowid_backend() override;

    unsigned long value_;
};

struct SOCI_SQLITE3_DECL sqlite3_blob_backend : details::trivial_blob_backend
{
    sqlite3_blob_backend(sqlite3_session_backend &session);

    ~sqlite3_blob_backend() override;

    void ensure_buffer_initialized();
};

struct SOCI_SQLITE3_DECL sqlite3_session_backend : details::session_backend
{
    sqlite3_session_backend(connection_parameters const & parameters);

    ~sqlite3_session_backend() override;

    bool is_connected() override { return true; }

    void begin() override;
    void commit() override;
    void rollback() override;

    bool get_last_insert_id(session&, std::string const&, long long&) override;

    std::string empty_blob() override
    {
        return "x\'\'";
    }

    std::string get_dummy_from_table() const override { return std::string(); }

    std::string get_backend_name() const override { return "sqlite3"; }

    void clean_up();

    sqlite3_statement_backend * make_statement_backend() override;
    sqlite3_rowid_backend * make_rowid_backend() override;
    sqlite3_blob_backend * make_blob_backend() override;
    std::string get_table_names_query() const override
    {
        return "select name as \"TABLE_NAME\""
                " from sqlite_master where type = 'table'";
    }
    std::string get_column_descriptions_query() const override
    {
        return "select name as 'COLUMN_NAME',"
            " 0 as 'CHARACTER_MAXIMUM_LENGTH',"
            " 0 as 'NUMERIC_PRECISION',"
            " case when type like '%real%' or type like '%float%' or type like '%double%' then 255 else 0 end as 'NUMERIC_SCALE',"
            " case"
                " when type like 'text'   or type like 'clob'     or type like '%char%'    then 'text'"
                " when type like '%int%'  or type like '%number%' or type like '%numeric%' then 'integer'"
                " when type like '%real%' or type like '%float%'  or type like '%double%'  then 'number'"
                " else type"
            " end as 'DATA_TYPE',"
        " case when \"notnull\" = 0 then 'YES' else 'NO' end as 'IS_NULLABLE'"
        " from (select name, lower(type) as type, \"notnull\" from pragma_table_info(:t))";
    }
    std::string create_column_type(db_type dt,
                                   int , int ) override
    {
        switch (dt)
        {
            case db_xml:
            case db_string:
                return "text";
            case db_double:
                return "real";
            case db_date:
                return "integer";
            case db_int8:
                return "tinyint";
            case db_uint8:
                return "unsignedtinyint";
            case db_int16:
                return "smallint";
            case db_uint16:
                return "unsignedsmallint";
            case db_int32:
                return "integer";
            case db_uint32:
                return "unsignedint";
            case db_int64:
                return "bigint";
            case db_uint64:
                return "unsignedbigint";
            case db_blob:
                return "blob";
            default:
                throw soci_error("this db_type is not supported in create_column");
        }
    }

    // Get information about SQLite3 version used.
    static const char* libversion();
    static int libversion_number();

    sqlite_api::sqlite3 *conn_;

    // This flag is set to true if the internal sqlite_sequence table exists in
    // the database.
    bool sequence_table_exists_;
};

struct sqlite3_backend_factory : backend_factory
{
    sqlite3_backend_factory() {}
    sqlite3_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_SQLITE3_DECL sqlite3_backend_factory const sqlite3;

extern "C"
{

// for dynamic backend loading
SOCI_SQLITE3_DECL backend_factory const * factory_sqlite3();
SOCI_SQLITE3_DECL void register_factory_sqlite3();

} // extern "C"

} // namespace soci

#endif // SOCI_SQLITE3_H_INCLUDED
