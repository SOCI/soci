//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_EMPTY_H_INCLUDED
#define SOCI_EMPTY_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_EMPTY_SOURCE
#   define SOCI_EMPTY_DECL __declspec(dllexport)
#  else
#   define SOCI_EMPTY_DECL __declspec(dllimport)
#  endif // SOCI_EMPTY_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_EMPTY_DECL isn't defined yet define it now
#ifndef SOCI_EMPTY_DECL
# define SOCI_EMPTY_DECL
#endif

#include "soci-backend.h"

namespace soci
{

struct empty_statement_backend;
struct SOCI_EMPTY_DECL empty_standard_into_type_backend : details::standard_into_type_backend
{
    empty_standard_into_type_backend(empty_statement_backend &st)
        : statement_(st) {}

    virtual void define_by_pos(int &position,
        void *data, details::eExchangeType type);

    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void clean_up();

    empty_statement_backend &statement_;
};

struct SOCI_EMPTY_DECL empty_vector_into_type_backend : details::vector_into_type_backend
{
    empty_vector_into_type_backend(empty_statement_backend &st)
        : statement_(st) {}

    virtual void define_by_pos(int &position,
        void *data, details::eExchangeType type);

    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void clean_up();

    empty_statement_backend &statement_;
};

struct SOCI_EMPTY_DECL empty_standard_use_type_backend : details::standard_use_type_backend
{
    empty_standard_use_type_backend(empty_statement_backend &st)
        : statement_(st) {}

    virtual void bind_by_pos(int &position,
        void *data, details::eExchangeType type);
    virtual void bind_by_name(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void pre_use(eIndicator const *ind);
    virtual void post_use(bool gotData, eIndicator *ind);

    virtual void clean_up();

    empty_statement_backend &statement_;
};

struct SOCI_EMPTY_DECL empty_vector_use_type_backend : details::vector_use_type_backend
{
    empty_vector_use_type_backend(empty_statement_backend &st)
        : statement_(st) {}

    virtual void bind_by_pos(int &position,
        void *data, details::eExchangeType type);
    virtual void bind_by_name(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void pre_use(eIndicator const *ind);

    virtual std::size_t size();

    virtual void clean_up();

    empty_statement_backend &statement_;
};

struct empty_session_backend;
struct SOCI_EMPTY_DECL empty_statement_backend : details::statement_backend
{
    empty_statement_backend(empty_session_backend &session);

    virtual void alloc();
    virtual void clean_up();
    virtual void prepare(std::string const &query,
        details::eStatementType eType);

    virtual execFetchResult execute(int number);
    virtual execFetchResult fetch(int number);

    virtual int get_number_of_rows();

    virtual std::string rewrite_for_procedure_call(std::string const &query);

    virtual int prepare_for_describe();
    virtual void describe_column(int colNum, eDataType &dtype,
        std::string &columnName);

    virtual empty_standard_into_type_backend * make_into_type_backend();
    virtual empty_standard_use_type_backend * make_use_type_backend();
    virtual empty_vector_into_type_backend * make_vector_into_type_backend();
    virtual empty_vector_use_type_backend * make_vector_use_type_backend();

    empty_session_backend &session_;
};

struct empty_rowid_backend : details::rowid_backend
{
    empty_rowid_backend(empty_session_backend &session);

    ~empty_rowid_backend();
};

struct empty_blob_backend : details::blob_backend
{
    empty_blob_backend(empty_session_backend &session);

    ~empty_blob_backend();

    virtual std::size_t get_len();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    empty_session_backend &session_;
};

struct empty_session_backend : details::session_backend
{
    empty_session_backend(std::string const &connectString);

    ~empty_session_backend();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void clean_up();

    virtual empty_statement_backend * make_statement_backend();
    virtual empty_rowid_backend * make_rowid_backend();
    virtual empty_blob_backend * make_blob_backend();
};

struct SOCI_EMPTY_DECL empty_backend_factory : backend_factory
{
    virtual empty_session_backend * make_session(
        std::string const &connectString) const;
};

SOCI_EMPTY_DECL extern empty_backend_factory const empty;

} // namespace soci

#endif // SOCI_EMPTY_H_INCLUDED

