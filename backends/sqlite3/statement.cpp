//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci-sqlite3.h"

#include <sstream>
#include <algorithm>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace sqlite_api;

sqlite3_statement_backend::sqlite3_statement_backend(
    sqlite3_session_backend &session)
    : session_(session), stmt_(0), dataCache_(), useData_(0), 
      databaseReady_(false), boundByName_(false), boundByPos_(false)
{
}

void sqlite3_statement_backend::alloc()
{
    // ...
}

void sqlite3_statement_backend::clean_up()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
        databaseReady_ = false;
    }
}

void sqlite3_statement_backend::prepare(std::string const & query,
    eStatementType /* eType */)
{
    clean_up();

    const char *tail; // unused;
    int res = sqlite3_prepare(session_.conn_, 
                              query.c_str(), 
                              static_cast<int>(query.size()), 
                              &stmt_, 
                              &tail);    
    if (res != SQLITE_OK)
    {
        const char *zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "sqlite3_statement_backend::prepare: "
           << zErrMsg;
        throw soci_error(ss.str());
    }
    databaseReady_ = true;
}

// sqlite3_reset needs to be called before a prepared statment can
// be executed a second time.  
void sqlite3_statement_backend::resetIfNeeded()
{
    if (stmt_ && !databaseReady_)
    {
        int res = sqlite3_reset(stmt_);
        if (SQLITE_OK == res)
            databaseReady_ = true;
    }   
}

// This is used by bulk operations 
statement_backend::execFetchResult 
sqlite3_statement_backend::loadRS(int totalRows)
{
    statement_backend::execFetchResult retVal = eSuccess;
    int numCols = -1;

    // make the vector big enough to hold the data we need
    dataCache_.resize(totalRows);
    
    int i = 0;
    for (i = 0; i < totalRows; ++i)
    {
        int res = sqlite3_step(stmt_);

        if (SQLITE_DONE == res)
        {
            databaseReady_ = false;
            retVal = eNoData;

            break;
        }
        else if (SQLITE_ROW == res)
        {
            // only need to set the number of columns once
            if (-1 == numCols)
            {
                numCols = sqlite3_column_count(stmt_);

                for (sqlite3_recordset::iterator it = dataCache_.begin();
                    it != dataCache_.end(); ++it)
                {
                    (*it).resize(numCols);
                }
            }
            for (int c = 0; c < numCols; ++c)
            {
                const char *buf = 
                reinterpret_cast<const char*>(sqlite3_column_text(
                                                  stmt_, 
                                                  c));
                bool isNull = false;
                
                if (0 == buf)
                {
                    isNull = true;
                    buf = "";
                }
                
                dataCache_[i][c].data_ = buf;
                dataCache_[i][c].isNull_ = isNull;
            }
        }
        else
        {
            clean_up();

            const char *zErrMsg = sqlite3_errmsg(session_.conn_);

            std::ostringstream ss;
            ss << "sqlite3_statement_backend::loadRS: "
               << zErrMsg;
            throw soci_error(ss.str());
        }
    }

    // if we read less than requested then shrink the vector
    dataCache_.resize(i);

    return retVal;
}

// This is used for non-bulk operations
statement_backend::execFetchResult 
sqlite3_statement_backend::loadOne()
{
    statement_backend::execFetchResult retVal = eSuccess;
    
    int res = sqlite3_step(stmt_);

    if (SQLITE_DONE == res)
    {
        databaseReady_ = false;
        retVal = eNoData;
    }
    else if (SQLITE_ROW == res)
    {
    }
    else
    {
        clean_up();

        const char *zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "sqlite3_statement_backend::loadOne: "
           << zErrMsg;
        throw soci_error(ss.str());
    }

    return retVal;
}

// Execute statements once for every row of useData
statement_backend::execFetchResult 
sqlite3_statement_backend::bindAndExecute(int number)
{
    statement_backend::execFetchResult retVal = eNoData;

    int rows = static_cast<int>(useData_.size());

    for (int row = 0; row < rows; ++row)
    {
        sqlite3_reset(stmt_);

        int totalPositions = static_cast<int>(useData_[0].size());
        for (int pos = 1; pos <= totalPositions; ++pos)
        {
            int bindRes = SQLITE_OK;
            const sqlite3_column& curCol = useData_[row][pos-1];
            if (curCol.isNull_)
                bindRes = sqlite3_bind_null(stmt_, pos);
            else
                bindRes = sqlite3_bind_text(stmt_, pos, 
                                            curCol.data_.c_str(), 
                                            static_cast<int>(curCol.data_.length()), 
                                            SQLITE_STATIC);

            if (SQLITE_OK != bindRes)
                throw soci_error("Failure to bind on bulk operations");
        }
        
        // Handle the case where there are both into and use elements 
        // in the same query and one of the into binds to a vector object.
        if (1 == rows && number != rows)
            return loadRS(number);
        
        retVal = loadOne(); //execute each bound line
    }
    return retVal;
}

statement_backend::execFetchResult
sqlite3_statement_backend::execute(int number)
{
    if (!stmt_)
        throw soci_error("No sqlite statement created");

    sqlite3_reset(stmt_);
    databaseReady_ = true;

    statement_backend::execFetchResult retVal = eNoData;

    if (!useData_.empty())
    {
           retVal = bindAndExecute(number);
    }
    else
    {
        if (1 == number)
            retVal = loadOne();
        else
            retVal = loadRS(number);
    }

    return retVal;
}

statement_backend::execFetchResult
sqlite3_statement_backend::fetch(int number)
{
    return loadRS(number);
}

int sqlite3_statement_backend::get_number_of_rows()
{
    return static_cast<int>(dataCache_.size());
}

std::string sqlite3_statement_backend::rewrite_for_procedure_call(
    std::string const &query)
{
    return query;
}

int sqlite3_statement_backend::prepare_for_describe()
{
    return sqlite3_column_count(stmt_);
}

void sqlite3_statement_backend::describe_column(int colNum, eDataType & type, 
                                             std::string & columnName)
{

    columnName = sqlite3_column_name(stmt_, colNum-1);

    // This is a hack, but the sqlite3 type system does not 
    // have a date or time field.  Also it does not reliably
    // id other data types.  It has a tendency to see everything
    // as text.  sqlite3_column_decltype returns the text that is
    // used in the create table statement
    bool typeFound = false;
  
    const char* declType = sqlite3_column_decltype(stmt_, colNum-1);
    std::string dt = declType;

    // do all comparisions in lower case
    std::transform(dt.begin(), dt.end(), dt.begin(), tolower);

    if (dt.find("time",0) != std::string::npos)
    {
        type = eDate;
        typeFound = true;
    }
    if (dt.find("date",0) != std::string::npos)
    {
        type = eDate;
        typeFound = true;
    }
    if (dt.find("int",0) != std::string::npos)
    {
        type = eInteger;
        typeFound = true;
    }
    if (dt.find("float",0) != std::string::npos)
    {
        type = eDouble;
        typeFound = true;
    }
    if (dt.find("char",0) != std::string::npos)
    {
        type = eString;
        typeFound = true;
    }

    if (typeFound)
        return;

    // try to get it from the weak ass type system

    // total hack - execute the statment once to get the column types
    // then clear so it can be executed again    
    sqlite3_step(stmt_);

    int sqlite3_type = sqlite3_column_type(stmt_, colNum-1);
    switch (sqlite3_type)
    {
    case SQLITE_INTEGER: type = eInteger; break;
    case SQLITE_FLOAT: type = eDouble; break;
    case SQLITE_BLOB:
    case SQLITE_TEXT: type = eString; break;
    default: type = eString; break;
    }

    sqlite3_reset(stmt_);
}

sqlite3_standard_into_type_backend *
sqlite3_statement_backend::make_into_type_backend()
{
    return new sqlite3_standard_into_type_backend(*this);
}

sqlite3_standard_use_type_backend * sqlite3_statement_backend::make_use_type_backend()
{
    return new sqlite3_standard_use_type_backend(*this);
}

sqlite3_vector_into_type_backend *
sqlite3_statement_backend::make_vector_into_type_backend()
{
    return new sqlite3_vector_into_type_backend(*this);
}

sqlite3_vector_use_type_backend *
sqlite3_statement_backend::make_vector_use_type_backend()
{
    return new sqlite3_vector_use_type_backend(*this);
}
