//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_POSTGRESQL_H_INCLUDED
#define SOCI_POSTGRESQL_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_POSTGRESQL_SOURCE
#   define SOCI_POSTGRESQL_DECL __declspec(dllexport)
#  else
#   define SOCI_POSTGRESQL_DECL __declspec(dllimport)
#  endif // SOCI_POSTGRESQL_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_POSTGRESQL_DECL isn't defined yet define it now
#ifndef SOCI_POSTGRESQL_DECL
# define SOCI_POSTGRESQL_DECL
#endif

#include <soci-backend.h>
#include <libpq-fe.h>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4512 4511)
#endif


namespace SOCI
{

struct PostgreSQLStatementBackEnd;
struct PostgreSQLStandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
{
    PostgreSQLStandardIntoTypeBackEnd(PostgreSQLStatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void cleanUp();

    PostgreSQLStatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct PostgreSQLVectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
{
    PostgreSQLVectorIntoTypeBackEnd(PostgreSQLStatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void cleanUp();

    PostgreSQLStatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct PostgreSQLStandardUseTypeBackEnd : details::StandardUseTypeBackEnd
{
    PostgreSQLStandardUseTypeBackEnd(PostgreSQLStatementBackEnd &st)
        : statement_(st), position_(0), buf_(NULL) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);
    virtual void postUse(bool gotData, eIndicator *ind);

    virtual void cleanUp();

    PostgreSQLStatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
    char *buf_;
};

struct PostgreSQLVectorUseTypeBackEnd : details::VectorUseTypeBackEnd
{
    PostgreSQLVectorUseTypeBackEnd(PostgreSQLStatementBackEnd &st)
        : statement_(st), position_(0) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);

    virtual std::size_t size();

    virtual void cleanUp();

    PostgreSQLStatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
    std::vector<char *> buffers_;
};

struct PostgreSQLSessionBackEnd;
struct PostgreSQLStatementBackEnd : details::StatementBackEnd
{
    PostgreSQLStatementBackEnd(PostgreSQLSessionBackEnd &session);

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

    virtual PostgreSQLStandardIntoTypeBackEnd * makeIntoTypeBackEnd();
    virtual PostgreSQLStandardUseTypeBackEnd * makeUseTypeBackEnd();
    virtual PostgreSQLVectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
    virtual PostgreSQLVectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

    PostgreSQLSessionBackEnd &session_;

    PGresult *result_;
    std::string query_;
    details::eStatementType eType_;
    std::string statementName_;
    std::vector<std::string> names_; // list of names for named binds

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
};

struct PostgreSQLRowIDBackEnd : details::RowIDBackEnd
{
    PostgreSQLRowIDBackEnd(PostgreSQLSessionBackEnd &session);

    ~PostgreSQLRowIDBackEnd();

    unsigned long value_;
};

struct PostgreSQLBLOBBackEnd : details::BLOBBackEnd
{
    PostgreSQLBLOBBackEnd(PostgreSQLSessionBackEnd &session);

    ~PostgreSQLBLOBBackEnd();

    virtual std::size_t getLen();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    PostgreSQLSessionBackEnd &session_;

    unsigned long oid_; // oid of the large object
    int fd_;            // descriptor of the large object
};

struct PostgreSQLSessionBackEnd : details::SessionBackEnd
{
    PostgreSQLSessionBackEnd(std::string const &connectString);

    ~PostgreSQLSessionBackEnd();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void cleanUp();

    virtual PostgreSQLStatementBackEnd * makeStatementBackEnd();
    virtual PostgreSQLRowIDBackEnd * makeRowIDBackEnd();
    virtual PostgreSQLBLOBBackEnd * makeBLOBBackEnd();

    std::string getNextStatementName();

    int statementCount_;
    PGconn *conn_;
};


struct PostgreSQLBackEndFactory : BackEndFactory
{
    virtual PostgreSQLSessionBackEnd * makeSession(
        std::string const &connectString) const;
};

SOCI_POSTGRESQL_DECL extern PostgreSQLBackEndFactory const postgresql;

} // namespace SOCI

#endif // SOCI_POSTGRESQL_H_INCLUDED

