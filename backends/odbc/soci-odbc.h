//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ODBC_H_INCLUDED
#define SOCI_ODBC_H_INCLUDED

#include "soci-backend.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <sqlext.h>

namespace SOCI
{

struct ODBCStatementBackEnd;
struct ODBCStandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
{
    ODBCStandardIntoTypeBackEnd(ODBCStatementBackEnd &st)
        : statement_(st), buf_(0)
    {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void cleanUp();

    ODBCStatementBackEnd &statement_;
    char *buf_;        // generic buffer
    void *data_;
    details::eExchangeType type_;
    int position_;
    SQLSMALLINT odbcType_;
    SQLINTEGER valueLen_;
};

struct ODBCVectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
{
    ODBCVectorIntoTypeBackEnd(ODBCStatementBackEnd &st)
        : statement_(st), indHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void cleanUp();

    // helper function for preparing indicators
    // (as part of the defineByPos)
    void prepareIndicators(std::size_t size);

    ODBCStatementBackEnd &statement_;

    SQLLEN *indHolders_;
    std::vector<SQLINTEGER> indHolderVec_;
    void *data_;
    char *buf_;              // generic buffer
    details::eExchangeType type_;
    std::size_t colSize_;    // size of the string column (used for strings)
    SQLSMALLINT odbcType_;
};

struct ODBCStandardUseTypeBackEnd : details::StandardUseTypeBackEnd
{
    ODBCStandardUseTypeBackEnd(ODBCStatementBackEnd &st)
        : statement_(st), data_(0), buf_(0), indHolder_(0) {}

    void prepareForBind(void *&data, SQLUINTEGER &size, 
                        SQLSMALLINT &sqlType, SQLSMALLINT &cType);
    void bindHelper(int &position,
        void *data, details::eExchangeType type);

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);
    virtual void postUse(bool gotData, eIndicator *ind);

    virtual void cleanUp();

    ODBCStatementBackEnd &statement_;
    void *data_;
    details::eExchangeType type_;
    char *buf_;
    SQLINTEGER indHolder_;
};

struct ODBCVectorUseTypeBackEnd : details::VectorUseTypeBackEnd
{
    ODBCVectorUseTypeBackEnd(ODBCStatementBackEnd &st)
        : statement_(st), indHolders_(NULL),
          data_(NULL), buf_(NULL) {}

    // helper function for preparing indicators
    // (as part of the defineByPos)
    void prepareIndicators(std::size_t size);

    // common part for bindByPos and bindByName
    void prepareForBind(void *&data, SQLUINTEGER &size, SQLSMALLINT &sqlType, SQLSMALLINT &cType);
    void bindHelper(int &position,
        void *data, details::eExchangeType type);

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);

    virtual std::size_t size();

    virtual void cleanUp();

    ODBCStatementBackEnd &statement_;

    SQLLEN *indHolders_;
    std::vector<SQLLEN> indHolderVec_;
    void *data_;
    details::eExchangeType type_;
    char *buf_;              // generic buffer
    std::size_t colSize_;    // size of the string column (used for strings)
    // used for strings only
    std::size_t maxSize_;
};

struct ODBCSessionBackEnd;
struct ODBCStatementBackEnd : details::StatementBackEnd
{
    ODBCStatementBackEnd(ODBCSessionBackEnd &session);

    virtual void alloc();
    virtual void cleanUp();
    virtual void prepare(std::string const &query,
        details::eStatementType eType);

    virtual execFetchResult execute(int number);
    virtual execFetchResult fetch(int number);

    virtual int getNumberOfRows();

    virtual std::string rewriteForProcedureCall(std::string const &query);

    virtual int prepareForDescribe();
    virtual void describeColumn(int colNum, eDataType &dtype,
        std::string &columnName);

    // helper for defining into vector<string>
    std::size_t columnSize(int position);

    virtual ODBCStandardIntoTypeBackEnd * makeIntoTypeBackEnd();
    virtual ODBCStandardUseTypeBackEnd * makeUseTypeBackEnd();
    virtual ODBCVectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
    virtual ODBCVectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

    ODBCSessionBackEnd &session_;
    SQLHSTMT hstmt_;
	SQLUINTEGER	numRowsFetched_;
    bool hasVectorUseElements_;
    bool boundByName_;
    bool boundByPos_;

    std::string query_;
    std::vector<std::string> names_; // list of names for named binds

};

struct ODBCRowIDBackEnd : details::RowIDBackEnd
{
    ODBCRowIDBackEnd(ODBCSessionBackEnd &session);

    ~ODBCRowIDBackEnd();
};

struct ODBCBLOBBackEnd : details::BLOBBackEnd
{
    ODBCBLOBBackEnd(ODBCSessionBackEnd &session);

    ~ODBCBLOBBackEnd();

    virtual std::size_t getLen();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    ODBCSessionBackEnd &session_;
};

struct ODBCSessionBackEnd : details::SessionBackEnd
{
    ODBCSessionBackEnd(std::string const &connectString);

    ~ODBCSessionBackEnd();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void reset_transaction();

    void cleanUp();

    virtual ODBCStatementBackEnd * makeStatementBackEnd();
    virtual ODBCRowIDBackEnd * makeRowIDBackEnd();
    virtual ODBCBLOBBackEnd * makeBLOBBackEnd();

    SQLHENV henv_;
    SQLHDBC hdbc_;
};

struct ODBCBackEndFactory : BackEndFactory
{
    virtual ODBCSessionBackEnd * makeSession(
        std::string const &connectString) const;
};

extern ODBCBackEndFactory const odbc;

class ODBCSOCIError : public SOCIError
{
    SQLCHAR message_[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate_[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode_;
    
public:
    ODBCSOCIError(SQLSMALLINT htype, 
                  SQLHANDLE hndl, 
                  std::string const & msg) 
        : SOCIError(msg)
    {
        SQLSMALLINT length, i = 1;
        SQLGetDiagRec(htype, hndl, i, sqlstate_, &sqlcode_,
                      message_, SQL_MAX_MESSAGE_LENGTH + 1,
                      &length);
        
    }
        
    SQLCHAR const * odbcErrorCode() const
    {
        return reinterpret_cast<SQLCHAR const *>(sqlstate_);
    }
    SQLINTEGER nativeErrorCode() const
    {
        return sqlcode_;
    }
    SQLCHAR const * odbcErrorMessage() const
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
