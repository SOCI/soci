//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_MYSQL_H_INCLUDED
#define SOCI_MYSQL_H_INCLUDED

#include "soci-common.h"
#include <mysql.h>

namespace SOCI
{

struct MySQLStatementBackEnd;
struct MySQLStandardIntoTypeBackEnd : details::StandardIntoTypeBackEnd
{
    MySQLStandardIntoTypeBackEnd(MySQLStatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, bool calledFromFetch,
        eIndicator *ind);

    virtual void cleanUp();

    MySQLStatementBackEnd &statement_;
    
    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct MySQLVectorIntoTypeBackEnd : details::VectorIntoTypeBackEnd
{
    MySQLVectorIntoTypeBackEnd(MySQLStatementBackEnd &st)
        : statement_(st) {}

    virtual void defineByPos(int &position,
        void *data, details::eExchangeType type);

    virtual void preFetch();
    virtual void postFetch(bool gotData, eIndicator *ind);

    virtual void resize(std::size_t sz);
    virtual std::size_t size();

    virtual void cleanUp();

    MySQLStatementBackEnd &statement_;
    
    void *data_;
    details::eExchangeType type_;
    int position_;
};

struct MySQLStandardUseTypeBackEnd : details::StandardUseTypeBackEnd
{
    MySQLStandardUseTypeBackEnd(MySQLStatementBackEnd &st)
        : statement_(st), position_(0), buf_(NULL) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);
    virtual void postUse(bool gotData, eIndicator *ind);

    virtual void cleanUp();

    MySQLStatementBackEnd &statement_;
    
    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
    char *buf_;
};

struct MySQLVectorUseTypeBackEnd : details::VectorUseTypeBackEnd
{
    MySQLVectorUseTypeBackEnd(MySQLStatementBackEnd &st)
        : statement_(st), position_(0) {}

    virtual void bindByPos(int &position,
        void *data, details::eExchangeType type);
    virtual void bindByName(std::string const &name,
        void *data, details::eExchangeType type);

    virtual void preUse(eIndicator const *ind);

    virtual std::size_t size();

    virtual void cleanUp();

    MySQLStatementBackEnd &statement_;
    
    void *data_;
    details::eExchangeType type_;
    int position_;
    std::string name_;
    std::vector<char *> buffers_;
};

struct MySQLSessionBackEnd;
struct MySQLStatementBackEnd : details::StatementBackEnd
{
    MySQLStatementBackEnd(MySQLSessionBackEnd &session);

    virtual void alloc();
    virtual void cleanUp();
    virtual void prepare(std::string const &query);

    virtual execFetchResult execute(int number);
    virtual execFetchResult fetch(int number);

    virtual int getNumberOfRows();

    virtual std::string rewriteForProcedureCall(std::string const &query);

    virtual int prepareForDescribe();
    virtual void describeColumn(int colNum, eDataType &dtype,
        std::string &columnName);

    virtual MySQLStandardIntoTypeBackEnd * makeIntoTypeBackEnd();
    virtual MySQLStandardUseTypeBackEnd * makeUseTypeBackEnd();
    virtual MySQLVectorIntoTypeBackEnd * makeVectorIntoTypeBackEnd();
    virtual MySQLVectorUseTypeBackEnd * makeVectorUseTypeBackEnd();

    MySQLSessionBackEnd &session_;
    
    MYSQL_RES *result_;
    
    // The query is split into chunks, separated by the named parameters;
    // e.g. for "SELECT id FROM ttt WHERE name = :foo AND gender = :bar"
    // we will have query chunks "SELECT id FROM ttt WHERE name = ",
    // "AND gender = " and names "foo", "bar".
    std::vector<std::string> queryChunks_;
    std::vector<std::string> names_; // list of names for named binds
    
    int numberOfRows_;  // number of rows retrieved from the server
    int currentRow_;    // "current" row number to consume in postFetch
    int rowsToConsume_; // number of rows to be consumed in postFetch
    
    bool justDescribed_; // to optimize row description with immediately
                         // following actual statement execution
    
    // the following maps are used for finding data buffers according to
    // use elements specified by the user

    typedef std::map<int, char **> UseByPosBuffersMap;
    UseByPosBuffersMap useByPosBuffers_;

    typedef std::map<std::string, char **> UseByNameBuffersMap;
    UseByNameBuffersMap useByNameBuffers_;
};

struct MySQLRowIDBackEnd : details::RowIDBackEnd
{
    MySQLRowIDBackEnd(MySQLSessionBackEnd &session);

    ~MySQLRowIDBackEnd();
};

struct MySQLBLOBBackEnd : details::BLOBBackEnd
{
    MySQLBLOBBackEnd(MySQLSessionBackEnd &session);

    ~MySQLBLOBBackEnd();

    virtual std::size_t getLen();
    virtual std::size_t read(std::size_t offset, char *buf,
        std::size_t toRead);
    virtual std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    virtual std::size_t append(char const *buf, std::size_t toWrite);
    virtual void trim(std::size_t newLen);

    MySQLSessionBackEnd &session_;
};

struct MySQLSessionBackEnd : details::SessionBackEnd
{
    MySQLSessionBackEnd(std::string const &connectString);

    ~MySQLSessionBackEnd();

    virtual void begin();
    virtual void commit();
    virtual void rollback();

    void cleanUp();

    virtual MySQLStatementBackEnd * makeStatementBackEnd();
    virtual MySQLRowIDBackEnd * makeRowIDBackEnd();
    virtual MySQLBLOBBackEnd * makeBLOBBackEnd();
    
    MYSQL *conn_;
};

} // namespace SOCI

#endif // SOCI_MYSQL_H_INCLUDED
