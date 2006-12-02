//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include <soci.h>
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef SOCI_PGSQL_NOPARAMS
#define SOCI_PGSQL_NOBINDBYNAME
#endif // SOCI_PGSQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


PostgreSQLStatementBackEnd::PostgreSQLStatementBackEnd(
    PostgreSQLSessionBackEnd &session)
     : session_(session), result_(NULL), justDescribed_(false),
       hasIntoElements_(false), hasVectorIntoElements_(false),
       hasUseElements_(false), hasVectorUseElements_(false)
{
}

void PostgreSQLStatementBackEnd::alloc()
{
    // nothing to do here
}

void PostgreSQLStatementBackEnd::cleanUp()
{
    if (result_ != NULL)
    {
        PQclear(result_);
        result_ = NULL;
    }
}

void PostgreSQLStatementBackEnd::prepare(std::string const &query,
    eStatementType eType)
{
#ifdef SOCI_PGSQL_NOBINDBYNAME
    query_ = query;
#else
    // rewrite the query by transforming all named parameters into
    // the PostgreSQL numbers ones (:abc -> $1, etc.)

    enum { eNormal, eInQuotes, eInName } state = eNormal;

    std::string name;
    int position = 1;

    for (std::string::const_iterator it = query.begin(), end = query.end();
         it != end; ++it)
    {
        switch (state)
        {
        case eNormal:
            if (*it == '\'')
            {
                query_ += *it;
                state = eInQuotes;
            }
            else if (*it == ':')
            {
                state = eInName;
            }
            else // regular character, stay in the same state
            {
                query_ += *it;
            }
            break;
        case eInQuotes:
            if (*it == '\'')
            {
                query_ += *it;
                state = eNormal;
            }
            else // regular quoted character
            {
                query_ += *it;
            }
            break;
        case eInName:
            if (std::isalnum(*it) || *it == '_')
            {
                name += *it;
            }
            else // end of name
            {
                names_.push_back(name);
                name.clear();
                std::ostringstream ss;
                ss << '$' << position++;
                query_ += ss.str();
                query_ += *it;
                state = eNormal;
            }
            break;
        }
    }

    if (state == eInName)
    {
        names_.push_back(name);
        std::ostringstream ss;
        ss << '$' << position++;
        query_ += ss.str();
    }

#endif // SOCI_PGSQL_NOBINDBYNAME

#ifndef SOCI_PGSQL_NOPREPARE

    if (eType == eRepeatableQuery)
    {
        statementName_ = session_.getNextStatementName();

        PGresult *res = PQprepare(session_.conn_, statementName_.c_str(),
            query_.c_str(), static_cast<int>(names_.size()), NULL);
        if (res == NULL)
        {
            throw SOCIError("Cannot prepare statement.");
        }
        ExecStatusType status = PQresultStatus(res);
        if (status != PGRES_COMMAND_OK)
        {
            throw SOCIError(PQresultErrorMessage(res));
        }
        PQclear(res);
    }

    eType_ = eType;

#endif // SOCI_PGSQL_NOPREPARE
}

StatementBackEnd::execFetchResult
PostgreSQLStatementBackEnd::execute(int number)
{
    // If the statement was "just described", then we know that
    // it was actually executed with all the use elements
    // already bound and pre-used. This means that the result of the
    // query is already on the client side, so there is no need
    // to re-execute it.

    if (justDescribed_ == false)
    {
        // This object could have been already filled with data before.
        cleanUp();

        if (number > 1 && hasIntoElements_)
        {
             throw SOCIError(
                  "Bulk use with single into elements is not supported.");
        }

        // Since the bulk operations are not natively supported by PostgreSQL,
        // we have to explicitly loop to achieve the bulk operations.
        // On the other hand, looping is not needed if there are single
        // use elements, even if there is a bulk fetch.
        // We know that single use and bulk use elements in the same query are
        // not supported anyway, so in the effect the 'number' parameter here
        // specifies the size of vectors (into/use), but 'numberOfExecutions'
        // specifies the number of loops that need to be performed.
        
        int numberOfExecutions = 1;
        if (number > 0)
        {
             numberOfExecutions = hasUseElements_ ? 1 : number;
        }

        if (!useByPosBuffers_.empty() || !useByNameBuffers_.empty())
        {
            if (!useByPosBuffers_.empty() && !useByNameBuffers_.empty())
            {
                throw SOCIError(
                    "Binding for use elements must be either by position "
                    "or by name.");
            }

            for (int i = 0; i != numberOfExecutions; ++i)
            {
                std::vector<char *> paramValues;

                if (!useByPosBuffers_.empty())
                {
                    // use elements bind by position
                    // the map of use buffers can be traversed
                    // in its natural order

                    for (UseByPosBuffersMap::iterator
                             it = useByPosBuffers_.begin(),
                             end = useByPosBuffers_.end();
                         it != end; ++it)
                    {
                        char **buffers = it->second;
                        paramValues.push_back(buffers[i]);
                    }
                }
                else
                {
                    // use elements bind by name

                    for (std::vector<std::string>::iterator
                             it = names_.begin(), end = names_.end();
                         it != end; ++it)
                    {
                        UseByNameBuffersMap::iterator b
                            = useByNameBuffers_.find(*it);
                        if (b == useByNameBuffers_.end())
                        {
                            std::string msg(
                                "Missing use element for bind by name (");
                            msg += *it;
                            msg += ").";
                            throw SOCIError(msg);
                        }
                        char **buffers = b->second;
                        paramValues.push_back(buffers[i]);
                    }
                }

#ifdef SOCI_PGSQL_NOPARAMS

                throw SOCIError("Queries with parameters are not supported.");

#else

#ifdef SOCI_PGSQL_NOPREPARE

                result_ = PQexecParams(session_.conn_, query_.c_str(),
                    static_cast<int>(paramValues.size()),
                    NULL, &paramValues[0], NULL, NULL, 0);

#else

                if (eType_ == eRepeatableQuery)
                {
                    // this query was separately prepared

                    result_ = PQexecPrepared(session_.conn_,
                        statementName_.c_str(),
                        static_cast<int>(paramValues.size()),
                        &paramValues[0], NULL, NULL, 0);
                }
                else // eType_ == eOneTimeQuery
                {
                    // this query was not separately prepared and should
                    // be executed as a one-time query

                    result_ = PQexecParams(session_.conn_, query_.c_str(),
                        static_cast<int>(paramValues.size()),
                        NULL, &paramValues[0], NULL, NULL, 0);
                }

#endif // SOCI_PGSQL_NOPREPARE

#endif // SOCI_PGSQL_NOPARAMS

                if (numberOfExecutions > 1)
                {
                    // there are only bulk use elements (no intos)
                    if (result_ == NULL)
                    {
                        throw SOCIError("Cannot execute query.");
                    }

                    ExecStatusType status = PQresultStatus(result_);
                    if (status != PGRES_COMMAND_OK)
                    {
                        throw SOCIError(PQresultErrorMessage(result_));
                    }
                    PQclear(result_);
                }
            }

            if (numberOfExecutions > 1)
            {
                // it was a bulk operation
                result_ = NULL;
                return eNoData;
            }

            // otherwise (no bulk), follow the code below
        }
        else
        {
            // there are no use elements
            // - execute the query without parameter information

#ifdef SOCI_PGSQL_NOPREPARE

            result_ = PQexec(session_.conn_, query_.c_str());
#else

            if (eType_ == eRepeatableQuery)
            {
                // this query was separately prepared

                result_ = PQexecPrepared(session_.conn_,
                    statementName_.c_str(), 0, NULL, NULL, NULL, 0);
            }
            else // eType_ == eOneTimeQuery
            {
                result_ = PQexec(session_.conn_, query_.c_str());
            }

#endif // SOCI_PGSQL_NOPREPARE

            if (result_ == NULL)
            {
                throw SOCIError("Cannot execute query.");
            }
        }
    }
    else
    {
        // The optimization based on the existing results
        // from the row description can be performed only once.
        // If the same statement is re-executed,
        // it will be *really* re-executed, without reusing existing data.

        justDescribed_ = false;
    }

    ExecStatusType status = PQresultStatus(result_);
    if (status == PGRES_TUPLES_OK)
    {
        currentRow_ = 0;
        rowsToConsume_ = 0;

        numberOfRows_ = PQntuples(result_);
        if (numberOfRows_ == 0)
        {
            return eNoData;
        }
        else
        {
            if (number > 0)
            {
                // prepare for the subsequent data consumption
                return fetch(number);
            }
            else
            {
                // execute(0) was meant to only perform the query
                return eSuccess;
            }
        }
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
    // Note: This function does not actually fetch anything from anywhere
    // - the data was already retrieved from the server in the execute()
    // function, and the actual consumption of this data will take place
    // in the postFetch functions, called for each into element.
    // Here, we only prepare for this to happen (to emulate "the Oracle way").

    // forward the "cursor" from the last fetch
    currentRow_ += rowsToConsume_;

    if (currentRow_ >= numberOfRows_)
    {
        // all rows were already consumed
        return eNoData;
    }
    else
    {
        if (currentRow_ + number > numberOfRows_)
        {
            rowsToConsume_ = numberOfRows_ - currentRow_;

            // this simulates the behaviour of Oracle
            // - when EOF is hit, we return eNoData even when there are
            // actually some rows fetched
            return eNoData;
        }
        else
        {
            rowsToConsume_ = number;
            return eSuccess;
        }
    }
}

int PostgreSQLStatementBackEnd::getNumberOfRows()
{
    return numberOfRows_ - currentRow_;
}

std::string PostgreSQLStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    std::string newQuery("select ");
    newQuery += query;
    return newQuery;
}

int PostgreSQLStatementBackEnd::prepareForDescribe()
{
    execute(1);
    justDescribed_ = true;

    int columns = PQnfields(result_);
    return columns;
}

void PostgreSQLStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName)
{
    // In PostgreSQL column numbers start from 0
    int pos = colNum - 1;

    unsigned long typeOid = PQftype(result_, pos);
    switch (typeOid)
    {
    // Note: the following list of OIDs was taken from the pg_type table
    // we do not claim that this list is exchaustive or even correct.

               // from pg_type:

    case 25:   // text
    case 1043: // varchar
    case 2275: // cstring
    case 18:   // char
    case 1042: // bpchar
        type = eString;
        break;

    case 702:  // abstime
    case 703:  // reltime
    case 1082: // date
    case 1083: // time
    case 1114: // timestamp
    case 1184: // timestamptz
    case 1266: // timetz
        type = eDate;
        break;

    case 700:  // float4
    case 701:  // float8
    case 1700: // numeric
        type = eDouble;
        break;

    case 16:   // bool
    case 21:   // int2
    case 23:   // int4
    case 20:   // int8
        type = eInteger;
        break;

    case 26:   // oid
        type = eUnsignedLong;
        break;

    default:
         throw SOCIError("Unknown data type.");
    }

    columnName = PQfname(result_, pos);
}

PostgreSQLStandardIntoTypeBackEnd *
PostgreSQLStatementBackEnd::makeIntoTypeBackEnd()
{
    hasIntoElements_ = true;
    return new PostgreSQLStandardIntoTypeBackEnd(*this);
}

PostgreSQLStandardUseTypeBackEnd *
PostgreSQLStatementBackEnd::makeUseTypeBackEnd()
{
    hasUseElements_ = true;
    return new PostgreSQLStandardUseTypeBackEnd(*this);
}

PostgreSQLVectorIntoTypeBackEnd *
PostgreSQLStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    hasVectorIntoElements_ = true;
    return new PostgreSQLVectorIntoTypeBackEnd(*this);
}

PostgreSQLVectorUseTypeBackEnd *
PostgreSQLStatementBackEnd::makeVectorUseTypeBackEnd()
{
    hasVectorUseElements_ = true;
    return new PostgreSQLVectorUseTypeBackEnd(*this);
}
