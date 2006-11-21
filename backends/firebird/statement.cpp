//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "error.h"
#include <cctype>
#include <sstream>

using namespace SOCI;
using namespace SOCI::details;
using namespace SOCI::details::Firebird;

FirebirdStatementBackEnd::FirebirdStatementBackEnd(FirebirdSessionBackEnd &session)
        : session_(session), stmtp_(0), sqldap_(NULL), sqlda2p_(NULL), rowsFetched_(0),
        intoType_(eStandard), useType_(eStandard), procedure_(false), boundByName_(false),
		boundByPos_(false)
{}

void FirebirdStatementBackEnd::prepareSQLDA(XSQLDA ** sqldap, int size)
{
    if (*sqldap != NULL)
    {
        *sqldap = reinterpret_cast<XSQLDA*>(realloc(*sqldap,XSQLDA_LENGTH(size)));
    }
    else
    {
        *sqldap = reinterpret_cast<XSQLDA*>(malloc(XSQLDA_LENGTH(size)));
    }

    (*sqldap)->sqln = size;
    (*sqldap)->version = 1;
}

void FirebirdStatementBackEnd::alloc()
{
    ISC_STATUS stat[stat_size];

    if (isc_dsql_allocate_statement(stat, &session_.dbhp_, &stmtp_))
    {
        throwISCError(stat);
    }
}

void FirebirdStatementBackEnd::cleanUp()
{
    ISC_STATUS stat[stat_size];

    if (stmtp_ != NULL)
    {
        if (isc_dsql_free_statement(stat, &stmtp_, DSQL_drop))
        {
            throwISCError(stat);
        }
        stmtp_ = NULL;
    }

    if (sqldap_ != NULL)
    {
        free(sqldap_);
        sqldap_ = NULL;
    }

    if (sqlda2p_ != NULL)
    {
        free(sqlda2p_);
        sqlda2p_ = NULL;
    }
}

void FirebirdStatementBackEnd::rewriteParameters(
    std::string const & src, std::vector<char> & dst)
{
    std::vector<char>::iterator dst_it = dst.begin();

    // rewrite the query by transforming all named parameters into
    // the Firebird question marks (:abc -> ?, etc.)

    enum { eNormal, eInQuotes, eInName } state = eNormal;

    std::string name;
    int position = 0;

    for (std::string::const_iterator it = src.begin(), end = src.end();
            it != end; ++it)
    {
        switch (state)
        {
            case eNormal:
                if (*it == '\'')
                {
                    *dst_it++ = *it;
                    state = eInQuotes;
                }
                else if (*it == ':')
                {
                    state = eInName;
                }
                else // regular character, stay in the same state
                {
                    *dst_it++ = *it;
                }
                break;
            case eInQuotes:
                if (*it == '\'')
                {
                    *dst_it++ = *it;
                    state = eNormal;
                }
                else // regular quoted character
                {
                    *dst_it++ = *it;
                }
                break;
            case eInName:
                if (std::isalnum(*it) || *it == '_')
                {
                    name += *it;
                }
                else // end of name
                {
                    names_.insert(std::pair<std::string, int>(name, position++));
                    name.clear();
                    *dst_it++ = '?';
                    *dst_it++ = *it;
                    state = eNormal;
                }
                break;
        }
    }

    if (state == eInName)
    {
        names_.insert(std::pair<std::string, int>(name, position++));
        *dst_it++ = '?';
    }

    *dst_it = '\0';
}

namespace
{
    int statementType(isc_stmt_handle stmt)
    {
        int statement_type;
        int length;
        char type_item[] = {isc_info_sql_stmt_type};
        char res_buffer[8];

        ISC_STATUS stat[stat_size];

        if (isc_dsql_sql_info(stat, &stmt, sizeof(type_item),
                              type_item, sizeof(res_buffer), res_buffer))
        {
            throwISCError(stat);
        }

        if (res_buffer[0] == isc_info_sql_stmt_type)
        {
            length = isc_vax_integer(res_buffer+1, 2);
            statement_type = isc_vax_integer(res_buffer+3, length);
        }
        else
        {
            throw SOCIError("Can't determine statement type.");
        }

        return statement_type;
    }
}

void FirebirdStatementBackEnd::rewriteQuery(
    std::string const &query, std::vector<char> &buffer)
{
    // buffer for temporary query
    std::vector<char> tmpQuery;
    std::vector<char>::iterator qItr;

    // buffer for query with named parameters changed to standard ones
    std::vector<char> rewQuery(query.size() + 1);

    // take care of named parameters in original query
    rewriteParameters(query, rewQuery);

    std::string const prefix("execute procedure ");
    std::string const prefix2("select * from ");

    // for procedures, we are preparing statement to determine
    // type of procedure.
    if (procedure_)
    {
        tmpQuery.resize(prefix.size() + rewQuery.size());
        qItr = tmpQuery.begin();
        std::copy(prefix.begin(), prefix.end(), qItr);
        qItr += prefix.size();
    }
    else
    {
        tmpQuery.resize(rewQuery.size());
        qItr = tmpQuery.begin();
    }

    // prepare temporary query
    std::copy(rewQuery.begin(), rewQuery.end(), qItr);

    // preparing buffers for output parameters
    if (sqldap_ == NULL)
    {
        prepareSQLDA(&sqldap_);
    }

    ISC_STATUS stat[stat_size];
    isc_stmt_handle tmpStmtp = 0;

    // allocate temporary statement to determine its type
    if (isc_dsql_allocate_statement(stat, &session_.dbhp_, &tmpStmtp))
    {
        throwISCError(stat);
    }

    // prepare temporary statement
    if (isc_dsql_prepare(stat, &(session_.trhp_), &tmpStmtp, 0,
                         &tmpQuery[0], SQL_DIALECT_V6, sqldap_))
    {
        throwISCError(stat);
    }

    // get statement type
    int stType = statementType(tmpStmtp);

    // free temporary prepared statement
    if (isc_dsql_free_statement(stat, &tmpStmtp, DSQL_drop))
    {
        throwISCError(stat);
    }

    // take care of special cases
    if (procedure_)
    {
        // for procedures that return values, we need to use correct syntax
        if (sqldap_->sqld != 0)
        {
            // this is "select" procedure, so we have to change syntax
            buffer.resize(prefix2.size() + rewQuery.size());
            qItr = buffer.begin();
            std::copy(prefix2.begin(), prefix2.end(), qItr);
            qItr += prefix2.size();
            std::copy(rewQuery.begin(), rewQuery.end(), qItr);

            // that won't be needed anymore
            procedure_ = false;

            return;
        }
    }
    else
    {
        // this is not procedure, so syntax is ok except for named
        // parameters in ddl
        if (stType == isc_info_sql_stmt_ddl)
        {
            // this statement is a DDL - we can't rewrite named parameters
            // so, we will use original query
            buffer.resize(query.size() + 1);
            std::copy(query.begin(), query.end(), buffer.begin());

            // that won't be needed anymore
            procedure_ = false;

            return;
        }
    }

    // here we know, that temporary query is OK, so we leave it as is
    buffer.resize(tmpQuery.size());
    std::copy(tmpQuery.begin(), tmpQuery.end(), buffer.begin());

    // that won't be needed anymore
    procedure_ = false;
}

void FirebirdStatementBackEnd::prepare(std::string const & query,
                                       eStatementType /* eType */)
{
    // clear named parametes
    names_.clear();

    std::vector<char> queryBuffer;

    // modify query's syntax and prepare buffer for use with
    // firebird's api
    rewriteQuery(query, queryBuffer);

    ISC_STATUS stat[stat_size];

    // prepare real statement
    if (isc_dsql_prepare(stat, &(session_.trhp_), &stmtp_, 0,
                         &queryBuffer[0], SQL_DIALECT_V6, sqldap_))
    {
        throwISCError(stat);
    }

    if (sqldap_->sqln < sqldap_->sqld)
    {
        // sqlda is too small for all columns. it must be reallocated
        prepareSQLDA(&sqldap_,sqldap_->sqld);

        if (isc_dsql_describe(stat, &stmtp_, SQL_DIALECT_V6, sqldap_))
        {
            throwISCError(stat);
        }
    }

    // preparing input parameters
    if (sqlda2p_ == NULL)
    {
        prepareSQLDA(&sqlda2p_);
    }

    if (isc_dsql_describe_bind(stat, &stmtp_, SQL_DIALECT_V6, sqlda2p_))
    {
        throwISCError(stat);
    }

    if (sqlda2p_->sqln < sqlda2p_->sqld)
    {
        // sqlda is too small for all columns. it must be reallocated
        prepareSQLDA(&sqlda2p_, sqlda2p_->sqld);

        if (isc_dsql_describe_bind(stat, &stmtp_, SQL_DIALECT_V6, sqlda2p_))
        {
            throwISCError(stat);
        }
    }

    // prepare buffers for indicators
    inds_.clear();
    inds_.resize(sqldap_->sqld);

    // reset types of into buffers
    intoType_ = eStandard;
    intos_.resize(0);

    // reset types of use buffers
    useType_ = eStandard;
    uses_.resize(0);
}


namespace
{
    void checkSize(std::size_t actual, std::size_t expected,
                   std::string const & name)
    {
        if (actual != expected)
        {
            std::ostringstream msg;
            msg << "Incorrect number of " << name << " variables. "
            << "Expected " << expected << ", got " << actual;
            throw SOCIError(msg.str());
        }
    }
}

StatementBackEnd::execFetchResult
FirebirdStatementBackEnd::execute(int number)
{
    ISC_STATUS stat[stat_size];
    XSQLDA *t = NULL;

    std::size_t usize = uses_.size();

    // do we have enough into variables ?
    checkSize(intos_.size(), sqldap_->sqld, "into");
    // do we have enough use variables ?
    checkSize(usize, sqlda2p_->sqld, "use");

    // do we have parameters ?
    if (sqlda2p_->sqld)
    {
        t = sqlda2p_;

        if (useType_ == eStandard)
        {
            for (std::size_t col=0; col<usize; ++col)
            {
                static_cast<FirebirdStandardUseTypeBackEnd*>(uses_[col])->exchangeData();
            }
        }
    }

    // make sure there is no active cursor
    if (isc_dsql_free_statement(stat, &stmtp_, DSQL_close))
    {
        // ignore attempt to close already closed cursor
        if (!checkISCError(stat, isc_dsql_cursor_close_err))
        {
            throwISCError(stat);
        }
    }

    if (useType_ == eVector)
    {
        // Here we have to explicitly loop to achieve the
        // effect of inserting or updating with vector use elements.
        std::size_t rows = static_cast<FirebirdVectorUseTypeBackEnd*>(uses_[0])->size();
        for (std::size_t row=0; row < rows; ++row)
        {
            // first we have to prepare input parameters
            for (std::size_t col=0; col<usize; ++col)
            {
                static_cast<FirebirdVectorUseTypeBackEnd*>(uses_[col])->exchangeData(row);
            }

            // then execute query
            if (isc_dsql_execute(stat, &session_.trhp_, &stmtp_, SQL_DIALECT_V6, t))
            {
                throwISCError(stat);
            }

            // SOCI does not allow bulk insert/update and bulk select operations
            // in same query. So here, we know that into elements are not
            // vectors. So, there is no need to fetch data here.
        }
    }
    else
    {
        // use elements aren't vectors
        if (isc_dsql_execute(stat, &session_.trhp_, &stmtp_, SQL_DIALECT_V6, t))
        {
            throwISCError(stat);
        }
    }

    if (sqldap_->sqld)
    {
        // query may return some data
        if (number > 0)
        {
            // number contains size of input variables, so we may fetch() data here
            return fetch(number);
        }
        else
        {
            // execute(0) was meant to only perform the query
            return eSuccess;
        }
    }
    else
    {
        // query can't return any data
        return eNoData;
    }
}

StatementBackEnd::execFetchResult
FirebirdStatementBackEnd::fetch(int number)
{
    ISC_STATUS stat[stat_size];

    for (size_t i = 0; i<static_cast<unsigned int>(sqldap_->sqld); ++i)
    {
        inds_[i].resize(number > 0 ? number : 1);
    }

    // Here we have to explicitly loop to achieve the effect of fetching
    // vector into elements. After each fetch, we have to exchange data
    // with into buffers.
    rowsFetched_ = 0;
    for (int i = 0; i < number; ++i)
    {
        long fetch_stat = isc_dsql_fetch(stat, &stmtp_, SQL_DIALECT_V6, sqldap_);

        // there is more data to read
        if (fetch_stat == 0)
        {
            ++rowsFetched_;
            exchangeData(true, i);
        }
        else if (fetch_stat == 100L)
        {
            return eNoData;
        }
        else
        {
            // error
            throwISCError(stat);
            return eNoData; // unreachable, for compiler only
        }
    } // for

    return eSuccess;
}

// here we put data fetched from database into user buffers
void FirebirdStatementBackEnd::exchangeData(bool gotData, int row)
{
    // first save indicators
    for (size_t i = 0; i < static_cast<unsigned int>(sqldap_->sqld); ++i)
    {
        if (!gotData)
        {
            inds_[i][row] = eIndicator(eNoData);
        }
        else
        {
            if (((sqldap_->sqlvar+i)->sqltype & 1) == 0)
            {
                // there is no indicator for this column
                inds_[i][row] = eOK;
            }
            else if (*((sqldap_->sqlvar+i)->sqlind) == 0)
            {
                inds_[i][row] = eOK;
            }
            else if (*((sqldap_->sqlvar+i)->sqlind) == -1)
            {
                inds_[i][row] = eNull;
            }
            else
            {
                throw SOCIError("Unknown state in FirebirdStatementBackEnd::exchangeData()");
            }
        }
    }

    // then deal with data
    if (gotData)
    {
        for (size_t i = 0; i<static_cast<unsigned int>(sqldap_->sqld); ++i)
        {
            if (inds_[i][row] != eNull)
            {
                if (intoType_ == eVector)
                {
                    static_cast<FirebirdVectorIntoTypeBackEnd*>(
                        intos_[i])->exchangeData(row);
                }
                else
                {
                    static_cast<FirebirdStandardIntoTypeBackEnd*>(
                        intos_[i])->exchangeData();
                }
            }
        }
    }
}

int FirebirdStatementBackEnd::getNumberOfRows()
{
    return rowsFetched_;
}

std::string FirebirdStatementBackEnd::rewriteForProcedureCall(
    std::string const &query)
{
    procedure_ = true;
    return query;
}

int FirebirdStatementBackEnd::prepareForDescribe()
{
    return static_cast<int>(sqldap_->sqld);
}

void FirebirdStatementBackEnd::describeColumn(int colNum,
        eDataType & type, std::string & columnName)
{
    XSQLVAR * var = sqldap_->sqlvar+(colNum-1);

    columnName.assign(var->aliasname, var->aliasname_length);

    switch (var->sqltype & ~1)
    {
        case SQL_TEXT:
        case SQL_VARYING:
            type = eString;
            break;
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TIMESTAMP:
            type = eDate;
            break;
        case SQL_FLOAT:
        case SQL_DOUBLE:
            type = eDouble;
            break;
        case SQL_SHORT:
        case SQL_LONG:
            if (var->sqlscale < 0)
            {
                type = eDouble;
            }
            else
            {
                type = eInteger;
            }
            break;
        case SQL_INT64:
            if (var->sqlscale < 0)
            {
                type = eDouble;
            }
            else
            {
                // 64bit integers are not supported
                std::ostringstream msg;
                msg << "Type of column ["<< colNum << "] \"" << columnName
                << "\" is not supported for dynamic queries";
                throw SOCIError(msg.str());
            }
            break;
            /* case SQL_BLOB:
            case SQL_ARRAY:*/
        default:
            std::ostringstream msg;
            msg << "Type of column ["<< colNum << "] \"" << columnName
            << "\" is not supported for dynamic queries";
            throw SOCIError(msg.str());
            break;
    }
}

FirebirdStandardIntoTypeBackEnd * FirebirdStatementBackEnd::makeIntoTypeBackEnd()
{
    return new FirebirdStandardIntoTypeBackEnd(*this);
}

FirebirdStandardUseTypeBackEnd * FirebirdStatementBackEnd::makeUseTypeBackEnd()
{
    return new FirebirdStandardUseTypeBackEnd(*this);
}

FirebirdVectorIntoTypeBackEnd * FirebirdStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new FirebirdVectorIntoTypeBackEnd(*this);
}

FirebirdVectorUseTypeBackEnd * FirebirdStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new FirebirdVectorUseTypeBackEnd(*this);
}
