//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/odbc/soci-odbc.h"
#include "soci/soci-unicode.h"
#include "soci/type-holder.h"

#include <cctype>
#include <sstream>
#include <cstring>

using namespace soci;
using namespace soci::details;


odbc_statement_backend::odbc_statement_backend(odbc_session_backend &session)
    : session_(session), hstmt_(0), numRowsFetched_(0), fetchVectorByRows_(false),
      boundByName_(false), boundByPos_(false),
      rowsAffected_(-1LL)
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
                              "allocating statement");
    }
}

void odbc_statement_backend::clean_up()
{
    rowsAffected_ = -1LL;

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
}


void odbc_statement_backend::prepare(std::string const & query,
    statement_type /* eType */)
{
    // rewrite the query by transforming all named parameters into
    // the ODBC numbers ones (:abc -> $1, etc.)

    enum { eNormal, eInQuotes, eInName, eInAccessDate } state = eNormal;

    std::string name;
    query_.reserve(query.length());

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
                query_ += "?";
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
        query_ += "?";
    }

    SQLRETURN rc = SQLPrepare(hstmt_, sqlchar_cast(query_), (SQLINTEGER)query_.size());
    if (is_odbc_error(rc))
    {
        std::ostringstream ss;
        ss << "preparing query \"" << query_ << "\"";
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_, ss.str());
    }

    // reset any old into buffers, they will be added later if they're used
    // with this query
    intos_.clear();
}

statement_backend::exec_fetch_result
odbc_statement_backend::execute(int number)
{
    // Store the number of rows processed by this call and the operation result
    // for each of them.
    SQLULEN rows_processed = 0;
    std::vector<SQLUSMALLINT> status;
    if (hasVectorUseElements_)
    {
        SQLSetStmtAttr(hstmt_, SQL_ATTR_PARAMS_PROCESSED_PTR, &rows_processed, 0);

        status.resize(number);
        SQLSetStmtAttr(hstmt_, SQL_ATTR_PARAM_STATUS_PTR, &status[0], 0);
    }

    // if we are called twice for the same statement we need to close the open
    // cursor or an "invalid cursor state" error will occur on execute
    SQLCloseCursor(hstmt_);

    SQLRETURN rc = SQLExecute(hstmt_);

    // Don't use is_odbc_error() here, as SQL_SUCCESS_WITH_INFO indicates an
    // error if it corresponds to a partial update.
    if (rc != SQL_SUCCESS && rc != SQL_NO_DATA)
    {
        // Construct the error object immediately, before calling any other
        // ODBC functions, in order to not lose the error message.
        const odbc_soci_error err(SQL_HANDLE_STMT, hstmt_, "executing statement");

        bool error = true;
        if (rc == SQL_SUCCESS_WITH_INFO)
        {
            // Check for partial update when using array parameters.
            if (hasVectorUseElements_)
            {
                rowsAffected_ = 0;

                error_row_ = -1;
                for (SQLULEN i = 0; i < rows_processed; ++i)
                {
                    switch (status[i])
                    {
                        case SQL_PARAM_SUCCESS:
                        case SQL_PARAM_SUCCESS_WITH_INFO:
                            ++rowsAffected_;
                            break;

                        case SQL_PARAM_ERROR:
                            if (error_row_ == -1)
                                error_row_ = soci_cast<int, SQLULEN>::cast(i);
                            break;

                        case SQL_PARAM_UNUSED:
                        case SQL_PARAM_DIAG_UNAVAILABLE:
                            // We shouldn't get those, normally, but just
                            // ignore them if we do.
                            break;
                    }
                }

                // In principle, it is possible to get success with info for an
                // operation which succeeded for all rows -- even though this
                // hasn't been observed so far. In this case, we shouldn't
                // throw an error.
                if (error_row_ == -1)
                    error = false;
            }
            else
            {
                // This is a weird case which has never been observed so far
                // and it's not clear what it might correspond to, but don't
                // handle it as an error to avoid throwing spurious exceptions
                // when there is no real problem.
                error = false;
            }
        }
        else
        {
            // If the statement failed completely, no rows should have been
            // affected.
            rowsAffected_ = 0;
        }

        if (error)
            throw err;
    }

    if (hasVectorUseElements_)
    {
        // We already have the number of rows, no need to do anything.
        rowsAffected_ = rows_processed;
    }
    else // We need to retrieve the number of rows affected explicitly.
    {
        SQLLEN res = 0;
        rc = SQLRowCount(hstmt_, &res);
        if (is_odbc_error(rc))
        {
            throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_,
                                  "getting number of affected rows");
        }

        rowsAffected_ = res;
    }
    SQLSMALLINT colCount;
    SQLNumResultCols(hstmt_, &colCount);

    if (number > 0 && colCount > 0)
    {
        return fetch(number);
    }

    return ef_success;
}

statement_backend::exec_fetch_result
odbc_statement_backend::do_fetch(int beginRow, int endRow)
{
    SQLRETURN rc = SQLFetch(hstmt_);

    if (SQL_NO_DATA == rc)
    {
        return ef_no_data;
    }

    if (is_odbc_error(rc))
    {
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_, "fetching data");
    }

    for (std::size_t j = 0; j != intos_.size(); ++j)
    {
        intos_[j]->do_post_fetch_rows(beginRow, endRow);
    }

    return ef_success;
}

statement_backend::exec_fetch_result
odbc_statement_backend::fetch(int number)
{
    numRowsFetched_ = 0;

    for (std::size_t i = 0; i != intos_.size(); ++i)
    {
        intos_[i]->resize(number);
    }

    SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);

    statement_backend::exec_fetch_result res SOCI_DUMMY_INIT(ef_success);

    // Usually we try to fetch the entire vector at once, but if some into
    // string columns are bigger than 8KB (ODBC_MAX_COL_SIZE) then we use
    // 100MB buffer for that columns. So in this case we downgrade to using
    // scalar fetches to hold the buffer only for a single row and not
    // rows_count * 100MB.
    // See odbc_vector_into_type_backend::define_by_pos().
    if (!fetchVectorByRows_)
    {
        SQLULEN row_array_size = static_cast<SQLULEN>(number);
        SQLSetStmtAttr(hstmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)row_array_size, 0);

        SQLSetStmtAttr(hstmt_, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched_, 0);

        res = do_fetch(0, number);
    }
    else // Use multiple calls to SQLFetch().
    {
        SQLULEN curNumRowsFetched = 0;
        SQLSetStmtAttr(hstmt_, SQL_ATTR_ROWS_FETCHED_PTR, &curNumRowsFetched, 0);

        for (int row = 0; row < number; ++row)
        {
            // Unfortunately we need to redefine all vector intos which
            // were bound to the first element of the vector initially.
            //
            // Note that we need to do it even for row == 0 as this might not
            // be the first call to fetch() and so the current bindings might
            // not be the same as initial ones.
            for (std::size_t j = 0; j != intos_.size(); ++j)
            {
                intos_[j]->rebind_row(row);
            }

            res = do_fetch(row, row + 1);
            if (res != ef_success)
                break;

            numRowsFetched_ += curNumRowsFetched;
        }
    }

    return res;
}

long long odbc_statement_backend::get_affected_rows()
{
    return rowsAffected_;
}

int odbc_statement_backend::get_number_of_rows()
{
    return static_cast<int>(numRowsFetched_);
}

std::string odbc_statement_backend::get_parameter_name(int index) const
{
    return names_.at(index);
}

std::string odbc_statement_backend::rewrite_for_procedure_call(
    std::string const &query)
{
    return query;
}

int odbc_statement_backend::prepare_for_describe()
{
    SQLSMALLINT numCols;
    SQLRETURN rc = SQLNumResultCols(hstmt_, &numCols);
    if (is_odbc_error(rc))
    {
        throw soci_error("Failed to get result columns count");
    }
    return numCols;
}

void odbc_statement_backend::describe_column(int colNum,
                                          db_type & dbtype,
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
        std::ostringstream ss;
        ss << "getting description of column at position " << colNum;
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_, ss.str());
    }

    char const *name = reinterpret_cast<char const *>(colNameBuffer);
    columnName.assign(name, std::strlen(name));

    SQLLEN is_unsigned = 0;
    SQLRETURN rc_colattr = SQLColAttribute(hstmt_, static_cast<SQLUSMALLINT>(colNum),
                                           SQL_DESC_UNSIGNED, 0, 0, 0, &is_unsigned);

    if (is_odbc_error(rc_colattr))
    {
        std::ostringstream ss;
        ss << "getting \"unsigned\" column attribute of the column at position " << colNum;
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_, ss.str());
    }

    switch (dataType)
    {
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
        dbtype = db_date;
        break;
    case SQL_DOUBLE:
    case SQL_DECIMAL:
    case SQL_REAL:
    case SQL_FLOAT:
    case SQL_NUMERIC:
        dbtype = db_double;
        break;
    case SQL_TINYINT:
        dbtype = is_unsigned == SQL_TRUE ? db_uint8 : db_int8;
        break;
    case SQL_SMALLINT:
        dbtype = is_unsigned == SQL_TRUE ? db_uint16 : db_int16;
        break;
    case SQL_INTEGER:
        dbtype = is_unsigned == SQL_TRUE ? db_uint32 : db_int32;
        break;
    case SQL_BIGINT:
        dbtype = is_unsigned == SQL_TRUE ? db_uint64 : db_int64;
        break;
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
        dbtype = db_wstring;
        break;
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    default:
        dbtype = db_string;
        break;
    }
}

data_type odbc_statement_backend::to_data_type(db_type dbt) const
{
    // Before adding db_type, this backend returned signed integer constants
    // even for unsigned types, so preserve this behaviour.
    switch (dbt)
    {
        case db_uint32: return dt_integer;
        case db_uint64: return dt_long_long;
        default:        return statement_backend::to_data_type(dbt);
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
        std::ostringstream ss;
        ss << "getting size of column at position " << colNum;
        throw odbc_soci_error(SQL_HANDLE_STMT, hstmt_, ss.str());
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
    return new odbc_vector_use_type_backend(*this);
}
