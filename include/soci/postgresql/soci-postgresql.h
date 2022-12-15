//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2011 Gevorg Voskanyan
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_POSTGRESQL_H_INCLUDED
#define SOCI_POSTGRESQL_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_POSTGRESQL_SOURCE
# define SOCI_POSTGRESQL_DECL SOCI_DECL_EXPORT
#else
# define SOCI_POSTGRESQL_DECL SOCI_DECL_IMPORT
#endif

#include <soci/soci-backend.h>
#include "soci/connection-parameters.h"
#include <libpq-fe.h>
#include <vector>
#include <unordered_map>

namespace soci
{

class SOCI_POSTGRESQL_DECL postgresql_soci_error : public soci_error
{
public:
    postgresql_soci_error(std::string const & msg, char const * sqlst);

    std::string sqlstate() const;

    error_category get_error_category() const override { return cat_; }

private:
    char sqlstate_[ 5 ];   // not std::string to keep copy-constructor no-throw
    error_category cat_;
};

struct postgresql_session_backend;

namespace details
{

// A class thinly encapsulating PGresult. Its main purpose is to ensure that
// PQclear() is always called, avoiding result memory leaks.
class postgresql_result
{
public:
    // Creates a wrapper for the given, possibly NULL, result. The wrapper
    // object takes ownership of the result object and will call PQclear() on it.
    explicit postgresql_result(
        postgresql_session_backend & sessionBackend,
        PGresult * result)
        : sessionBackend_(sessionBackend)
    {
        init(result);
    }

    // Frees any currently stored result pointer and takes ownership of the
    // given one.
    void reset(PGresult* result = NULL)
    {
        clear();
        init(result);
    }

    // Check whether the status is PGRES_COMMAND_OK and throw an exception if
    // it is different. Notice that if the query can return any results,
    // check_for_data() below should be used instead to verify whether anything
    // was returned or not.
    //
    // The provided error message is used only for the exception being thrown
    // and should describe the operation which yielded this result.
    void check_for_errors(char const* errMsg) const;

    // Check whether the status indicates successful query completion, either
    // with the return results (in which case true is returned) or without them
    // (then false is returned). If the status corresponds to an error, throws
    // an exception, just as check_for_errors().
    bool check_for_data(char const* errMsg) const;

    // Implicit conversion to const PGresult: this is somewhat dangerous but
    // allows us to avoid changing the existing code that uses PGresult and
    // avoids the really bad problem with calling PQclear() twice accidentally
    // as this would require a conversion to non-const pointer that we do not
    // provide.
    operator const PGresult*() const { return result_; }

    // Get the associated result (which may be NULL). Unlike the implicit
    // conversion above, this one returns a non-const pointer, so you should be
    // careful to avoid really modifying it.
    PGresult* get_result() const { return result_; }

    // Dtor frees the result.
    ~postgresql_result() { clear(); }

private:
    void init(PGresult* result)
    {
        result_ = result;
    }

    void clear()
    {
        // Notice that it is safe to call PQclear() with NULL pointer, it
        // simply does nothing in this case.
        PQclear(result_);
    }

    postgresql_session_backend & sessionBackend_;
    PGresult* result_;

    SOCI_NOT_COPYABLE(postgresql_result)
};

} // namespace details

struct postgresql_statement_backend;
struct postgresql_standard_into_type_backend : details::standard_into_type_backend
{
    postgresql_standard_into_type_backend(postgresql_statement_backend & st)
        : statement_(st) {}

    void define_by_pos(int & position,
        void * data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch,
        indicator * ind) override;

    void clean_up() override;

    postgresql_statement_backend & statement_;

    void * data_;
    details::exchange_type type_;
    int position_;
};

struct postgresql_vector_into_type_backend : details::vector_into_type_backend
{
    postgresql_vector_into_type_backend(postgresql_statement_backend & st)
        : statement_(st), user_ranges_(true) {}

    void define_by_pos(int & position,
        void * data, details::exchange_type type) override
    {
        user_ranges_ = false;
        define_by_pos_bulk(position, data, type, 0, &end_var_);
    }

    void define_by_pos_bulk(int & position,
        void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator * ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() override; // active size (might be lower than full vector size)
    std::size_t full_size();    // actual size of the user-provided vector

    void clean_up() override;

    postgresql_statement_backend & statement_;

    void * data_;
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;
    bool user_ranges_;
    int position_;
};

struct postgresql_standard_use_type_backend : details::standard_use_type_backend
{
    postgresql_standard_use_type_backend(postgresql_statement_backend & st)
        : statement_(st), position_(0), buf_(NULL) {}

    void bind_by_pos(int & position,
        void * data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const & name,
        void * data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const * ind) override;
    void post_use(bool gotData, indicator * ind) override;

    void clean_up() override;

    postgresql_statement_backend & statement_;

    void * data_;
    details::exchange_type type_;
    int position_;
    std::string name_;
    char * buf_;

private:
    // Allocate buf_ of appropriate size and copy string data into it.
    void copy_from_string(std::string const& s);
};

struct postgresql_vector_use_type_backend : details::vector_use_type_backend
{
    postgresql_vector_use_type_backend(postgresql_statement_backend & st)
        : statement_(st), position_(0) {}

    void bind_by_pos(int & position,
        void * data, details::exchange_type type) override
    {
        bind_by_pos_bulk(position, data, type, 0, &end_var_);
    }

    void bind_by_pos_bulk(int & position,
        void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    void bind_by_name(std::string const & name,
        void * data, details::exchange_type type) override
    {
        bind_by_name_bulk(name, data, type, 0, &end_var_);
    }

    void bind_by_name_bulk(const std::string & name,
        void * data, details::exchange_type type,
        std::size_t begin, std::size_t * end) override;

    void pre_use(indicator const * ind) override;

    std::size_t size() override; // active size (might be lower than full vector size)
    std::size_t full_size();    // actual size of the user-provided vector

    void clean_up() override;

    postgresql_statement_backend & statement_;

    void * data_;
    details::exchange_type type_;
    std::size_t begin_;
    std::size_t * end_;
    std::size_t end_var_;
    int position_;
    std::string name_;
    std::vector<char *> buffers_;
};

struct postgresql_statement_backend : details::statement_backend
{
    postgresql_statement_backend(postgresql_session_backend & session,
        bool single_row_mode);
    ~postgresql_statement_backend() override;

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const & query,
        details::statement_type stType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;

    std::string rewrite_for_procedure_call(std::string const & query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum, data_type & dtype,
        std::string & columnName) override;

    postgresql_standard_into_type_backend * make_into_type_backend() override;
    postgresql_standard_use_type_backend * make_use_type_backend() override;
    postgresql_vector_into_type_backend * make_vector_into_type_backend() override;
    postgresql_vector_use_type_backend * make_vector_use_type_backend() override;

    postgresql_session_backend & session_;

    bool single_row_mode_;

    details::postgresql_result result_;
    std::string query_;
    details::statement_type stType_;
    std::string statementName_;
    std::vector<std::string> names_; // list of names for named binds

    long long rowsAffectedBulk_; // number of rows affected by the last bulk operation

    int numberOfRows_;  // number of rows retrieved from the server
    int currentRow_;    // "current" row number to consume in postFetch
    int rowsToConsume_; // number of rows to be consumed in postFetch

    bool justDescribed_; // to optimize row description with immediately
                         // following actual statement execution

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

    // the following map is used to keep the results of column
    // type queries with custom types
    typedef std::unordered_map<unsigned long, char> CategoryByColumnOID;
    CategoryByColumnOID categoryByColumnOID_;
};

struct postgresql_rowid_backend : details::rowid_backend
{
    postgresql_rowid_backend(postgresql_session_backend & session);

    ~postgresql_rowid_backend() override;

    unsigned long value_;
};

struct postgresql_blob_backend : details::blob_backend
{
    postgresql_blob_backend(postgresql_session_backend & session);

    ~postgresql_blob_backend() override;

    std::size_t get_len() override;

    std::size_t read_from_start(char * buf, std::size_t toRead, std::size_t offset = 0) override;

    std::size_t write_from_start(const char * buf, std::size_t toWrite, std::size_t offset = 0) override;

    std::size_t append(char const * buf, std::size_t toWrite) override;

    void trim(std::size_t newLen) override;

    postgresql_session_backend & session_;

    unsigned long oid_; // oid of the large object
    int fd_;            // descriptor of the large object
};

struct postgresql_session_backend : details::session_backend
{
    postgresql_session_backend(connection_parameters const & parameters,
        bool single_row_mode);

    ~postgresql_session_backend() override;

    void connect(connection_parameters const & parameters);

    bool is_connected() override;

    void begin() override;
    void commit() override;
    void rollback() override;

    void deallocate_prepared_statement(const std::string & statementName);

    bool get_next_sequence_value(session & s,
        std::string const & sequence, long long & value) override;

    std::string get_dummy_from_table() const override { return std::string(); }

    std::string get_backend_name() const override { return "postgresql"; }

    void clean_up();

    postgresql_statement_backend * make_statement_backend() override;
    postgresql_rowid_backend * make_rowid_backend() override;
    postgresql_blob_backend * make_blob_backend() override;

    std::string get_next_statement_name();

    int statementCount_;
    bool single_row_mode_;
    PGconn * conn_;
    connection_parameters connectionParameters_;
};


struct postgresql_backend_factory : backend_factory
{
    postgresql_backend_factory() {}
    postgresql_session_backend * make_session(
        connection_parameters const & parameters) const override;
};

extern SOCI_POSTGRESQL_DECL postgresql_backend_factory const postgresql;

extern "C"
{

// for dynamic backend loading
SOCI_POSTGRESQL_DECL backend_factory const * factory_postgresql();
SOCI_POSTGRESQL_DECL void register_factory_postgresql();

} // extern "C"

} // namespace soci

#endif // SOCI_POSTGRESQL_H_INCLUDED
