//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-odbc.h"
#include <cctype>

#ifdef _MSC_VER
// disables the warning about converting int to void*.  This is a 64 bit compatibility
// warning, but odbc requires the value to be converted on this line
// SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)number, 0);
#pragma warning(disable:4312)
#endif

using namespace SOCI;
using namespace SOCI::details;


ODBCStatementBackEnd::ODBCStatementBackEnd(ODBCSessionBackEnd &session)
    : session_(session), hstmt_(0), numRowsFetched_(0)
    , hasVectorUseElements_(false), boundByName_(false), boundByPos_(false)
{
}

void ODBCStatementBackEnd::alloc()
{
    SQLRETURN rc;
    
	// Allocate environment handle
	rc = SQLAllocHandle(SQL_HANDLE_STMT, session_.hdbc_, &hstmt_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_DBC, session_.hdbc_, 
            "Allocating statement");
    }
}

void ODBCStatementBackEnd::cleanUp()
{
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
}


void ODBCStatementBackEnd::prepare(std::string const & query,
    eStatementType /* eType */)
{
    // rewrite the query by transforming all named parameters into
    // the ODBC numbers ones (:abc -> $1, etc.)

    enum { eNormal, eInQuotes, eInName, eInAccessDate } state = eNormal;

    std::string name;

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
            else if (*it == '#')
            {
                query_ += *it;
                state = eInAccessDate;
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
                ss << '?';
                query_ += ss.str();
                query_ += *it;
                state = eNormal;
            }
            break;
        case eInAccessDate:
            if (*it == '#')
            {
                query_ += *it;
                state = eNormal;
            }
            else // regular quoted character
            {
                query_ += *it;
            }
            break;            
        }
    }

    if (state == eInName)
    {
        names_.push_back(name);
        std::ostringstream ss;
        ss << '?';
        query_ += ss.str();
    }

    SQLRETURN rc = SQLPrepare(hstmt_, (SQLCHAR*)query_.c_str(), (SQLINTEGER)query_.size());
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, hstmt_, 
                         query_.c_str());
    }
}

StatementBackEnd::execFetchResult
ODBCStatementBackEnd::execute(int number)
{
	// made this static because MSVC debugger was reporting
	// that there was an attempt to use rows_processed after the stack
	// was destroyed.  Some ODBC cleanup ?
    static SQLUSMALLINT rows_processed = 0;
    
    if (hasVectorUseElements_)
    {
        SQLSetStmtAttr(hstmt_, SQL_ATTR_PARAMS_PROCESSED_PTR, &rows_processed, 0);
    }

    // if we are called twice for the same statement we need to close the open
    // cursor or an "invalid cursor state" error will occur on execute
    SQLCloseCursor(hstmt_);

    SQLRETURN rc = SQLExecute(hstmt_);
    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, hstmt_, 
                         "Statement Execute");
    }

    SQLSMALLINT colCount;
    SQLNumResultCols(hstmt_, &colCount);    

    if (number > 0 && colCount > 0)
        return fetch(number);

    return eSuccess;
}

StatementBackEnd::execFetchResult
ODBCStatementBackEnd::fetch(int number)
{
    numRowsFetched_ = 0;
    
	SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
	SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)number, 0);
	SQLSetStmtAttr(hstmt_, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched_, 0);

    SQLRETURN rc = SQLFetch(hstmt_);

    if (SQL_NO_DATA == rc)
        return eNoData;

    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, hstmt_, 
                         "Statement Fetch");
    }
    
    return eSuccess;
}

int ODBCStatementBackEnd::getNumberOfRows()
{
    return numRowsFetched_;
}

std::string ODBCStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    return query;
}

int ODBCStatementBackEnd::prepareForDescribe()
{ 
    SQLSMALLINT numCols;
    SQLNumResultCols(hstmt_, &numCols);
    return numCols;
}

void ODBCStatementBackEnd::describeColumn(int colNum, eDataType & type, 
                                          std::string & columnName)
{
    SQLCHAR colNameBuffer[2048];
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLUINTEGER colSize;
    SQLSMALLINT decDigits;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, colNum, colNameBuffer, 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, hstmt_, 
                         "describe Column");
    }
    
    char const *name = reinterpret_cast<char const *>(colNameBuffer);
    columnName.assign(name, std::strlen(name));
    
    switch (dataType)
    {
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
        type = eDate;
        break;
    case SQL_DOUBLE:
    case SQL_DECIMAL:
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_NUMERIC:
        type = eDouble;
        break;
    case SQL_TINYINT:
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIGINT:
        type = eInteger;
        break;       
    case SQL_CHAR:
    case SQL_VARCHAR:
    default:
        type = eString;
        break;
    }
}

std::size_t ODBCStatementBackEnd::columnSize(int colNum)
{
    SQLCHAR colNameBuffer[2048];
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLUINTEGER colSize;
    SQLSMALLINT decDigits;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, colNum, colNameBuffer, 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        throw ODBCSOCIError(SQL_HANDLE_STMT, hstmt_, 
                         "column size");
    }

    return colSize;
}



ODBCStandardIntoTypeBackEnd * ODBCStatementBackEnd::makeIntoTypeBackEnd()
{
    return new ODBCStandardIntoTypeBackEnd(*this);
}

ODBCStandardUseTypeBackEnd * ODBCStatementBackEnd::makeUseTypeBackEnd()
{
    return new ODBCStandardUseTypeBackEnd(*this);
}

ODBCVectorIntoTypeBackEnd *
ODBCStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new ODBCVectorIntoTypeBackEnd(*this);
}

ODBCVectorUseTypeBackEnd * ODBCStatementBackEnd::makeVectorUseTypeBackEnd()
{
    hasVectorUseElements_ = true;
    return new ODBCVectorUseTypeBackEnd(*this);
}
