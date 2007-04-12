//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ODBC_H_INCLUDED
#define SOCI_ODBC_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_ODBC_SOURCE
#   define SOCI_ODBC_DECL __declspec(dllexport)
#  else
#   define SOCI_ODBC_DECL __declspec(dllimport)
#  endif // SOCI_ODBC_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_ODBC_DECL isn't defined yet define it now
#ifndef SOCI_ODBC_DECL
# define SOCI_ODBC_DECL
#endif

#include <soci-backend.h>
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <sqlext.h> // ODBC
#include <vector>

namespace soci
{

struct odbc_statement_backend;
struct odbc_standard_into_type_backend : details::standard_into_type_backend
{
    odbc_standard_into_type_backend(odbc_statement_backend &st)
        : statement_(st), buf_(0)
    {}

    virtual void define_by_pos(int &position,
        void *data, details::eExchangeType type);

    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void clean_up();

    odbc_statement_backend &statement_;
    char *buf_;        // generic buffer
    void *data_;
    details::eExchangeType type_;
    int position_;
    SQLSMALLINT odbcType_;
    SQLINTEGER valueLen_;
};

struct odbc_vector_into_type_backend : details::vector_into_type_backend
{
    odbc_vector_into_type_backend(odbc_statement_backend &st)
        : statement_(st), indHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    virtual void define_by_pos(int &position,
        void *data, details::eExchangeType type);

    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void clean_up();

    // helper function for preparing indicators
    // (as part of the define_by_pos)
    void prepare_indicators(std::size_t size);

    odbc_statement_backend &statement_;

    SQLLEN *indHolders_;
    std::vector<SQLINTEGER> indHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::eExchangeType type_;
    std::size_t colSize_;    // size of the string column (used for strings)
    SQLSMALLINT odbcType_;
};

struct odbc_standard_use_type_backend : details::standard_use_type_backend
{
    odbc_standard_use_type_backend(odbc_statement_backend &st)
        : statement_(st), data_(0), buf_(0), indHolder_(0) {}

    void prepare_for_bind(void *&data, SQLUINTEGER &size, 
                        SQLSMALLINT &sqlType, SQLSMALLINT &cType);
    void bind_helper(int &position,
        void *data, details::eExchangeType type);

    virtual void bind_by_pos(int &position,
        void *data, details::eExchangeType type);
    virtual void bind_by_name(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void pre_use(eIndicator const *ind);
    virtual void post_use(bool gotData, eIndicator *ind);

    virtual void clean_up();

    odbc_statement_backend &statement_;
    void *data_;
    details::eExchangeType type_;
    char *buf_;
    SQLINTEGER indHolder_;
};

struct odbc_vector_use_type_backend : details::vector_use_type_backend
{
    odbc_vector_use_type_backend(odbc_statement_backend &st)
        : statement_(st), indHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    // helper function for preparing indicators
    // (as part of the define_by_pos)
    void prepare_indicators(std::size_t size);

    // common part for bind_by_pos and bind_by_name
    void prepare_for_bind(void *&data, SQLUINTEGER &size, SQLSMALLINT &sqlType, SQLSMALLINT &cType);
    void bind_helper(int &position,
        void *data, details::eExchangeType type);

    virtual void bind_by_pos(int &position,
        void *data, details::eExchangeType type);
    virtual void bind_by_name(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void pre_use(eIndicator const *ind);

    virtual std::size_t size();

    virtual void clean_up();

    odbc_statement_backend &statement_;

    SQLLEN *indHolders_;
    std::vector<SQLLEN> indHolderVec_;
    void *data_;
    details::eExchangeType type_;
    char *buf_;              // generic buffer
    std::size_t colSize_;    // size of the string column (used for strings)
    // used for strings only
    std::size_t maxSize_;
};

struct odbc_session_backend;
struct odbc_statement_backend : details::statement_backend
{
    odbc_statement_backend(odbc_session_backend &session);

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

    // helper for defining into vector<string>
    std::size_t column_size(int position);

    virtual odbc_standard_into_type_backend * make_into_type_backend();
    virtual odbc_standard_use_type_backend * make_use_type_backend();
    virtual odbc_vector_into_type_backend * make_vector_into_type_backend();
    virtual odbc_vector_use_type_backend * make_vector_use_type_backend();

    odbc_session_backend &session_;
    SQLHSTMT hstmt_;
	SQLUINTEGER	numRowsFetched_;
    bool hasVectorUseElements_;
    bool boundByName_;
    bool boundByPos_;

    std::string query_;
    std::vector<std::string> names_; // list of names for named binds

};

struct odbc_rowid_backend : details::rowid_backend
{
    odbc_rowid_backend(odbc_session_backend &session);

    ~odbc_rowid_backend();
};

struct odbc_blob_backend : details::blob_backend
{
    odbc_blob_backend(odbc_session_backend &session);

    ~odbc_blob_backend();

    virtual std::size_t get_len();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    odbc_session_backend &session_;
};

struct odbc_session_backend : details::session_backend
{
    odbc_session_backend(std::string const &connectString);

    ~odbc_session_backend();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void reset_transaction();

    void clean_up();

    virtual odbc_statement_backend * make_statement_backend();
    virtual odbc_rowid_backend * make_rowid_backend();
    virtual odbc_blob_backend * make_blob_backend();

    SQLHENV henv_;
    SQLHDBC hdbc_;
};

struct odbc_backend_factory : backend_factory
{
    virtual odbc_session_backend * make_session(
        std::string const &connectString) const;
};

SOCI_ODBC_DECL extern odbc_backend_factory const odbc;

class SOCI_ODBC_DECL odbc_soci_error : public soci_error
{
    SQLCHAR message_[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate_[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode_;
    
public:
    odbc_soci_error(SQLSMALLINT htype, 
                  SQLHANDLE hndl, 
                  std::string const & msg) 
        : soci_error(msg)
    {
        SQLSMALLINT length, i = 1;
        SQLGetDiagRec(htype, hndl, i, sqlstate_, &sqlcode_,
                      message_, SQL_MAX_MESSAGE_LENGTH + 1,
                      &length);
        
    }
        
    SQLCHAR const * odbc_error_code() const
    {
        return reinterpret_cast<SQLCHAR const *>(sqlstate_);
    }
    SQLINTEGER native_error_code() const
    {
        return sqlcode_;
    }
    SQLCHAR const * odbc_error_message() const
    {
        return reinterpret_cast<SQLCHAR const *>(message_);
    }
};

inline bool is_odbc_error(SQLRETURN rc)
{
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        return true;
    else
        return false;
}

} // namespace SOCI


#endif // SOCI_EMPTY_H_INCLUDED

