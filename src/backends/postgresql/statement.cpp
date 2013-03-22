//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_POSTGRESQL_SOURCE
#include "soci-postgresql.h"
#include "error.h"
#include <soci-platform.h>
#include <libpq/libpq-fs.h> // libpq
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef SOCI_POSTGRESQL_NOPARAMS
#define SOCI_POSTGRESQL_NOBINDBYNAME
#endif // SOCI_POSTGRESQL_NOPARAMS

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace soci::details::postgresql;

postgresql_statement_backend::postgresql_statement_backend(
    postgresql_session_backend &session)
     : session_(session), result_(NULL), justDescribed_(false),
       hasIntoElements_(false), hasVectorIntoElements_(false),
       hasUseElements_(false), hasVectorUseElements_(false)
{
}

postgresql_statement_backend::~postgresql_statement_backend()
{
    if (statementName_.empty() == false)
        session_.deallocate_prepared_statement(statementName_);
}

void postgresql_statement_backend::alloc()
{
    // nothing to do here
}

void postgresql_statement_backend::clean_up()
{
    if (result_ != NULL)
    {
        PQclear(result_);
        result_ = NULL;
    }
}

void postgresql_statement_backend::prepare(std::string const & query,
    statement_type stType)
{
#ifdef SOCI_POSTGRESQL_NOBINDBYNAME
    query_ = query;
#else
    // rewrite the query by transforming all named parameters into
    // the postgresql_ numbers ones (:abc -> $1, etc.)

    enum { normal, in_quotes, in_name } state = normal;

    std::string name;
    int position = 1;

    for (std::string::const_iterator it = query.begin(), end = query.end();
         it != end; ++it)
    {
        switch (state)
        {
        case normal:
            if (*it == '\'')
            {
                query_ += *it;
                state = in_quotes;
            }
            else if (*it == ':')
            {
                // Check whether this is a cast operator (e.g. 23::float)
                // and treat it as a special case, not as a named binding
                const std::string::const_iterator next_it = it + 1;
                if ((next_it != end) && (*next_it == ':'))
                {
                    query_ += "::";
                    ++it;
                }
                // Check whether this is an assignment(e.g. x:=y)
                // and treat it as a special case, not as a named binding
                if ((next_it != end) && (*next_it == '='))
                {
                    query_ += ":=";
                    ++it;
                }
                else
                {
                    state = in_name;
                }
            }
            else // regular character, stay in the same state
            {
                query_ += *it;
            }
            break;
        case in_quotes:
            if (*it == '\'')
            {
                query_ += *it;
                state = normal;
            }
            else // regular quoted character
            {
                query_ += *it;
            }
            break;
        case in_name:
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
                state = normal;

                // Check whether the named parameter is immediatelly
                // followed by a cast operator (e.g. :name::float)
                // and handle the additional colon immediately to avoid
                // its misinterpretation later on.
                if (*it == ':')
                {
                    const std::string::const_iterator next_it = it + 1;
                    if ((next_it != end) && (*next_it == ':'))
                    {
                        query_ += ':';
                        ++it;
                    }
                }
            }
            break;
        }
    }

    if (state == in_name)
    {
        names_.push_back(name);
        std::ostringstream ss;
        ss << '$' << position++;
        query_ += ss.str();
    }

#endif // SOCI_POSTGRESQL_NOBINDBYNAME

#ifndef SOCI_POSTGRESQL_NOPREPARE

    if (stType == st_repeatable_query)
    {
        // Holding the name temporarily in this var because
        // if it fails to prepare it we can't DEALLOCATE it. 
        std::string statementName = session_.get_next_statement_name();

        PGresult* result = PQprepare(session_.conn_, statementName.c_str(),
            query_.c_str(), static_cast<int>(names_.size()), NULL);
        if (result == NULL)
        {
            throw soci_error("Cannot prepare statement.");
        }
        // Now it's safe to save this info.
        statementName_ = statementName;
        
        ExecStatusType status = PQresultStatus(result);
        if (status != PGRES_COMMAND_OK)
        {
            // releases result with PQclear
            throw_postgresql_soci_error(result);
        }
        PQclear(result);
    }

    stType_ = stType;

#endif // SOCI_POSTGRESQL_NOPREPARE
}

statement_backend::exec_fetch_result
postgresql_statement_backend::execute(int number)
{
    // If the statement was "just described", then we know that
    // it was actually executed with all the use elements
    // already bound and pre-used. This means that the result of the
    // query is already on the client side, so there is no need
    // to re-execute it.

    if (justDescribed_ == false)
    {
        // This object could have been already filled with data before.
        clean_up();

        if (number > 1 && hasIntoElements_)
        {
             throw soci_error(
                  "Bulk use with single into elements is not supported.");
        }

        // Since the bulk operations are not natively supported by postgresql_,
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

        if ((useByPosBuffers_.empty() == false) ||
            (useByNameBuffers_.empty() == false))
        {
            if ((useByPosBuffers_.empty() == false) &&
                (useByNameBuffers_.empty() == false))
            {
                throw soci_error(
                    "Binding for use elements must be either by position "
                    "or by name.");
            }

            for (int i = 0; i != numberOfExecutions; ++i)
            {
                std::vector<char *> paramValues;

                if (useByPosBuffers_.empty() == false)
                {
                    // use elements bind by position
                    // the map of use buffers can be traversed
                    // in its natural order

                    for (UseByPosBuffersMap::iterator
                             it = useByPosBuffers_.begin(),
                             end = useByPosBuffers_.end();
                         it != end; ++it)
                    {
                        char ** buffers = it->second;
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
                            throw soci_error(msg);
                        }
                        char ** buffers = b->second;
                        paramValues.push_back(buffers[i]);
                    }
                }

#ifdef SOCI_POSTGRESQL_NOPARAMS

                throw soci_error("Queries with parameters are not supported.");

#else

#ifdef SOCI_POSTGRESQL_NOPREPARE

                result_ = PQexecParams(session_.conn_, query_.c_str(),
                    static_cast<int>(paramValues.size()),
                    NULL, &paramValues[0], NULL, NULL, 0);
#else
                if (stType_ == st_repeatable_query)
                {
                    // this query was separately prepared

                    result_ = PQexecPrepared(session_.conn_,
                        statementName_.c_str(),
                        static_cast<int>(paramValues.size()),
                        &paramValues[0], NULL, NULL, 0);
                }
                else // stType_ == st_one_time_query
                {
                    // this query was not separately prepared and should
                    // be executed as a one-time query

                    result_ = PQexecParams(session_.conn_, query_.c_str(),
                        static_cast<int>(paramValues.size()),
                        NULL, &paramValues[0], NULL, NULL, 0);
                }

#endif // SOCI_POSTGRESQL_NOPREPARE

#endif // SOCI_POSTGRESQL_NOPARAMS

                if (result_ == NULL)
                {
                    throw soci_error("Cannot execute query.");
                }

                if (numberOfExecutions > 1)
                {
                    // there are only bulk use elements (no intos)

                    ExecStatusType status = PQresultStatus(result_);
                    if (status != PGRES_COMMAND_OK)
                    {
                        // releases result_ with PQclear
                        throw_postgresql_soci_error(result_);
                    }
                    PQclear(result_);
                }
            }

            if (numberOfExecutions > 1)
            {
                // it was a bulk operation
                result_ = NULL;
                return ef_no_data;
            }

            // otherwise (no bulk), follow the code below
        }
        else
        {
            // there are no use elements
            // - execute the query without parameter information

#ifdef SOCI_POSTGRESQL_NOPREPARE

            result_ = PQexec(session_.conn_, query_.c_str());
#else
            if (stType_ == st_repeatable_query)
            {
                // this query was separately prepared

                result_ = PQexecPrepared(session_.conn_,
                    statementName_.c_str(), 0, NULL, NULL, NULL, 0);
            }
            else // stType_ == st_one_time_query
            {
                result_ = PQexec(session_.conn_, query_.c_str());
            }

#endif // SOCI_POSTGRESQL_NOPREPARE

            if (result_ == NULL)
            {
                throw soci_error("Cannot execute query.");
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
            return ef_no_data;
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
                return ef_success;
            }
        }
    }
    else if (status == PGRES_COMMAND_OK)
    {
        return ef_no_data;
    }
    else
    {
        // releases result_ with PQclear
        throw_postgresql_soci_error(result_);

        // dummy, never reach
        return ef_no_data;
    }
}

statement_backend::exec_fetch_result
postgresql_statement_backend::fetch(int number)
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
        return ef_no_data;
    }
    else
    {
        if (currentRow_ + number > numberOfRows_)
        {
            rowsToConsume_ = numberOfRows_ - currentRow_;

            // this simulates the behaviour of Oracle
            // - when EOF is hit, we return ef_no_data even when there are
            // actually some rows fetched
            return ef_no_data;
        }
        else
        {
            rowsToConsume_ = number;
            return ef_success;
        }
    }
}

long long postgresql_statement_backend::get_affected_rows()
{
    const char * resultStr = PQcmdTuples(result_);
    char * end;
    long long result = std::strtoll(resultStr, &end, 0);
    if (end != resultStr)
    {
        return result;
    }
    else
    {
        return -1;
    }
}

int postgresql_statement_backend::get_number_of_rows()
{
    return numberOfRows_ - currentRow_;
}

std::string postgresql_statement_backend::rewrite_for_procedure_call(
    std::string const & query)
{
    std::string newQuery("select ");
    newQuery += query;
    return newQuery;
}

int postgresql_statement_backend::prepare_for_describe()
{
    execute(1);
    justDescribed_ = true;

    int columns = PQnfields(result_);
    return columns;
}

void postgresql_statement_backend::describe_column(int colNum, data_type & type,
    std::string & columnName)
{
    // In postgresql_ column numbers start from 0
    int const pos = colNum - 1;

    unsigned long const typeOid = PQftype(result_, pos);
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
    case 142: // xml
    case 114:  // json
    case 17: // bytea
        type = dt_string;
        break;

    case 702:  // abstime
    case 703:  // reltime
    case 1082: // date
    case 1083: // time
    case 1114: // timestamp
    case 1184: // timestamptz
    case 1266: // timetz
        type = dt_date;
        break;

    case 700:  // float4
    case 701:  // float8
    case 1700: // numeric
        type = dt_double;
        break;

    case 16:   // bool
    case 21:   // int2
    case 23:   // int4
    case 26:   // oid
        type = dt_integer;
        break;

    case 20:   // int8
        type = dt_long_long;
        break;
    
    default:
    {
        std::stringstream message;
        message << "unknown data type with typelem: " << typeOid << " for colNum: " << colNum << " with name: " << PQfname(result_, pos);
        throw soci_error(message.str());
        
    }
    }

    columnName = PQfname(result_, pos);
}

postgresql_standard_into_type_backend *
postgresql_statement_backend::make_into_type_backend()
{
    hasIntoElements_ = true;
    return new postgresql_standard_into_type_backend(*this);
}

postgresql_standard_use_type_backend *
postgresql_statement_backend::make_use_type_backend()
{
    hasUseElements_ = true;
    return new postgresql_standard_use_type_backend(*this);
}

postgresql_vector_into_type_backend *
postgresql_statement_backend::make_vector_into_type_backend()
{
    hasVectorIntoElements_ = true;
    return new postgresql_vector_into_type_backend(*this);
}

postgresql_vector_use_type_backend *
postgresql_statement_backend::make_vector_use_type_backend()
{
    hasVectorUseElements_ = true;
    return new postgresql_vector_use_type_backend(*this);
}
