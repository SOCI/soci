//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BACKEND_H_INCLUDED
#define SOCI_BACKEND_H_INCLUDED

#include "soci-config.h"

#include "error.h"
#include <cstddef>
#include <map>
#include <string>

namespace soci
{

// data types, as seen by the user
enum eDataType
{
    eString, eChar, eDate, eDouble, eInteger, eUnsignedLong
};

// the enum type for indicator variables
enum eIndicator { eOK, eNoData, eNull, eTruncated };

namespace details
{

// data types, as used to describe exchange format
enum eExchangeType
{
    eXChar, eXCString, eXStdString, eXShort, eXInteger,
    eXUnsignedLong, eXDouble, eXStdTm, eXStatement,
    eXRowID, eXBLOB
};

// type of statement (used for optimizing statement preparation)
enum eStatementType { eOneTimeQuery, eRepeatableQuery };

// polymorphic into type backend

class standard_into_type_backend
{
public:
    virtual ~standard_into_type_backend() {}

    virtual void define_by_pos(int &position,
        void *data, eExchangeType type) = 0;

    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData, bool calledFromFetch,
        eIndicator *ind) = 0;

    virtual void clean_up() = 0;
};

class vector_into_type_backend
{
public:
    virtual ~vector_into_type_backend() {}

    virtual void define_by_pos(int &position,
        void *data, eExchangeType type) = 0;

    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData, eIndicator *ind) = 0;

    virtual void resize(std::size_t sz) = 0;
    virtual std::size_t size() = 0;

    virtual void clean_up() = 0;
};

// polymorphic use type backend

class standard_use_type_backend
{
public:
    virtual ~standard_use_type_backend() {}

    virtual void bind_by_pos(int &position,
        void *data, eExchangeType type) = 0;
    virtual void bind_by_name(std::string const &name,
        void *data, eExchangeType type) = 0;

    virtual void pre_use(eIndicator const *ind) = 0;
    virtual void post_use(bool gotData, eIndicator *ind) = 0;

    virtual void clean_up() = 0;
};

class vector_use_type_backend
{
public:
    virtual ~vector_use_type_backend() {}

    virtual void bind_by_pos(int &position,
        void *data, eExchangeType type) = 0;
    virtual void bind_by_name(std::string const &name,
        void *data, eExchangeType type) = 0;

    virtual void pre_use(eIndicator const *ind) = 0;

    virtual std::size_t size() = 0;

    virtual void clean_up() = 0;
};

// polymorphic statement backend

class statement_backend
{
public:
    virtual ~statement_backend() {}

    virtual void alloc() = 0;
    virtual void clean_up() = 0;

    virtual void prepare(std::string const &query, eStatementType eType) = 0;

    enum execFetchResult { eSuccess, eNoData };
    virtual execFetchResult execute(int number) = 0;
    virtual execFetchResult fetch(int number) = 0;

    virtual int get_number_of_rows() = 0;

    virtual std::string rewrite_for_procedure_call(std::string const &query) = 0;

    virtual int prepare_for_describe() = 0;
    virtual void describe_column(int colNum, eDataType &dtype,
        std::string &column_name) = 0;

    virtual standard_into_type_backend * make_into_type_backend() = 0;
    virtual standard_use_type_backend * make_use_type_backend() = 0;
    virtual vector_into_type_backend * make_vector_into_type_backend() = 0;
    virtual vector_use_type_backend * make_vector_use_type_backend() = 0;
};

// polymorphic RowID backend

class rowid_backend
{
public:
    virtual ~rowid_backend() {}
};

// polymorphic blob backend

class blob_backend
{
public:
    virtual ~blob_backend() {}

    virtual std::size_t get_len() = 0;
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead) = 0;
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite) = 0;
    virtual std::size_t append(char const *buf, std::size_t toWrite) = 0;
    virtual void trim(std::size_t newLen) = 0;
};

// polymorphic session backend

class session_backend
{
public:
    virtual ~session_backend() {}

    virtual void begin() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;

    virtual statement_backend * make_statement_backend() = 0;
    virtual rowid_backend * make_rowid_backend() = 0;
    virtual blob_backend * make_blob_backend() = 0;
};


// helper class used to keep pointer and buffer size as a single object
struct cstring_descriptor
{
    cstring_descriptor(char *str, std::size_t bufSize)
        : str_(str), bufSize_(bufSize) {}

    char *str_;
    std::size_t bufSize_;
};

} // namespace details

// simple base class for the session back-end factory

struct SOCI_DECL backend_factory
{
    virtual ~backend_factory() {}

    virtual details::session_backend * make_session(
        std::string const &connectString) const = 0;
};

} // namespace soci

#endif
