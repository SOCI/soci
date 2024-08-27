//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_MYSQL_H_INCLUDED
#define SOCI_MYSQL_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_MYSQL_SOURCE
# define SOCI_MYSQL_DECL SOCI_DECL_EXPORT
#else
# define SOCI_MYSQL_DECL SOCI_DECL_IMPORT
#endif

#include <soci/soci-backend.h>
#include <private/soci-trivial-blob-backend.h>
#ifdef _WIN32
#include <winsock.h> // SOCKET
#endif // _WIN32

// Some version of mysql.h contain trailing comma in an enum declaration that
// trigger -Wpedantic, so suppress it as there is nothing to be done about it
// using the macros defined in our private soci-compiler.h header, that we can
// only include when building SOCI itself.
#ifdef SOCI_MYSQL_SOURCE
    #include "soci-compiler.h"
#endif

#ifdef SOCI_GCC_WARNING_SUPPRESS
    SOCI_GCC_WARNING_SUPPRESS(pedantic)
#endif

#include <mysql.h> // MySQL Client
#include <errmsg.h> // MySQL Error codes

#ifdef SOCI_GCC_WARNING_RESTORE
    SOCI_GCC_WARNING_RESTORE(pedantic)
#endif

#include <vector>


namespace soci
{

class SOCI_MYSQL_DECL mysql_soci_error : public soci_error
{
public:
    mysql_soci_error(std::string const & msg, int errNum)
        : soci_error(msg), err_num_(errNum), cat_(unknown) {
            if(errNum == CR_CONNECTION_ERROR ||
               errNum == CR_CONN_HOST_ERROR ||
               errNum == CR_SERVER_GONE_ERROR ||
               errNum == CR_SERVER_LOST ||
               errNum == 1927) { // Lost connection to backend server
                cat_ = connection_error;
            }
        }

    error_category get_error_category() const override { return cat_; }

    unsigned int err_num_;
    error_category cat_;
};

struct mysql_statement_backend;
struct mysql_standard_into_type_backend : details::standard_into_type_backend
{
    mysql_standard_into_type_backend(mysql_statement_backend &st)
        : statement_(st) {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
        indicator *ind) override;

    void clean_up() override;

    mysql_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
};

struct mysql_vector_into_type_backend : details::vector_into_type_backend
{
    mysql_vector_into_type_backend(mysql_statement_backend &st)
        : statement_(st) {}

    void define_by_pos(int &position,
        void *data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator *ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() override;

    void clean_up() override;

    mysql_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
};

struct mysql_standard_use_type_backend : details::standard_use_type_backend
{
    mysql_standard_use_type_backend(mysql_statement_backend &st)
        : statement_(st), position_(0), buf_(NULL) {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const *ind) override;
    void post_use(bool gotData, indicator *ind) override;

    void clean_up() override;

    mysql_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
    std::string name_;
    char *buf_;
};

struct mysql_vector_use_type_backend : details::vector_use_type_backend
{
    mysql_vector_use_type_backend(mysql_statement_backend &st)
        : statement_(st), position_(0) {}

    void bind_by_pos(int &position,
        void *data, details::exchange_type type) override;
    void bind_by_name(std::string const &name,
        void *data, details::exchange_type type) override;

    void pre_use(indicator const *ind) override;

    std::size_t size() override;

    void clean_up() override;

    mysql_statement_backend &statement_;

    void *data_;
    details::exchange_type type_;
    int position_;
    std::string name_;
    std::vector<char *> buffers_;
};

struct mysql_session_backend;
struct mysql_statement_backend : details::statement_backend
{
    mysql_statement_backend(mysql_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const &query,
        details::statement_type eType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;

    std::string rewrite_for_procedure_call(std::string const &query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum,
        db_type &dbtype,
        std::string &columnName) override;
    data_type to_data_type(db_type dbt) const override;

    mysql_standard_into_type_backend * make_into_type_backend() override;
    mysql_standard_use_type_backend * make_use_type_backend() override;
    mysql_vector_into_type_backend * make_vector_into_type_backend() override;
    mysql_vector_use_type_backend * make_vector_use_type_backend() override;

    mysql_session_backend &session_;

    MYSQL_RES *result_;

    // The query is split into chunks, separated by the named parameters;
    // e.g. for "SELECT id FROM ttt WHERE name = :foo AND gender = :bar"
    // we will have query chunks "SELECT id FROM ttt WHERE name = ",
    // "AND gender = " and names "foo", "bar".
    std::vector<std::string> queryChunks_;
    std::vector<std::string> names_; // list of names for named binds

    long long rowsAffectedBulk_; // number of rows affected by the last bulk operation

    int numberOfRows_;  // number of rows retrieved from the server
    int currentRow_;    // "current" row number to consume in postFetch
    int rowsToConsume_; // number of rows to be consumed in postFetch

    bool justDescribed_; // to optimize row description with immediately
                         // following actual statement execution

    // Set to true if the last column passed to describe_column() was a
    // MEDIUMINT UNSIGNED one, see to_data_type().
    bool lastDescribedUnsignedMediumInt_ = false;

    // Prefetch the row offsets in order to use mysql_row_seek() for
    // random access to rows, since mysql_data_seek() is expensive.
    std::vector<MYSQL_ROW_OFFSET> resultRowOffsets_;

    bool hasIntoElements_;
    bool hasVectorIntoElements_;
    bool hasUseElements_;
    bool hasVectorUseElements_;

    // the following maps are used for finding data buffers according to
    // use elements specified by the user

    typedef std::map<int, char **> UseByPosBuffersMap;
    UseByPosBuffersMap useByPosBuffers_;

    typedef std::map<std::string, char **> UseByNameBuffersMap;
    UseByNameBuffersMap useByNameBuffers_;
};

struct mysql_rowid_backend : details::rowid_backend
{
    mysql_rowid_backend(mysql_session_backend &session);

    ~mysql_rowid_backend() override;
};

class mysql_blob_backend : public details::trivial_blob_backend
{
public:
    mysql_blob_backend(mysql_session_backend &session);

    ~mysql_blob_backend() override;

    std::size_t hex_str_size() const;
    void write_hex_str(char *buf, std::size_t size) const;
    std::string as_hex_str() const;

    void load_from_hex_str(const char* str, std::size_t length);
};

struct mysql_session_backend : details::session_backend
{
    mysql_session_backend(connection_parameters const & parameters);

    ~mysql_session_backend() override;

    bool is_connected() override;

    void begin() override;
    void commit() override;
    void rollback() override;

    bool get_last_insert_id(session&, std::string const&, long long&) override;

    // Note that MySQL supports both "SELECT 2+2" and "SELECT 2+2 FROM DUAL"
    // syntaxes, but there doesn't seem to be any reason to use the longer one.
    std::string get_dummy_from_table() const override { return std::string(); }

    std::string get_backend_name() const override { return "mysql"; }

    void clean_up();

    mysql_statement_backend * make_statement_backend() override;
    mysql_rowid_backend * make_rowid_backend() override;
    mysql_blob_backend * make_blob_backend() override;

    std::string get_table_names_query() const override
    {
        return R"delim(SELECT LOWER(table_name) AS 'TABLE_NAME' FROM information_schema.tables WHERE table_schema = DATABASE())delim";
    }

    std::string get_column_descriptions_query() const override
    {
        return R"delim(SELECT column_name as "COLUMN_NAME",
            data_type as "DATA_TYPE",
            character_maximum_length as "CHARACTER_MAXIMUM_LENGTH",
            numeric_precision as "NUMERIC_PRECISION",
            numeric_scale as "NUMERIC_SCALE",
            is_nullable as "IS_NULLABLE"
            from information_schema.columns
            where
            case
            when :s is not NULL THEN table_schema = :s
            else table_schema = DATABASE()
            end
            and UPPER(table_name) = UPPER(:t))delim";
    }

    MYSQL *conn_;
};


struct mysql_backend_factory : backend_factory
{
    mysql_backend_factory() {}
    mysql_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_MYSQL_DECL mysql_backend_factory const mysql;

extern "C"
{

// for dynamic backend loading
SOCI_MYSQL_DECL backend_factory const * factory_mysql();
SOCI_MYSQL_DECL void register_factory_mysql();

} // extern "C"

} // namespace soci

#endif // SOCI_MYSQL_H_INCLUDED
