//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-mysql.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using std::string;


MySQLStatementBackEnd::MySQLStatementBackEnd(MySQLSessionBackEnd &session)
    : session_(session), result_(NULL), justDescribed_(false),
       hasIntoElements_(false), hasVectorIntoElements_(false),
       hasUseElements_(false), hasVectorUseElements_(false)
{
}

void MySQLStatementBackEnd::alloc()
{
    // nothing to do here.
}

void MySQLStatementBackEnd::cleanUp()
{
    if (result_ != NULL)
    {
        mysql_free_result(result_);
        result_ = NULL;
    }
}

void MySQLStatementBackEnd::prepare(std::string const & query)
{
    queryChunks_.clear();
    enum { eNormal, eInQuotes, eInName } state = eNormal;

    std::string name;
    queryChunks_.push_back("");

    for (std::string::const_iterator it = query.begin(), end = query.end();
         it != end; ++it)
    {
        switch (state)
        {
        case eNormal:
            if (*it == '\'')
            {
                queryChunks_.back() += *it;
                state = eInQuotes;
            }
            else if (*it == ':')
            {
                state = eInName;
            }
            else // regular character, stay in the same state
            {
                queryChunks_.back() += *it;
            }
            break;
        case eInQuotes:
            if (*it == '\'')
            {
                queryChunks_.back() += *it;
                state = eNormal;
            }
            else // regular quoted character
            {
                queryChunks_.back() += *it;
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
                queryChunks_.push_back("");
                queryChunks_.back() += *it;
                state = eNormal;
            }
            break;
        }
    }

    if (state == eInName)
    {
        names_.push_back(name);
    }
/*
  cerr << "Chunks: ";
  for (std::vector<std::string>::iterator i = queryChunks_.begin();
  i != queryChunks_.end(); ++i)
  {
  cerr << "\"" << *i << "\" ";
  }
  cerr << "\nNames: ";
  for (std::vector<std::string>::iterator i = names_.begin();
  i != names_.end(); ++i)
  {
  cerr << "\"" << *i << "\" ";
  }
  cerr << endl;
*/
}

StatementBackEnd::execFetchResult
MySQLStatementBackEnd::execute(int number)
{
    if (justDescribed_ == false)
    {
        cleanUp();
        
        if (number > 1 && hasIntoElements_)
        {
             throw SOCIError(
                  "Bulk use with single into elements is not supported.");
        }
        // number - size of vectors (into/use)
        // numberOfExecutions - number of loops to perform
        int numberOfExecutions;
        if (number > 0)
        {
             numberOfExecutions = hasUseElements_ ? 1 : number;
        }
        
        std::string query;
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
                        //cerr<<"i: "<<i<<", buffers[i]: "<<buffers[i]<<endl;
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
                //cerr << "queryChunks_.size(): "<<queryChunks_.size()<<endl;
                //cerr << "paramValues.size(): "<<paramValues.size()<<endl;
                if (queryChunks_.size() != paramValues.size()
                    and queryChunks_.size() != paramValues.size() + 1)
                {
                    throw SOCIError("Wrong number of parameters.");
                }
		
                std::vector<std::string>::const_iterator ci
                    = queryChunks_.begin();
                for (std::vector<char*>::const_iterator
                         pi = paramValues.begin(), end = paramValues.end();
                     pi != end; ++ci, ++pi) {
                    query += *ci;
                    query += *pi;
                }
                if (ci != queryChunks_.end())
                {
                    query += *ci;
                }
                if (numberOfExecutions > 1)
                {
                    // bulk operation
                    //cerr << query << endl;
                    if (0 != mysql_real_query(session_.conn_, query.c_str(),
                            query.size()))
                    {
                        throw SOCIError(mysql_error(session_.conn_));
                    }
                    if (mysql_field_count(session_.conn_) != 0)
                    {
                        throw SOCIError("The query shouldn't have returned"
                            " any data but it did.");
                    }
                    query.clear();
                }
            }
            if (numberOfExecutions > 1)
            {
                // bulk
                return eNoData;
            }
        }
        else
        {
            query = queryChunks_.front();
        }

        //cerr << query << endl;
        if (0 != mysql_real_query(session_.conn_, query.c_str(),
                query.size()))
        {
            throw SOCIError(mysql_error(session_.conn_));
        }
        result_ = mysql_store_result(session_.conn_);
        if (result_ == NULL and mysql_field_count(session_.conn_) != 0)
        {
            throw SOCIError(mysql_error(session_.conn_));
        }
    }
    else
    {
        justDescribed_ = false;
    }

    if (result_ != NULL)
    {
        currentRow_ = 0;
        rowsToConsume_ = 0;
	
        numberOfRows_ = mysql_num_rows(result_);
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
    else
    {
        // it was not a SELECT
        return eNoData;
    }
}

StatementBackEnd::execFetchResult
MySQLStatementBackEnd::fetch(int number)
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

int MySQLStatementBackEnd::getNumberOfRows()
{
    return numberOfRows_ - currentRow_;
}

std::string MySQLStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    std::string newQuery("select ");
    newQuery += query;
    return newQuery;
}

int MySQLStatementBackEnd::prepareForDescribe()
{
    execute(1);
    justDescribed_ = true;

    int columns = mysql_field_count(session_.conn_);
    return columns;
}

void MySQLStatementBackEnd::describeColumn(int colNum,
    eDataType & type, std::string & columnName)
{
    int pos = colNum - 1;
    MYSQL_FIELD *field = mysql_fetch_field_direct(result_, pos);
    switch (field->type) {
    case FIELD_TYPE_CHAR:       //MYSQL_TYPE_TINY:
    case FIELD_TYPE_SHORT:      //MYSQL_TYPE_SHORT:
    case FIELD_TYPE_LONG:       //MYSQL_TYPE_LONG:
    case FIELD_TYPE_LONGLONG:   //MYSQL_TYPE_LONGLONG:
    case FIELD_TYPE_INT24:      //MYSQL_TYPE_INT24:
        type = eInteger;
        break;
    case FIELD_TYPE_FLOAT:      //MYSQL_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:     //MYSQL_TYPE_DOUBLE:
    case FIELD_TYPE_DECIMAL:    //MYSQL_TYPE_DECIMAL:
//  case MYSQL_TYPE_NEWDECIMAL:
        type = eDouble;
        break;
    case FIELD_TYPE_TIMESTAMP:  //MYSQL_TYPE_TIMESTAMP:
    case FIELD_TYPE_DATE:       //MYSQL_TYPE_DATE:
    case FIELD_TYPE_TIME:       //MYSQL_TYPE_TIME:
    case FIELD_TYPE_DATETIME:   //MYSQL_TYPE_DATETIME:
    case FIELD_TYPE_YEAR:       //MYSQL_TYPE_YEAR:
    case FIELD_TYPE_NEWDATE:    //MYSQL_TYPE_NEWDATE:
        type = eDate;
        break;
//  case MYSQL_TYPE_VARCHAR:
    case FIELD_TYPE_VAR_STRING: //MYSQL_TYPE_VAR_STRING:
    case FIELD_TYPE_STRING:     //MYSQL_TYPE_STRING:
        type = eString;
        break;
    default:
        throw SOCIError("Unknown data type.");
    }
    columnName = field->name;
}

MySQLStandardIntoTypeBackEnd * MySQLStatementBackEnd::makeIntoTypeBackEnd()
{
    hasIntoElements_ = true;
    return new MySQLStandardIntoTypeBackEnd(*this);
}

MySQLStandardUseTypeBackEnd * MySQLStatementBackEnd::makeUseTypeBackEnd()
{
    hasUseElements_ = true;
    return new MySQLStandardUseTypeBackEnd(*this);
}

MySQLVectorIntoTypeBackEnd *
MySQLStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    hasVectorIntoElements_ = true;
    return new MySQLVectorIntoTypeBackEnd(*this);
}

MySQLVectorUseTypeBackEnd * MySQLStatementBackEnd::makeVectorUseTypeBackEnd()
{
    hasVectorUseElements_ = true;
    return new MySQLVectorUseTypeBackEnd(*this);
}

