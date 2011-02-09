//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Copyright (C) 2011 Denis Chapligin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_DB2_H_INCLUDED
#define SOCI_DB2_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_DB2_SOURCE
#   define SOCI_DB2_DECL __declspec(dllexport)
#  else
#   define SOCI_DB2_DECL __declspec(dllimport)
#  endif // SOCI_DB2_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_DB2_DECL isn't defined yet define it now
#ifndef SOCI_DB2_DECL
# define SOCI_DB2_DECL
#endif

#include "soci-backend.h"

#include <cstddef>
#include <string>

#include <sqlcli1.h>

namespace soci
{

class db2_soci_error : public soci_error {
public:
    db2_soci_error(std::string const & msg, SQLRETURN rc) : soci_error(msg),errorCode(rc) {};
    
    SQLRETURN errorCode;    
};

struct db2_statement_backend;

struct SOCI_DB2_DECL db2_standard_into_type_backend : details::standard_into_type_backend
{
    db2_standard_into_type_backend(db2_statement_backend &st)
        : statement_(st)
    {}

    void define_by_pos(int& position, void* data, details::exchange_type type);

    void pre_fetch();
    void post_fetch(bool gotData, bool calledFromFetch, indicator* ind);

    void clean_up();

    db2_statement_backend& statement_;
};

struct SOCI_DB2_DECL db2_vector_into_type_backend : details::vector_into_type_backend
{
    db2_vector_into_type_backend(db2_statement_backend &st)
        : statement_(st)
    {}

    void define_by_pos(int& position, void* data, details::exchange_type type);

    void pre_fetch();
    void post_fetch(bool gotData, indicator* ind);

    void resize(std::size_t sz);
    std::size_t size();

    void clean_up();

    db2_statement_backend& statement_;
};

struct SOCI_DB2_DECL db2_standard_use_type_backend : details::standard_use_type_backend
{
    db2_standard_use_type_backend(db2_statement_backend &st)
        : statement_(st)
    {}

    void bind_by_pos(int& position, void* data, details::exchange_type type, bool readOnly);
    void bind_by_name(std::string const& name, void* data, details::exchange_type type, bool readOnly);

    void pre_use(indicator const* ind);
    void post_use(bool gotData, indicator* ind);

    void clean_up();

    db2_statement_backend& statement_;
};

struct SOCI_DB2_DECL db2_vector_use_type_backend : details::vector_use_type_backend
{
    db2_vector_use_type_backend(db2_statement_backend &st)
        : statement_(st) {}

    void bind_by_pos(int& position, void* data, details::exchange_type type);
    void bind_by_name(std::string const& name, void* data, details::exchange_type type);

    void pre_use(indicator const* ind);

    std::size_t size();

    void clean_up();

    db2_statement_backend& statement_;
};

struct db2_session_backend;
struct SOCI_DB2_DECL db2_statement_backend : details::statement_backend
{
    db2_statement_backend(db2_session_backend &session);

    void alloc();
    void clean_up();
    void prepare(std::string const& query, details::statement_type eType);

    exec_fetch_result execute(int number);
    exec_fetch_result fetch(int number);

    long long get_affected_rows();
    int get_number_of_rows();

    std::string rewrite_for_procedure_call(std::string const& query);

    int prepare_for_describe();
    void describe_column(int colNum, data_type& dtype, std::string& columnName);

    db2_standard_into_type_backend* make_into_type_backend();
    db2_standard_use_type_backend* make_use_type_backend();
    db2_vector_into_type_backend* make_vector_into_type_backend();
    db2_vector_use_type_backend* make_vector_use_type_backend();

    db2_session_backend& session_;
};

struct db2_rowid_backend : details::rowid_backend
{
    db2_rowid_backend(db2_session_backend &session);

    ~db2_rowid_backend();
};

struct db2_blob_backend : details::blob_backend
{
    db2_blob_backend(db2_session_backend& session);

    ~db2_blob_backend();

    std::size_t get_len();
    std::size_t read(std::size_t offset, char* buf, std::size_t toRead);
    std::size_t write(std::size_t offset, char const* buf, std::size_t toWrite);
    std::size_t append(char const* buf, std::size_t toWrite);
    void trim(std::size_t newLen);

    db2_session_backend& session_;
};

struct db2_session_backend : details::session_backend
{
    db2_session_backend(std::string const& connectString);

    ~db2_session_backend();

    void begin();
    void commit();
    void rollback();

    std::string get_backend_name() const { return "DB2"; }

    void clean_up();

    db2_statement_backend* make_statement_backend();
    db2_rowid_backend* make_rowid_backend();
    db2_blob_backend* make_blob_backend();

    std::string dsn;
    std::string username;
    std::string password;
    bool autocommit;

    SQLHANDLE hEnv; /* Environment handle */
    SQLHANDLE hDbc; /* Connection handle */
};

struct SOCI_DB2_DECL db2_backend_factory : backend_factory
{
	db2_backend_factory() {}
    db2_session_backend* make_session(std::string const& connectString) const;
};

extern SOCI_DB2_DECL db2_backend_factory const db2;

extern "C"
{

// for dynamic backend loading
SOCI_DB2_DECL backend_factory const* factory_db2();
SOCI_DB2_DECL void register_factory_db2();

} // extern "C"

} // namespace soci

#endif // SOCI_DB2_H_INCLUDED
