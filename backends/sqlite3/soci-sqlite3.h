//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_SQLITE3_H_INCLUDED
#define SOCI_SQLITE3_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_SQLITE3_SOURCE
#   define SOCI_SQLITE3_DECL __declspec(dllexport)
#  else
#   define SOCI_SQLITE3_DECL __declspec(dllimport)
#  endif // SOCI_SQLITE3_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_SQLITE3_DECL isn't defined yet define it now
#ifndef SOCI_SQLITE3_DECL
# define SOCI_SQLITE3_DECL
#endif

#include "soci-backend.h"

// Disable flood of nonsense warnings generated for SQLite
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4510 4610)
#endif

namespace sqlite_api
{
#include <sqlite3.h>
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif


namespace SOCI
{

struct Sqlite3StatementBackEnd;
struct Sqlite3StandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
{
    Sqlite3StandardIntoTypeBackEnd(Sqlite3StatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
                             void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch,
                           eIndicator *ind);

    virtual void cleanUp();

    Sqlite3StatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct Sqlite3VectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
{
    Sqlite3VectorIntoTypeBackEnd(Sqlite3StatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
                             void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void cleanUp();

    Sqlite3StatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct Sqlite3StandardUseTypeBackEnd : details::StandardUseTypeBackEnd
{
    Sqlite3StandardUseTypeBackEnd(Sqlite3StatementBackEnd &st)
        : statement_(st), buf_(0) {}

    virtual void bindByPos(int &position,
                           void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
                            void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);
    virtual void postUse(bool gotData, eIndicator *ind);

    virtual void cleanUp();

    Sqlite3StatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
    char *buf_;
};

struct Sqlite3VectorUseTypeBackEnd : details::VectorUseTypeBackEnd
{
    Sqlite3VectorUseTypeBackEnd(Sqlite3StatementBackEnd &st)
        : statement_(st) {}

    virtual void bindByPos(int &position,
                           void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
                            void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);

    virtual std::size_t size();

    virtual void cleanUp();

    Sqlite3StatementBackEnd &statement_;

    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
};

struct Sqlite3Column
{
    std::string data_;
    bool isNull_;
};

typedef std::vector<Sqlite3Column> Sqlite3Row;
typedef std::vector<Sqlite3Row> Sqlite3RecordSet;

struct Sqlite3SessionBackEnd;
struct Sqlite3StatementBackEnd : details::StatementBackEnd
{
    Sqlite3StatementBackEnd(Sqlite3SessionBackEnd &session);

    virtual void alloc();
    virtual void cleanUp();
    virtual void prepare(std::string const &query,
        details::eStatementType eType);
    void resetIfNeeded();

    virtual execFetchResult execute(int number);
    virtual execFetchResult fetch(int number);

    virtual int getNumberOfRows();

    virtual std::string rewriteForProcedureCall(std::string const &query);

    virtual int prepareForDescribe();
    virtual void describeColumn(int colNum, eDataType &dtype,
                                std::string &columnName);

    virtual Sqlite3StandardIntoTypeBackEnd * makeIntoTypeBackEnd();
    virtual Sqlite3StandardUseTypeBackEnd * makeUseTypeBackEnd();
    virtual Sqlite3VectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
    virtual Sqlite3VectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

    Sqlite3SessionBackEnd &session_;
    sqlite_api::sqlite3_stmt *stmt_;
    Sqlite3RecordSet dataCache_;
    Sqlite3RecordSet useData_;
    bool databaseReady_;
    bool boundByName_;
    bool boundByPos_;

private:
    execFetchResult loadRS(int totalRows);
    execFetchResult loadOne();
    execFetchResult bindAndExecute(int number);
};

struct Sqlite3RowIDBackEnd : details::RowIDBackEnd
{
    Sqlite3RowIDBackEnd(Sqlite3SessionBackEnd &session);

    ~Sqlite3RowIDBackEnd();

    unsigned long value_;
};

struct Sqlite3BLOBBackEnd : details::BLOBBackEnd
{
    Sqlite3BLOBBackEnd(Sqlite3SessionBackEnd &session);

    ~Sqlite3BLOBBackEnd();

    void setData(const char *tableName, const char *columnName,
                 const char *buf, size_t len);

    virtual std::size_t getLen();
    virtual std::size_t read(std::size_t offset, char *buf,
                             std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
                              std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    Sqlite3SessionBackEnd &session_;
private:

    void updateBLOB();

    std::string tableName_;
    std::string columnName_;
    char *buf_;
    size_t len_;
};

struct Sqlite3SessionBackEnd : details::SessionBackEnd
{
    Sqlite3SessionBackEnd(std::string const &connectString);

    ~Sqlite3SessionBackEnd();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void cleanUp();

    virtual Sqlite3StatementBackEnd * makeStatementBackEnd();
    virtual Sqlite3RowIDBackEnd * makeRowIDBackEnd();
    virtual Sqlite3BLOBBackEnd * makeBLOBBackEnd();

    sqlite_api::sqlite3 *conn_;
};

struct Sqlite3BackEndFactory : BackEndFactory
{
    virtual Sqlite3SessionBackEnd * makeSession(
        std::string const &connectString) const;
};

SOCI_SQLITE3_DECL extern Sqlite3BackEndFactory const sqlite3;

} // namespace SOCI

#endif // SOCI_SQLITE3_H_INCLUDED

