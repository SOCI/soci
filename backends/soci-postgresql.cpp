//
// Copyright (C) 2004, 2005 Maciej Sobczak, Steve Hutton
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "soci.h"
#include "soci-postgresql.h"


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


PostgreSQLSessionBackEnd::PostgreSQLSessionBackEnd(
    std::string const & connectString)
{
    conn_ = PQconnectdb(connectString.c_str());
    if (conn_ == NULL || PQstatus(conn_) != CONNECTION_OK)
    {
        throw SOCIError("Cannot establish connection to the database.");
    }
}

PostgreSQLSessionBackEnd::~PostgreSQLSessionBackEnd()
{
    cleanUp();
}

void PostgreSQLSessionBackEnd::commit()
{
    // TODO:
}

void PostgreSQLSessionBackEnd::rollback()
{
    // TODO:
}

void PostgreSQLSessionBackEnd::cleanUp()
{
    if (conn_ != NULL)
    {
        PQfinish(conn_);
        conn_ = NULL;
    }
}

PostgreSQLStatementBackEnd * PostgreSQLSessionBackEnd::makeStatementBackEnd()
{
    return new PostgreSQLStatementBackEnd(*this);
}

PostgreSQLRowIDBackEnd * PostgreSQLSessionBackEnd::makeRowIDBackEnd()
{
    return new PostgreSQLRowIDBackEnd(*this);
}

PostgreSQLBLOBBackEnd * PostgreSQLSessionBackEnd::makeBLOBBackEnd()
{
    return new PostgreSQLBLOBBackEnd(*this);
}

PostgreSQLStatementBackEnd::PostgreSQLStatementBackEnd(
    PostgreSQLSessionBackEnd &session)
    : session_(session), result_(NULL)
{
}

void PostgreSQLStatementBackEnd::alloc()
{
    // TODO:
}

void PostgreSQLStatementBackEnd::cleanUp()
{
    if (result_ != NULL)
    {
        PQclear(result_);
        result_ = NULL;
    }
}

void PostgreSQLStatementBackEnd::prepare(std::string const &query)
{
    query_ = query;
}

StatementBackEnd::execFetchResult
PostgreSQLStatementBackEnd::execute(int number)
{
    if (number != 1)
    {
        throw SOCIError("Only unit excutions are supported.");
    }

    result_ = PQexec(session_.conn_, query_.c_str());
    if (result_ == NULL)
    {
        throw SOCIError("Cannot execute query.");
    }
    
    ExecStatusType status = PQresultStatus(result_);
    if (status == PGRES_TUPLES_OK)
    {
        return eSuccess;
    }
    else if (status == PGRES_COMMAND_OK)
    {
        return eNoData;
    }
    else
    {
        throw SOCIError(PQresultErrorMessage(result_));
    }
}

StatementBackEnd::execFetchResult
PostgreSQLStatementBackEnd::fetch(int number)
{
    // TODO:
    return eSuccess;
}

int PostgreSQLStatementBackEnd::getNumberOfRows()
{
    return PQntuples(result_);
}

int PostgreSQLStatementBackEnd::prepareForDescribe()
{
    // TODO:
    return 0;
}

void PostgreSQLStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName, int &size, int &precision, int &scale,
    bool &nullOk)
{
    // TODO:
}

PostgreSQLStandardIntoTypeBackEnd *
PostgreSQLStatementBackEnd::makeIntoTypeBackEnd()
{
    return new PostgreSQLStandardIntoTypeBackEnd(*this);
}

PostgreSQLStandardUseTypeBackEnd *
PostgreSQLStatementBackEnd::makeUseTypeBackEnd()
{
    return new PostgreSQLStandardUseTypeBackEnd(*this);
}

PostgreSQLVectorIntoTypeBackEnd *
PostgreSQLStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new PostgreSQLVectorIntoTypeBackEnd(*this);
}

PostgreSQLVectorUseTypeBackEnd *
PostgreSQLStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new PostgreSQLVectorUseTypeBackEnd(*this);
}

void PostgreSQLStandardIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLStandardIntoTypeBackEnd::preFetch()
{
    // TODO:
}

void PostgreSQLStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator *ind)
{
    // TODO:
}

void PostgreSQLStandardIntoTypeBackEnd::cleanUp()
{
    // TODO:
}

void PostgreSQLVectorIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLVectorIntoTypeBackEnd::preFetch()
{
    // TODO:
}

void PostgreSQLVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    // TODO:
}

void PostgreSQLVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    // TODO:
}

std::size_t PostgreSQLVectorIntoTypeBackEnd::size()
{
    // TODO:
    return 0;
}

void PostgreSQLVectorIntoTypeBackEnd::cleanUp()
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::cleanUp()
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // TODO:
}

std::size_t PostgreSQLVectorUseTypeBackEnd::size()
{
    // TODO:
    return 0;
}

void PostgreSQLVectorUseTypeBackEnd::cleanUp()
{
    // TODO:
}

PostgreSQLRowIDBackEnd::PostgreSQLRowIDBackEnd(
    PostgreSQLSessionBackEnd &session)
{
    // TODO:
}

PostgreSQLRowIDBackEnd::~PostgreSQLRowIDBackEnd()
{
    // TODO:
}

PostgreSQLBLOBBackEnd::PostgreSQLBLOBBackEnd(
    PostgreSQLSessionBackEnd &session)
    : session_(session)
{
    // TODO:
}

PostgreSQLBLOBBackEnd::~PostgreSQLBLOBBackEnd()
{
    // TODO:
}

std::size_t PostgreSQLBLOBBackEnd::getLen()
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::read(
    std::size_t offset, char *buf, std::size_t toRead)
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::append(
    char const *buf, std::size_t toWrite)
{
    // TODO:
    return 0;
}

void PostgreSQLBLOBBackEnd::trim(std::size_t newLen)
{
    // TODO:
}


// concrete factory for PostgreSQL concrete strategies
struct PostgreSQLBackEndFactory : BackEndFactory
{
    virtual PostgreSQLSessionBackEnd * makeSession(
        std::string const &connectString) const
    {
        return new PostgreSQLSessionBackEnd(connectString);
    }

} postgresqlBEF;

namespace
{

// global object for automatic factory registration
struct PostgreSQLAutoRegister
{
    PostgreSQLAutoRegister()
    {
        theBEFRegistry().registerMe("postgresql", &postgresqlBEF);
    }
} postgresqlAutoRegister;

} // namespace anonymous
