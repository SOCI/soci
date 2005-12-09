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
#include <cstring>
#include <sstream>
#include <cstdio>
#include <ctime>
#include <cctype>
#include <libpq/libpq-fs.h>


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

namespace // anonymous
{

// helper function for hardoded queries
void hardExec(PGconn *conn, char const *query, char const *errMsg)
{
    PGresult *result = PQexec(conn, query);

    if (result == NULL)
    {
        throw SOCIError(errMsg);
    }

    ExecStatusType status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK)
    {
        throw SOCIError(PQresultErrorMessage(result));
    }

    PQclear(result);
}

} // namespace anonymous

void PostgreSQLSessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN", "Cannot begin transaction.");
}

void PostgreSQLSessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT", "Cannot commit transaction.");
}

void PostgreSQLSessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK", "Cannot rollback transaction.");
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

void PostgreSQLStatementBackEnd::prepare(std::string const &query)
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
}

StatementBackEnd::execFetchResult
PostgreSQLStatementBackEnd::execute(int number)
{
    if (!useByPosBuffers_.empty() || !useByNameBuffers_.empty())
    {
        // Here we have to explicitly loop to achieve the
        // effect of inserting or updating with vector use elements.
        // The 'number' parameter to this function comes from the
        // core part of the library and is guaranteed to be the size
        // of the use elements, if they are present (they have equal sizes).
        // If use elements were specified with single variables, it is 1.
        // If use elements were specified for vectors,
        // it is the size of those vectors.

        // We know also that even if there are both use and into elements
        // specified together, they are not for bulk queries (number == 1).

        if (!useByPosBuffers_.empty() && !useByNameBuffers_.empty())
        {
            throw SOCIError(
                "Binding for use elements must be either by position "
                "or by name.");
        }

        for (int i = 0; i != number; ++i)
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
            result_ = PQexecParams(session_.conn_, query_.c_str(),
                static_cast<int>(paramValues.size()),
                NULL, &paramValues[0], NULL, NULL, 0);
#endif // SOCI_PGSQL_NOPARAMS

            if (number > 1)
            {
                // there are only use elements (no intos)
                // and it is a bulk operation
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

        if (number > 1)
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
        result_ = PQexec(session_.conn_, query_.c_str());

        if (result_ == NULL)
        {
            throw SOCIError("Cannot execute query.");
        }
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
    throw SOCIError("Dynamic row description is not supported.");
    return 0; // non-reachable
}

void PostgreSQLStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName, int &size, int &precision, int &scale,
    bool &nullOk)
{
    throw SOCIError("Dynamic row description is not supported.");
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
    data_ = data;
    type_ = type;
    position_ = position++;
}

void PostgreSQLStandardIntoTypeBackEnd::preFetch()
{
    // nothing to do here
}


namespace // anonymous
{

// helper function for parsing decimal data (for std::tm)
long parse10(char const *&p1, char *&p2, char *msg)
{
    long v = strtol(p1, &p2, 10);
    if (p2 != p1)
    {
        p1 = p2 + 1;
        return v;
    }
    else
    {
        throw SOCIError(msg);
    }
}

void parseStdTm(char const *buf, std::tm &t)
{
    char const *p1 = buf;
    char *p2;
    long year, month, day;
    long hour = 0, minute = 0, second = 0;

    char *errMsg = "Cannot convert data to std::tm.";

    year  = parse10(p1, p2, errMsg);
    month = parse10(p1, p2, errMsg);
    day   = parse10(p1, p2, errMsg);

    if (*p2 != '\0')
    {
        // there is also the time of day available
        hour   = parse10(p1, p2, errMsg);
        minute = parse10(p1, p2, errMsg);
        second = parse10(p1, p2, errMsg);
    }

    t.tm_isdst = -1;
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;

    std::mktime(&t);
}

} // namespace anonymous

void PostgreSQLStandardIntoTypeBackEnd::postFetch(
    bool gotData, bool calledFromFetch, eIndicator *ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    if (gotData)
    {
        // PostgreSQL positions start at 0
        int pos = position_ - 1;

        // first, deal with indicators
        if (PQgetisnull(statement_.result_, statement_.currentRow_, pos) != 0)
        {
            if (ind == NULL)
            {
                throw SOCIError(
                    "Null value fetched and no indicator defined.");
            }

            *ind = eNull;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = eOK;
            }
        }

        // raw data, in text format
        char *buf = PQgetvalue(statement_.result_,
            statement_.currentRow_, pos);

        switch (type_)
        {
        case eXChar:
            {
                char *dest = static_cast<char*>(data_);
                *dest = *buf;
            }
            break;
        case eXCString:
            {
                CStringDescriptor *strDescr
                    = static_cast<CStringDescriptor *>(data_);

                std::strncpy(strDescr->str_, buf, strDescr->bufSize_ - 1);
                strDescr->str_[strDescr->bufSize_ - 1] = '\0';

                if (std::strlen(buf) >= strDescr->bufSize_ && ind != NULL)
                {
                    *ind = eTruncated;
                }
            }
            break;
        case eXStdString:
            {
                std::string *dest = static_cast<std::string *>(data_);
                dest->assign(buf);
            }
            break;
        case eXShort:
            {
                short *dest = static_cast<short*>(data_);
                long val = strtol(buf, NULL, 10);
                *dest = static_cast<short>(val);
            }
            break;
        case eXInteger:
            {
                int *dest = static_cast<int*>(data_);
                long val = strtol(buf, NULL, 10);
                *dest = static_cast<int>(val);
            }
            break;
        case eXUnsignedLong:
            {
                unsigned long *dest = static_cast<unsigned long *>(data_);
                long long val = strtoll(buf, NULL, 10);
                *dest = static_cast<unsigned long>(val);
            }
            break;
        case eXDouble:
            {
                double *dest = static_cast<double*>(data_);
                double val = strtod(buf, NULL);
                *dest = static_cast<double>(val);
            }
            break;
        case eXStdTm:
            {
                // attempt to parse the string and convert to std::tm
                std::tm *dest = static_cast<std::tm *>(data_);
                parseStdTm(buf, *dest);
            }
            break;
        case eXRowID:
            {
                // RowID is internally identical to unsigned long

                RowID *rid = static_cast<RowID *>(data_);
                PostgreSQLRowIDBackEnd *rbe
                    = static_cast<PostgreSQLRowIDBackEnd *>(
                        rid->getBackEnd());

                long long val = strtoll(buf, NULL, 10);
                rbe->value_ = static_cast<unsigned long>(val);
            }
            break;
        case eXBLOB:
            {
                long long llval = strtoll(buf, NULL, 10);
                unsigned long oid = static_cast<unsigned long>(llval);

                int fd = lo_open(statement_.session_.conn_, oid,
                    INV_READ | INV_WRITE);
                if (fd == -1)
                {
                    throw SOCIError("Cannot open the BLOB object.");
                }

                BLOB *b = static_cast<BLOB *>(data_);
                PostgreSQLBLOBBackEnd *bbe
                     = static_cast<PostgreSQLBLOBBackEnd *>(b->getBackEnd());

                if (bbe->fd_ != -1)
                {
                    lo_close(statement_.session_.conn_, bbe->fd_);
                }

                bbe->fd_ = fd;
            }
            break;

        default:
            throw SOCIError("Into element used with non-supported type.");
        }
    }
    else // no data retrieved
    {
        if (ind != NULL)
        {
            *ind = eNoData;
        }
        else
        {
            throw SOCIError("No data fetched and no indicator defined.");
        }
    }
}

void PostgreSQLStandardIntoTypeBackEnd::cleanUp()
{
    // nothing to do here
}

void PostgreSQLVectorIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void PostgreSQLVectorIntoTypeBackEnd::preFetch()
{
    // nothing to do here
}

namespace // anonymous
{

template <typename T, typename U>
void setInVector(void *p, int indx, U const &val)
{
    std::vector<T> *dest =
        static_cast<std::vector<T> *>(p);

    std::vector<T> &v = *dest;
    v[indx] = val;
}

} // namespace anonymous

void PostgreSQLVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // Here, rowsToConsume_ in the Statement object designates
        // the number of rows that need to be put in the user's buffers.

        // PostgreSQL column positions start at 0
        int pos = position_ - 1;

        int const endRow = statement_.currentRow_ + statement_.rowsToConsume_;

        for (int curRow = statement_.currentRow_, i = 0;
             curRow != endRow; ++curRow, ++i)
        {
            // first, deal with indicators
            if (PQgetisnull(statement_.result_, curRow, pos) != 0)
            {
                if (ind == NULL)
                {
                    throw SOCIError(
                        "Null value fetched and no indicator defined.");
                }

                ind[i] = eNull;
            }
            else
            {
                if (ind != NULL)
                {
                    ind[i] = eOK;
                }
            }

            // buffer with data retrieved from server, in text format
            char *buf = PQgetvalue(statement_.result_, curRow, pos);

            switch (type_)
            {
            case eXChar:
                setInVector<char>(data_, i, *buf);
                break;
            case eXStdString:
                setInVector<std::string>(data_, i, buf);
                break;
            case eXShort:
                {
                    long val = strtol(buf, NULL, 10);
                    setInVector<short>(data_, i, static_cast<short>(val));
                }
                break;
            case eXInteger:
                {
                    long val = strtol(buf, NULL, 10);
                    setInVector<int>(data_, i, static_cast<int>(val));
                }
                break;
            case eXUnsignedLong:
                {
                    long long val = strtoll(buf, NULL, 10);
                    setInVector<unsigned long>(data_, i,
                        static_cast<unsigned long>(val));
                }
                break;
            case eXDouble:
                {
                    double val = strtod(buf, NULL);
                    setInVector<double>(data_, i, val);
                }
                break;
            case eXStdTm:
                {
                    // attempt to parse the string and convert to std::tm
                    std::tm t;
                    parseStdTm(buf, t);

                    setInVector<std::tm>(data_, i, t);
                }
                break;

            default:
                throw SOCIError("Into element used with non-supported type.");
            }
        }
    }
    else // no data retrieved
    {
        // nothing to do, into vectors are already truncated
    }
}

namespace // anonymous
{

template <typename T>
void resizeVector(void *p, std::size_t sz)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    v->resize(sz);
}

template <typename T>
std::size_t getVectorSize(void *p)
{
    std::vector<T> *v = static_cast<std::vector<T> *>(p);
    return v->size();
}

} // namespace anonymous

void PostgreSQLVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    switch (type_)
    {
    // simple cases
    case eXChar:         resizeVector<char>         (data_, sz); break;
    case eXShort:        resizeVector<short>        (data_, sz); break;
    case eXInteger:      resizeVector<int>          (data_, sz); break;
    case eXUnsignedLong: resizeVector<unsigned long>(data_, sz); break;
    case eXDouble:       resizeVector<double>       (data_, sz); break;
    case eXStdString:    resizeVector<std::string>  (data_, sz); break;
    case eXStdTm:        resizeVector<std::tm>      (data_, sz); break;

    default:
        throw SOCIError("Into vector element used with non-supported type.");
    }
}

std::size_t PostgreSQLVectorIntoTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:         sz = getVectorSize<char>         (data_); break;
    case eXShort:        sz = getVectorSize<short>        (data_); break;
    case eXInteger:      sz = getVectorSize<int>          (data_); break;
    case eXUnsignedLong: sz = getVectorSize<unsigned long>(data_); break;
    case eXDouble:       sz = getVectorSize<double>       (data_); break;
    case eXStdString:    sz = getVectorSize<std::string>  (data_); break;
    case eXStdTm:        sz = getVectorSize<std::tm>      (data_); break;

    default:
        throw SOCIError("Into vector element used with non-supported type.");
    }

    return sz;
}

void PostgreSQLVectorIntoTypeBackEnd::cleanUp()
{
    // nothing to do here
}

void PostgreSQLStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void PostgreSQLStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void PostgreSQLStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    if (ind != NULL && *ind == eNull)
    {
        // leave the working buffer as NULL
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case eXChar:
            {
                buf_ = new char[2];
                buf_[0] = *static_cast<char*>(data_);
                buf_[1] = '\0';
            }
            break;
        case eXCString:
            {
                CStringDescriptor *strDescr
                    = static_cast<CStringDescriptor *>(data_);

                std::size_t len = std::strlen(strDescr->str_);
                buf_ = new char[len + 1];
                std::strcpy(buf_, strDescr->str_);
            }
            break;
        case eXStdString:
            {
                std::string *s = static_cast<std::string *>(data_);
                buf_ = new char[s->size() + 1];
                std::strcpy(buf_, s->c_str());
            }
            break;
        case eXShort:
            {
                std::size_t const bufSize
                    = std::numeric_limits<short>::digits10 + 3;
                buf_ = new char[bufSize];
                std::snprintf(buf_, bufSize, "%d",
                    static_cast<int>(*static_cast<short*>(data_)));
            }
            break;
        case eXInteger:
            {
                std::size_t const bufSize
                    = std::numeric_limits<int>::digits10 + 3;
                buf_ = new char[bufSize];
                std::snprintf(buf_, bufSize, "%d",
                    *static_cast<int*>(data_));
            }
            break;
        case eXUnsignedLong:
            {
                std::size_t const bufSize
                    = std::numeric_limits<unsigned long>::digits10 + 2;
                buf_ = new char[bufSize];
                std::snprintf(buf_, bufSize, "%lu",
                    *static_cast<unsigned long*>(data_));
            }
            break;
        case eXDouble:
            {
                // no need to overengineer it (KISS)...

                std::size_t const bufSize = 100;
                buf_ = new char[bufSize];

                std::snprintf(buf_, bufSize, "%.20g",
                    *static_cast<double*>(data_));
            }
            break;
        case eXStdTm:
            {
                std::size_t const bufSize = 20;
                buf_ = new char[bufSize];

                std::tm *t = static_cast<std::tm *>(data_);
                std::snprintf(buf_, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec);
            }
            break;
        case eXRowID:
            {
                // RowID is internally identical to unsigned long

                RowID *rid = static_cast<RowID *>(data_);
                PostgreSQLRowIDBackEnd *rbe
                    = static_cast<PostgreSQLRowIDBackEnd *>(
                        rid->getBackEnd());

                std::size_t const bufSize
                    = std::numeric_limits<unsigned long>::digits10 + 2;
                buf_ = new char[bufSize];

                std::snprintf(buf_, bufSize, "%lu", rbe->value_);
            }
            break;

        default:
            throw SOCIError("Use element used with non-supported type.");
        }
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buf_;
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buf_;
    }
}

void PostgreSQLStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // TODO: if PostgreSQL allows to *get* data via this channel,
    // write it back to client buffers (variable)

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    cleanUp();
}

void PostgreSQLStandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}

void PostgreSQLVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void PostgreSQLVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void PostgreSQLVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    std::size_t const vsize = size();
    for (size_t i = 0; i != vsize; ++i)
    {
        char *buf;

        // the data in vector can be either eOK or eNull
        if (ind != NULL && ind[i] == eNull)
        {
            buf = NULL;
        }
        else
        {
            // allocate and fill the buffer with text-formatted client data
            switch (type_)
            {
            case eXChar:
                {
                    std::vector<char> *pv
                        = static_cast<std::vector<char> *>(data_);
                    std::vector<char> &v = *pv;

                    buf = new char[2];
                    buf[0] = v[i];
                    buf[1] = '\0';
                }
                break;
            case eXStdString:
                {
                    std::vector<std::string> *pv
                        = static_cast<std::vector<std::string> *>(data_);
                    std::vector<std::string> &v = *pv;

                    buf = new char[v[i].size() + 1];
                    std::strcpy(buf, v[i].c_str());
                }
                break;
            case eXShort:
                {
                    std::vector<short> *pv
                        = static_cast<std::vector<short> *>(data_);
                    std::vector<short> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<short>::digits10 + 3;
                    buf = new char[bufSize];
                    std::snprintf(buf, bufSize, "%d", static_cast<int>(v[i]));
                }
                break;
            case eXInteger:
                {
                    std::vector<int> *pv
                        = static_cast<std::vector<int> *>(data_);
                    std::vector<int> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<int>::digits10 + 3;
                    buf = new char[bufSize];
                    std::snprintf(buf, bufSize, "%d", v[i]);
                }
                break;
            case eXUnsignedLong:
                {
                    std::vector<unsigned long> *pv
                        = static_cast<std::vector<unsigned long> *>(data_);
                    std::vector<unsigned long> &v = *pv;

                    std::size_t const bufSize
                        = std::numeric_limits<unsigned long>::digits10 + 2;
                    buf = new char[bufSize];
                    std::snprintf(buf, bufSize, "%lu", v[i]);
                }
                break;
            case eXDouble:
                {
                    // no need to overengineer it (KISS)...

                    std::vector<double> *pv
                        = static_cast<std::vector<double> *>(data_);
                    std::vector<double> &v = *pv;

                    std::size_t const bufSize = 100;
                    buf = new char[bufSize];

                    std::snprintf(buf, bufSize, "%.20g", v[i]);
                }
                break;
            case eXStdTm:
                {
                    std::vector<std::tm> *pv
                        = static_cast<std::vector<std::tm> *>(data_);
                    std::vector<std::tm> &v = *pv;

                    std::size_t const bufSize = 20;
                    buf = new char[bufSize];

                    std::snprintf(buf, bufSize, "%d-%02d-%02d %02d:%02d:%02d",
                        v[i].tm_year + 1900, v[i].tm_mon + 1, v[i].tm_mday,
                        v[i].tm_hour, v[i].tm_min, v[i].tm_sec);
                }
                break;

            default:
                throw SOCIError(
                    "Use vector element used with non-supported type.");
            }
        }

        buffers_.push_back(buf);
    }

    if (position_ > 0)
    {
        // binding by position
        statement_.useByPosBuffers_[position_] = &buffers_[0];
    }
    else
    {
        // binding by name
        statement_.useByNameBuffers_[name_] = &buffers_[0];
    }
}

std::size_t PostgreSQLVectorUseTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:         sz = getVectorSize<char>         (data_); break;
    case eXShort:        sz = getVectorSize<short>        (data_); break;
    case eXInteger:      sz = getVectorSize<int>          (data_); break;
    case eXUnsignedLong: sz = getVectorSize<unsigned long>(data_); break;
    case eXDouble:       sz = getVectorSize<double>       (data_); break;
    case eXStdString:    sz = getVectorSize<std::string>  (data_); break;
    case eXStdTm:        sz = getVectorSize<std::tm>      (data_); break;

    default:
        throw SOCIError("Use vector element used with non-supported type.");
    }

    return sz;
}

void PostgreSQLVectorUseTypeBackEnd::cleanUp()
{
    std::size_t const bsize = buffers_.size();
    for (std::size_t i = 0; i != bsize; ++i)
    {
        delete [] buffers_[i];
    }
}

PostgreSQLRowIDBackEnd::PostgreSQLRowIDBackEnd(
    PostgreSQLSessionBackEnd &session)
{
    // nothing to do here
}

PostgreSQLRowIDBackEnd::~PostgreSQLRowIDBackEnd()
{
    // nothing to do here
}

PostgreSQLBLOBBackEnd::PostgreSQLBLOBBackEnd(
    PostgreSQLSessionBackEnd &session)
    : session_(session), fd_(-1)
{
    // nothing to do here, the descriptor is open in the postFetch
    // method of the Into element
}

PostgreSQLBLOBBackEnd::~PostgreSQLBLOBBackEnd()
{
    lo_close(session_.conn_, fd_);
}

std::size_t PostgreSQLBLOBBackEnd::getLen()
{
    int pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw SOCIError("Cannot retrieve the size of BLOB.");
    }

    return static_cast<std::size_t>(pos);
}

std::size_t PostgreSQLBLOBBackEnd::read(
    std::size_t offset, char *buf, std::size_t toRead)
{
    int pos = lo_lseek(session_.conn_, fd_, offset, SEEK_SET);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int readn = lo_read(session_.conn_, fd_, buf, toRead);
    if (readn < 0)
    {
        throw SOCIError("Cannot read from BLOB.");
    }

    return static_cast<std::size_t>(readn);
}

std::size_t PostgreSQLBLOBBackEnd::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    int pos = lo_lseek(session_.conn_, fd_, offset, SEEK_SET);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw SOCIError("Cannot write to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

std::size_t PostgreSQLBLOBBackEnd::append(
    char const *buf, std::size_t toWrite)
{
    int pos = lo_lseek(session_.conn_, fd_, 0, SEEK_END);
    if (pos == -1)
    {
        throw SOCIError("Cannot seek in BLOB.");
    }

    int writen = lo_write(session_.conn_, fd_,
        const_cast<char *>(buf), toWrite);
    if (writen < 0)
    {
        throw SOCIError("Cannot append to BLOB.");
    }

    return static_cast<std::size_t>(writen);
}

void PostgreSQLBLOBBackEnd::trim(std::size_t newLen)
{
    throw SOCIError("Trimming BLOBs is not supported.");
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
