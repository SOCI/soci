//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_EMPTY_H_INCLUDED
#define SOCI_EMPTY_H_INCLUDED

#include <soci/soci-platform.h>

#ifdef SOCI_EMPTY_SOURCE
# define SOCI_EMPTY_DECL SOCI_DECL_EXPORT
#else
# define SOCI_EMPTY_DECL SOCI_DECL_IMPORT
#endif

#include <soci/soci-backend.h>

#include <cstddef>
#include <string>

namespace soci
{

struct empty_statement_backend;

struct SOCI_EMPTY_DECL empty_standard_into_type_backend : details::standard_into_type_backend
{
    empty_standard_into_type_backend(empty_statement_backend &st)
        : statement_(st)
    {}

    void define_by_pos(int& position, void* data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, bool calledFromFetch, indicator* ind) override;

    void clean_up() override;

    empty_statement_backend& statement_;
};

struct SOCI_EMPTY_DECL empty_vector_into_type_backend : details::vector_into_type_backend
{
    empty_vector_into_type_backend(empty_statement_backend &st)
        : statement_(st)
    {}

    void define_by_pos(int& position, void* data, details::exchange_type type) override;

    void pre_fetch() override;
    void post_fetch(bool gotData, indicator* ind) override;

    void resize(std::size_t sz) override;
    std::size_t size() const override;

    void clean_up() override;

    empty_statement_backend& statement_;
};

struct SOCI_EMPTY_DECL empty_standard_use_type_backend : details::standard_use_type_backend
{
    empty_standard_use_type_backend(empty_statement_backend &st)
        : statement_(st)
    {}

    void bind_by_pos(int& position, void* data, details::exchange_type type, bool readOnly) override;
    void bind_by_name(std::string const& name, void* data, details::exchange_type type, bool readOnly) override;

    void pre_use(indicator const* ind) override;
    void post_use(bool gotData, indicator* ind) override;

    void clean_up() override;

    empty_statement_backend& statement_;
};

struct SOCI_EMPTY_DECL empty_vector_use_type_backend : details::vector_use_type_backend
{
    empty_vector_use_type_backend(empty_statement_backend &st)
        : statement_(st) {}

    void bind_by_pos(int& position, void* data, details::exchange_type type) override;
    void bind_by_name(std::string const& name, void* data, details::exchange_type type) override;

    void pre_use(indicator const* ind) override;

    std::size_t size() const override;

    void clean_up() override;

    empty_statement_backend& statement_;
};

struct empty_session_backend;
struct SOCI_EMPTY_DECL empty_statement_backend : details::statement_backend
{
    empty_statement_backend(empty_session_backend &session);

    void alloc() override;
    void clean_up() override;
    void prepare(std::string const& query, details::statement_type eType) override;

    exec_fetch_result execute(int number) override;
    exec_fetch_result fetch(int number) override;

    long long get_affected_rows() override;
    int get_number_of_rows() override;
    std::string get_parameter_name(int index) const override;

    std::string rewrite_for_procedure_call(std::string const& query) override;

    int prepare_for_describe() override;
    void describe_column(int colNum, db_type& dbtype, std::string& columnName) override;

    empty_standard_into_type_backend* make_into_type_backend() override;
    empty_standard_use_type_backend* make_use_type_backend() override;
    empty_vector_into_type_backend* make_vector_into_type_backend() override;
    empty_vector_use_type_backend* make_vector_use_type_backend() override;

    empty_session_backend& session_;
};

struct SOCI_EMPTY_DECL empty_rowid_backend : details::rowid_backend
{
    empty_rowid_backend(empty_session_backend &session);

    ~empty_rowid_backend() override;
};

struct SOCI_EMPTY_DECL empty_blob_backend : details::blob_backend
{
    empty_blob_backend(empty_session_backend& session);

    ~empty_blob_backend() override;

    std::size_t get_len() override;

    std::size_t read_from_start(void * buf, std::size_t toRead, std::size_t offset = 0) override;

    std::size_t write_from_start(const void * buf, std::size_t toWrite, std::size_t offset = 0) override;

    std::size_t append(const void* buf, std::size_t toWrite) override;
    void trim(std::size_t newLen) override;

    details::session_backend &get_session_backend() override;

    empty_session_backend& session_;
};

struct SOCI_EMPTY_DECL empty_session_backend : details::session_backend
{
    empty_session_backend(connection_parameters const& parameters);

    ~empty_session_backend() override;

    bool is_connected() override { return true; }

    void begin() override;
    void commit() override;
    void rollback() override;

    std::string get_dummy_from_table() const override { return std::string(); }

    std::string get_backend_name() const override { return "empty"; }

    void clean_up();

    empty_statement_backend* make_statement_backend() override;
    empty_rowid_backend* make_rowid_backend() override;
    empty_blob_backend* make_blob_backend() override;
};

struct SOCI_EMPTY_DECL empty_backend_factory : backend_factory
{
    empty_backend_factory() {}
    empty_session_backend* make_session(connection_parameters const& parameters) const override;
};

extern SOCI_EMPTY_DECL empty_backend_factory const empty;

extern "C"
{

// for dynamic backend loading
SOCI_EMPTY_DECL backend_factory const* factory_empty();
SOCI_EMPTY_DECL void register_factory_empty();

} // extern "C"

} // namespace soci

#endif // SOCI_EMPTY_H_INCLUDED
