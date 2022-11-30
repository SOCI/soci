//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
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
#include <vector>
#include <soci/soci-backend.h>

// Disable flood of nonsense warnings generated for SQLite
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4510 4512 4610)
#endif

namespace sqlite_api
{

#if SQLITE_VERSION_NUMBER < 3003010
// The sqlite3_destructor_type typedef introduced in 3.3.10
// http://www.sqlite.org/cvstrac/tktview?tn=2191
typedef void (*sqlite3_destructor_type)(void*);
#endif

#include <sqlite3.h>

} // namespace sqlite_api

#undef SQLITE_STATIC
#define SQLITE_STATIC ((sqlite_api::sqlite3_destructor_type)0)

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace soci
{

class SOCI_SQLITE3_DECL sqlite3_soci_error : public soci_error
{
public:
    sqlite3_soci_error(std::string const & msg, int result);

    int result() const;

private:
    int result_;
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
    std::size_t size() override;

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

    std::size_t size() override;

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
    data_type type_;

    union
    {
        sqlite3_column_buffer buffer_;
        int int32_;
        sqlite_api::sqlite3_int64 int64_;
        double double_;
    };
};

typedef std::vector<sqlite3_column> sqlite3_row;
typedef std::vector<sqlite3_row> sqlite3_recordset;


struct sqlite3_column_info
{
    data_type type_;
    std::string name_;
};
typedef std::vector<sqlite3_column_info> sqlite3_column_info_list;

struct sqlite3_session_backend;
struct sqlite3_statement_backend : details::statement_backend
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

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum, data_type &dtype,
                                std::string &columnName) override;

    sqlite3_standard_into_type_backend * make_into_type_backend() override;
    sqlite3_standard_use_type_backend * make_use_type_backend() override;
    sqlite3_vector_into_type_backend * make_vector_into_type_backend() override;
    sqlite3_vector_use_type_backend * make_vector_use_type_backend() override;

    sqlite3_session_backend &session_;
    sqlite_api::sqlite3_stmt *stmt_;
    sqlite3_recordset dataCache_;
    sqlite3_recordset useData_;
    bool databaseReady_;
    bool boundByName_;
    bool boundByPos_;
    sqlite3_column_info_list columns_;


    bool hasVectorIntoElements_;
    long long rowsAffectedBulk_; // number of rows affected by the last bulk operation

private:
    exec_fetch_result load_rowset(int totalRows);
    exec_fetch_result load_one();
    exec_fetch_result bind_and_execute(int number);
};

struct sqlite3_rowid_backend : details::rowid_backend
{
    sqlite3_rowid_backend(sqlite3_session_backend &session);

    ~sqlite3_rowid_backend() override;

    unsigned long value_;
};

struct sqlite3_blob_backend : details::blob_backend
{
    sqlite3_blob_backend(sqlite3_session_backend &session);

    ~sqlite3_blob_backend() override;

    std::size_t get_len() override;

    std::size_t read_from_start(char * buf, std::size_t toRead, std::size_t offset = 0) override;

    std::size_t write_from_start(const char * buf, std::size_t toWrite, std::size_t offset = 0) override;

    std::size_t append(char const *buf, std::size_t toWrite) override;
    void trim(std::size_t newLen) override;

    sqlite3_session_backend &session_;

    std::size_t set_data(char const *buf, std::size_t toWrite);
    const char *get_buffer() const { return buf_; }

private:
    char *buf_;
    size_t len_;
};

struct sqlite3_session_backend : details::session_backend
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
    std::string create_column_type(data_type dt,
                                           int , int ) override
    {
        switch (dt)
        {
            case dt_xml:
            case dt_string:
                return "text";
            case dt_double:
                return "real";
            case dt_date:
            case dt_integer:
            case dt_long_long:
            case dt_unsigned_long_long:
                return "integer";
            case dt_blob:
                return "blob";
            default:
                throw soci_error("this data_type is not supported in create_column");
        }

    }
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
