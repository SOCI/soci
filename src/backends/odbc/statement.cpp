//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_ODBC_SOURCE
#include "soci-odbc.h"
#include <cctype>
#include <sstream>
#include <cstring>

#ifdef _MSC_VER
// disables the warning about converting int to void*.  This is a 64 bit compatibility
// warning, but odbc requires the value to be converted on this line
// SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)number, 0);
#pragma warning(disable:4312)
#endif

using namespace soci;
using namespace soci::details;


odbc_statement_backend::odbc_statement_backend(odbc_session_backend &session)
    : session_(session), hstmt_(0), numRowsFetched_(0),
      hasVectorUseElements_(false), boundByName_(false), boundByPos_(false),
      lastNoData_(false)
{
}

void odbc_statement_backend::alloc()
{
    SQLRETURN rc;

    // Allocate environment handle
    rc = SQLAllocHandle(SQL_HANDLE_STMT, session_.hdbc_, &hstmt_);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_DBC, session_.hdbc_,
            "Allocating statement");
    }
}

void odbc_statement_backend::clean_up()
{
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
}


void odbc_statement_backend::prepare(std::string const & query,
    statement_type /* eType */)
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
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         query_.c_str());
    }
}

statement_backend::exec_fetch_result
odbc_statement_backend::execute(int number)
{
    // made this static because MSVC debugger was reporting
    // that there was an attempt to use rows_processed after the stack
    // was destroyed.  Some ODBC clean_up ?
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
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         "Statement Execute");
    }

    lastNoData_ = rc == SQL_NO_DATA;

    SQLSMALLINT colCount;
    SQLNumResultCols(hstmt_, &colCount);

    if (number > 0 && colCount > 0)
    {
        return fetch(number);
    }

    return ef_success;
}

statement_backend::exec_fetch_result
odbc_statement_backend::fetch(int number)
{
    numRowsFetched_ = 0;
    SQLULEN const row_array_size = static_cast<SQLULEN>(number);

    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)row_array_size, 0);
    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched_, 0);

    SQLRETURN rc = SQLFetch(hstmt_);

    if (SQL_NO_DATA == rc)
    {
        return ef_no_data;
    }

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         "Statement Fetch");
    }

    return ef_success;
}

long long odbc_statement_backend::get_affected_rows()
{
    // Calling SQLRowCount() when the last call to SQLExecute() returned
    // SQL_NO_DATA can fail, so simply always return 0 in this case as we know
    // that nothing was done anyhow.
    if (lastNoData_)
        return 0;

    SQLLEN res;
    SQLRETURN rc = SQLRowCount(hstmt_, &res);
    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                          "Getting number of affected rows");
    }

    return res;
}

int odbc_statement_backend::get_number_of_rows()
{
    return numRowsFetched_;
}

std::string odbc_statement_backend::rewrite_for_procedure_call(
    std::string const &query)
{
    return query;
}

int odbc_statement_backend::prepare_for_describe()
{
    SQLSMALLINT numCols;
    SQLNumResultCols(hstmt_, &numCols);
    return numCols;
}

void odbc_statement_backend::describe_column(int colNum, data_type & type,
                                          std::string & columnName)
{
    SQLCHAR colNameBuffer[2048];
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLULEN colSize;
    SQLSMALLINT decDigits;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, static_cast<SQLUSMALLINT>(colNum),
                                  colNameBuffer, 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         "describe Column");
    }

    char const *name = reinterpret_cast<char const *>(colNameBuffer);
    columnName.assign(name, std::strlen(name));

    switch (dataType)
    {
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
        type = dt_date;
        break;
    case SQL_DOUBLE:
    case SQL_DECIMAL:
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_NUMERIC:
        type = dt_double;
        break;
    case SQL_TINYINT:
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIGINT:
        type = dt_integer;
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    default:
        type = dt_string;
        break;
    }
}

std::size_t odbc_statement_backend::column_size(int colNum)
{
    SQLCHAR colNameBuffer[2048];
    SQLSMALLINT colNameBufferOverflow;
    SQLSMALLINT dataType;
    SQLULEN colSize;
    SQLSMALLINT decDigits;
    SQLSMALLINT isNullable;

    SQLRETURN rc = SQLDescribeCol(hstmt_, static_cast<SQLUSMALLINT>(colNum),
                                  colNameBuffer, 2048,
                                  &colNameBufferOverflow, &dataType,
                                  &colSize, &decDigits, &isNullable);

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                         "column size");
    }

    return colSize;
}

odbc_standard_into_type_backend * odbc_statement_backend::make_into_type_backend()
{
    return new odbc_standard_into_type_backend(*this);
}

odbc_standard_use_type_backend * odbc_statement_backend::make_use_type_backend()
{
    return new odbc_standard_use_type_backend(*this);
}

odbc_vector_into_type_backend *
odbc_statement_backend::make_vector_into_type_backend()
{
    return new odbc_vector_into_type_backend(*this);
}

odbc_vector_use_type_backend * odbc_statement_backend::make_vector_use_type_backend()
{
    hasVectorUseElements_ = true;
    return new odbc_vector_use_type_backend(*this);
}
