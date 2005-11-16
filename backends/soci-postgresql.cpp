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
#include <ctime>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;


// namespace // anonymous
// {

// // definitions of basic data types (OIDs from pg_type system table)
// // (currently not used, because all results are in text format anyway)
// int const pg_type_Int4 = 23;

// bool hostIsLittleEndian;

// // helper function for copying binary data with big-endian/little-endian
// // converion (as stated by the above flag)
// void copyBinary(void *dst, void *src, std::size_t size)
// {
//     char *s = static_cast<char*>(src);
//     char *d = static_cast<char*>(dst);

//     if (hostIsLittleEndian)
//     {
//         // convert data (swap bytes)
//         s += size - 1;
//         for (; size != 0; --size)
//         {
//             *d++ = *s--;
//         }
//     }
//     else
//     {
//         // no coversion
//         for (; size != 0; --size)
//         {
//             *d++ = *s++;
//         }
//     }
// }

// } // namespace anonymous


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

void PostgreSQLSessionBackEnd::commit()
{
    // TODO:
}

void PostgreSQLSessionBackEnd::rollback()
{
    // TODO:
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
    // TODO:
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
    query_ = query;
}

StatementBackEnd::execFetchResult
PostgreSQLStatementBackEnd::execute(int number)
{
    if (number == 0)
    {
        // In the Oracle world, this means that the statement needs to be
        // executed, but no data should be fetched.
        // This is different in PostgreSQL: we *have to* execute the statement
        // and get data from the server, but the actual "fetch" will be
        // performed later.
        number = 1;
    }

    if (number != 1)
    {
        throw SOCIError("Only unit excutions are supported.");
    }

    result_ = PQexec(session_.conn_, query_.c_str());
    if (result_ == NULL)
    {
        throw SOCIError("Cannot execute query.");
    }
    
    ExecStatusType status = PQresultStatus(result_);
    if (status == PGRES_TUPLES_OK)
    {
        numberOfRows_ = PQntuples(result_);
        if (numberOfRows_ == 0)
        {
            return eNoData;
        }
        else
        {
            currentRow_ = 0;
            return eSuccess;
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

int PostgreSQLStatementBackEnd::prepareForDescribe()
{
    // TODO:
    return 0;
}

void PostgreSQLStatementBackEnd::describeColumn(int colNum, eDataType &type,
    std::string &columnName, int &size, int &precision, int &scale,
    bool &nullOk)
{
    // TODO:
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
    position_ = position;
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

// Note: for the moment, only text format is used for output data
//         // format: 0-text, 1-binary
//         int format = PQfformat(statement_.result_, pos);
// 
//         // server data type
//         int srvDataType = PQftype(statement_.result_, pos);

        // raw data, in whatever format it is
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

        default:
            throw SOCIError("Into element used with non-supported type.");
        }

        ++statement_.currentRow_;
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
    position_ = position;
}

void PostgreSQLVectorIntoTypeBackEnd::preFetch()
{
    // nothing to do here
}

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
                {
                    std::vector<char> *dest =
                        static_cast<std::vector<char> *>(data_);

                    std::vector<char> &v = *dest;
                    v[i] = *buf;
                }
                break;
            case eXStdString:
                {
                    std::vector<std::string> *dest =
                        static_cast<std::vector<std::string> *>(data_);

                    std::vector<std::string> &v = *dest;
                    v[i] = buf;
                }
                break;
            case eXShort:
                {
                    std::vector<short> *dest =
                        static_cast<std::vector<short> *>(data_);

                    std::vector<short> &v = *dest;
                    long val = strtol(buf, NULL, 10);
                    v[i] = static_cast<short>(val);
                }
                break;
            case eXInteger:
                {
                    std::vector<int> *dest =
                        static_cast<std::vector<int> *>(data_);

                    std::vector<int> &v = *dest;
                    long val = strtol(buf, NULL, 10);
                    v[i] = static_cast<int>(val);
                }
                break;
            case eXUnsignedLong:
                {
                    std::vector<unsigned long> *dest =
                        static_cast<std::vector<unsigned long> *>(data_);

                    std::vector<unsigned long> &v = *dest;
                    long long val = strtoll(buf, NULL, 10);
                    v[i] = static_cast<unsigned long>(val);
                }
                break;
            case eXDouble:
                {
                    std::vector<double> *dest =
                        static_cast<std::vector<double> *>(data_);

                    std::vector<double> &v = *dest;
                    double val = strtod(buf, NULL);
                    v[i] = static_cast<double>(val);
                }
                break;
            case eXStdTm:
                {
                    // attempt to parse the string and convert to std::tm
                    std::tm t;
                    parseStdTm(buf, t);

                    std::vector<std::tm> *dest =
                        static_cast<std::vector<std::tm> *>(data_);

                    std::vector<std::tm> &v = *dest;
                    v[i] = t;
                }
                break;

            default:
                throw SOCIError("Into element used with non-supported type.");
            }
        }

        statement_.currentRow_ += statement_.rowsToConsume_;
    }
    else // no data retrieved
    {
        // nothing to do, into vectors are already truncated
    }
}

void PostgreSQLVectorIntoTypeBackEnd::resize(std::size_t sz)
{
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            v->resize(sz);
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            v->resize(sz);
        }
        break;
    case eXInteger:
        {
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
            v->resize(sz);
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            v->resize(sz);
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            v->resize(sz);
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            v->resize(sz);
        }
        break;

    default:
        throw SOCIError("Into element used with non-supported type.");
    }
}

std::size_t PostgreSQLVectorIntoTypeBackEnd::size()
{
    std::size_t sz = 0; // dummy initialization to please the compiler
    switch (type_)
    {
    // simple cases
    case eXChar:
        {
            std::vector<char> *v = static_cast<std::vector<char> *>(data_);
            sz = v->size();
        }
        break;
    case eXShort:
        {
            std::vector<short> *v = static_cast<std::vector<short> *>(data_);
            sz = v->size();
        }
        break;
    case eXInteger:
        {
            std::vector<int> *v = static_cast<std::vector<int> *>(data_);
            sz = v->size();
        }
        break;
    case eXUnsignedLong:
        {
            std::vector<unsigned long> *v
                = static_cast<std::vector<unsigned long> *>(data_);
            sz = v->size();
        }
        break;
    case eXDouble:
        {
            std::vector<double> *v
                = static_cast<std::vector<double> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdString:
        {
            std::vector<std::string> *v
                = static_cast<std::vector<std::string> *>(data_);
            sz = v->size();
        }
        break;
    case eXStdTm:
        {
            std::vector<std::tm> *v
                = static_cast<std::vector<std::tm> *>(data_);
            sz = v->size();
        }
        break;

    default:
        throw SOCIError("Into element used with non-supported type.");
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
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    // TODO:
}

void PostgreSQLStandardUseTypeBackEnd::cleanUp()
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::bindByPos(int &position,
        void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    // TODO:
}

void PostgreSQLVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    // TODO:
}

std::size_t PostgreSQLVectorUseTypeBackEnd::size()
{
    // TODO:
    return 0;
}

void PostgreSQLVectorUseTypeBackEnd::cleanUp()
{
    // TODO:
}

PostgreSQLRowIDBackEnd::PostgreSQLRowIDBackEnd(
    PostgreSQLSessionBackEnd &session)
{
    // TODO:
}

PostgreSQLRowIDBackEnd::~PostgreSQLRowIDBackEnd()
{
    // TODO:
}

PostgreSQLBLOBBackEnd::PostgreSQLBLOBBackEnd(
    PostgreSQLSessionBackEnd &session)
    : session_(session)
{
    // TODO:
}

PostgreSQLBLOBBackEnd::~PostgreSQLBLOBBackEnd()
{
    // TODO:
}

std::size_t PostgreSQLBLOBBackEnd::getLen()
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::read(
    std::size_t offset, char *buf, std::size_t toRead)
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::write(
    std::size_t offset, char const *buf, std::size_t toWrite)
{
    // TODO:
    return 0;
}

std::size_t PostgreSQLBLOBBackEnd::append(
    char const *buf, std::size_t toWrite)
{
    // TODO:
    return 0;
}

void PostgreSQLBLOBBackEnd::trim(std::size_t newLen)
{
    // TODO:
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

//         // in addition, determine the local byte order
//         // this is a simple check, we do not expect exotic platforms

//         int dummy = 1;
//         char *p = reinterpret_cast<char*>(&dummy);
//         hostIsLittleEndian = (*p != 0);
    }
} postgresqlAutoRegister;

} // namespace anonymous
