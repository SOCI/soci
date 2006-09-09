//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

Sqlite3StatementBackEnd::Sqlite3StatementBackEnd(
    Sqlite3SessionBackEnd &session)
    : session_(session), stmt_(0), dataCache_(), useData_(0), 
      databaseReady_(false), boundByName_(false), boundByPos_(false)
{
}

void Sqlite3StatementBackEnd::alloc()
{
    // ...
}

void Sqlite3StatementBackEnd::cleanUp()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
        databaseReady_ = false;
    }
}

void Sqlite3StatementBackEnd::prepare(std::string const & query)
{
    cleanUp();

    const char *tail; // unused;
    int res = sqlite3_prepare(session_.conn_, 
                              query.c_str(), 
                              query.size(), 
                              &stmt_, 
                              &tail);    
    if (res != SQLITE_OK)
    {
        const char *zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "Sqlite3StatementBackEnd::prepare: "
           << zErrMsg;
        throw SOCIError(ss.str());
    }
    databaseReady_ = true;
}

// sqlite3_reset needs to be called before a prepared statment can
// be executed a second time.  
void Sqlite3StatementBackEnd::resetIfNeeded()
{
    if (stmt_ && !databaseReady_)
    {
        int res = sqlite3_reset(stmt_);
        if (SQLITE_OK == res)
            databaseReady_ = true;
    }   
}

// This is used by bulk operations 
StatementBackEnd::execFetchResult 
Sqlite3StatementBackEnd::loadRS(int totalRows)
{
    StatementBackEnd::execFetchResult retVal = eSuccess;
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

                for (Sqlite3RecordSet::iterator it = dataCache_.begin();
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
            cleanUp();

            const char *zErrMsg = sqlite3_errmsg(session_.conn_);

            std::ostringstream ss;
            ss << "Sqlite3StatementBackEnd::loadRS: "
               << zErrMsg;
            throw SOCIError(ss.str());
        }
    }

    // if we read less than requested then shrink the vector
    dataCache_.resize(i);

    return retVal;
}

// This is used for non-bulk operations
StatementBackEnd::execFetchResult 
Sqlite3StatementBackEnd::loadOne()
{
    StatementBackEnd::execFetchResult retVal = eSuccess;
    
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
        cleanUp();

        const char *zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "Sqlite3StatementBackEnd::loadOne: "
           << zErrMsg;
        throw SOCIError(ss.str());
    }

    return retVal;
}

// Execute statements once for every row of useData
StatementBackEnd::execFetchResult 
Sqlite3StatementBackEnd::bindAndExecute(int number)
{
    StatementBackEnd::execFetchResult retVal = eNoData;

    int rows = useData_.size();

    for (int row = 0; row < rows; ++row)
    {
        sqlite3_reset(stmt_);

        int totalPositions = useData_[0].size();
        for (int pos = 1; pos <= totalPositions; ++pos)
        {
            int bindRes = SQLITE_OK;
            const Sqlite3Column& curCol = useData_[row][pos-1];
            if (curCol.isNull_)
                bindRes = sqlite3_bind_null(stmt_, pos);
            else
                bindRes = sqlite3_bind_text(stmt_, pos, 
                                            curCol.data_.c_str(), 
                                            curCol.data_.length(), 
                                            SQLITE_STATIC);

            if (SQLITE_OK != bindRes)
                throw SOCIError("Failure to bind on bulk operations");
        }
        
        // Handle the case where there are both into and use elements 
        // in the same query and one of the into binds to a vector object.
        if (1 == rows && number != rows)
            return loadRS(number);
        
        retVal = loadOne(); //execute each bound line
    }
    return retVal;
}

StatementBackEnd::execFetchResult
Sqlite3StatementBackEnd::execute(int number)
{
    if (!stmt_)
        throw SOCIError("No sqlite statement created");

    sqlite3_reset(stmt_);
    databaseReady_ = true;

    StatementBackEnd::execFetchResult retVal = eNoData;

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

StatementBackEnd::execFetchResult
Sqlite3StatementBackEnd::fetch(int number)
{
    return loadRS(number);
}

int Sqlite3StatementBackEnd::getNumberOfRows()
{
    return dataCache_.size();
}

std::string Sqlite3StatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    return query;
}

int Sqlite3StatementBackEnd::prepareForDescribe()
{
    return sqlite3_column_count(stmt_);
}

void Sqlite3StatementBackEnd::describeColumn(int colNum, eDataType & type, 
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

Sqlite3StandardIntoTypeBackEnd *
Sqlite3StatementBackEnd::makeIntoTypeBackEnd()
{
    return new Sqlite3StandardIntoTypeBackEnd(*this);
}

Sqlite3StandardUseTypeBackEnd * Sqlite3StatementBackEnd::makeUseTypeBackEnd()
{
    return new Sqlite3StandardUseTypeBackEnd(*this);
}

Sqlite3VectorIntoTypeBackEnd *
Sqlite3StatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new Sqlite3VectorIntoTypeBackEnd(*this);
}

Sqlite3VectorUseTypeBackEnd *
Sqlite3StatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new Sqlite3VectorUseTypeBackEnd(*this);
}
