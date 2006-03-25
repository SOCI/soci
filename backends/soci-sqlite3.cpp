//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci.h"
#include "soci-sqlite3.h"

#include <limits>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;

Sqlite3SessionBackEnd::Sqlite3SessionBackEnd(
    std::string const & connectString)
{
    int res;
    res = sqlite3_open(connectString.c_str(), &conn_);
    if (res != SQLITE_OK)
    {
        const char *zErrMsg = sqlite3_errmsg(conn_);

        std::ostringstream ss;
        ss << "Cannot establish connection to the database. "
           << zErrMsg;
        
        throw SOCIError(ss.str());
    }
}

Sqlite3SessionBackEnd::~Sqlite3SessionBackEnd()
{
    cleanUp();
}


namespace // anonymous
{

// helper function for hardcoded queries
void hardExec(sqlite3* conn, char const *query, char const *errMsg)
{
    char *zErrMsg = 0;
    int res = sqlite3_exec(conn, query, 0, 0, &zErrMsg);
    if (res != SQLITE_OK)
    {
        std::ostringstream ss;
        ss << errMsg << " "
           << zErrMsg;

        sqlite3_free(zErrMsg);
        
        throw SOCIError(ss.str());
    }
}

} // namespace anonymous

void Sqlite3SessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN", "Cannot begin transaction.");    
}

void Sqlite3SessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT", "Cannot commit transaction.");
}

void Sqlite3SessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void Sqlite3SessionBackEnd::cleanUp()
{
    sqlite3_close(conn_);
}

Sqlite3StatementBackEnd * Sqlite3SessionBackEnd::makeStatementBackEnd()
{
    return new Sqlite3StatementBackEnd(*this);
}

Sqlite3RowIDBackEnd * Sqlite3SessionBackEnd::makeRowIDBackEnd()
{
    return new Sqlite3RowIDBackEnd(*this);
}

Sqlite3BLOBBackEnd * Sqlite3SessionBackEnd::makeBLOBBackEnd()
{
    return new Sqlite3BLOBBackEnd(*this);
}

Sqlite3StatementBackEnd::Sqlite3StatementBackEnd(
    Sqlite3SessionBackEnd &session)
    : session_(session), stmt_(0), dataCache_(), useData_(0), 
      databaseReady_(false)
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

    assert(rows == number); // sanity check

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

void Sqlite3StandardIntoTypeBackEnd::defineByPos(int & position, void * data
                                                 , eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;    
}

void Sqlite3StandardIntoTypeBackEnd::preFetch()
{
    // ...
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

void Sqlite3StandardIntoTypeBackEnd::postFetch(bool gotData, 
                                               bool calledFromFetch, 
                                               eIndicator * ind)
{
    if (calledFromFetch == true && gotData == false)
    {
        // this is a normal end-of-rowset condition,
        // no need to do anything (fetch() will return false)
        return;
    }

    // sqlite columns start at 0
    int pos = position_ - 1;

    if (gotData)
    {
        // first, deal with indicators
        if (sqlite3_column_type(statement_.stmt_, pos) == SQLITE_NULL)
        {
            if (ind == NULL)
            {
                throw SOCIError(
                    "Null value fetched and no indicator defined.");
            }

            *ind = eNull;
            return;
        }
        else
        {
            if (ind != NULL)
            {
                *ind = eOK;
            }
        }

        const char *buf = 
        reinterpret_cast<const char*>(sqlite3_column_text(
                                          statement_.stmt_, 
                                          pos));

        if (!buf)
            buf = "";


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
            Sqlite3RowIDBackEnd *rbe
            = static_cast<Sqlite3RowIDBackEnd *>(
                rid->getBackEnd());
            long long val = strtoll(buf, NULL, 10);
            rbe->value_ = static_cast<unsigned long>(val);
        }
        break;
        case eXBLOB:
        {
#ifdef SQLITE_ENABLE_COLUMN_METADATA

            BLOB *b = static_cast<BLOB *>(data_);
            Sqlite3BLOBBackEnd *bbe =
                static_cast<Sqlite3BLOBBackEnd *>(b->getBackEnd());
            
            buf = reinterpret_cast<const char*>(sqlite3_column_blob(
                                          statement_.stmt_, 
                                          pos));

            int len = sqlite3_column_bytes(statement_.stmt_, pos);
            const char *tableName =
                sqlite3_column_table_name(statement_.stmt_, 
                                          pos);
            const char *columnName = sqlite3_column_name(statement_.stmt_, 
                                          pos);
            bbe->setData(tableName, columnName, buf, len);
#endif
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

void Sqlite3StandardIntoTypeBackEnd::cleanUp()
{
    // ...
}

void Sqlite3VectorIntoTypeBackEnd::defineByPos(int & position, void * data, 
                                               eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void Sqlite3VectorIntoTypeBackEnd::preFetch()
{
    // ...
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

void Sqlite3VectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator * ind)
{
    if (gotData)
    {
        int endRow = statement_.dataCache_.size();
        for (int i = 0; i < endRow; ++i)
        {
            const Sqlite3Column& curCol =
                statement_.dataCache_[i][position_-1];
            
            if (curCol.isNull_)
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

            const char * buf = curCol.data_.c_str();


            // set buf to a null string if a null pointer is returned
            if (!buf) 
            {
                buf = "";
            }

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


void Sqlite3VectorIntoTypeBackEnd::resize(std::size_t sz)
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

std::size_t Sqlite3VectorIntoTypeBackEnd::size()
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

void Sqlite3VectorIntoTypeBackEnd::cleanUp()
{
    // ...
}

void Sqlite3StandardUseTypeBackEnd::bindByPos(int & position, void * data, 
                                              eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void Sqlite3StandardUseTypeBackEnd::bindByName(std::string const & name, 
                                               void * data,
                                               eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = ":" + name;

    statement_.resetIfNeeded();
    position_ = sqlite3_bind_parameter_index(statement_.stmt_, name_.c_str());

    if (0 == position_)
    {
        std::ostringstream ss;
        ss << "Cannot bind to (by name) " << name_;
        throw SOCIError(ss.str());
    }
}

void Sqlite3StandardUseTypeBackEnd::preUse(eIndicator const * ind)
{
    statement_.useData_.resize(1);
    int pos = position_ - 1;

    if (statement_.useData_[0].size() < static_cast<std::size_t>(position_))
        statement_.useData_[0].resize(position_);

    if (ind != NULL && *ind == eNull)
    {
        statement_.useData_[0][pos].isNull_ = true;
        statement_.useData_[0][pos].data_ = "";
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
            Sqlite3RowIDBackEnd *rbe
            = static_cast<Sqlite3RowIDBackEnd *>(
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

        statement_.useData_[0][pos].isNull_ = false;
        statement_.useData_[0][pos].data_ = buf_;
    }
}

void Sqlite3StandardUseTypeBackEnd::postUse(
    bool /* gotData */, eIndicator * /* ind */)
{
    // TODO: if sqlite3 allows to *get* data via this channel,
    // write it back to client buffers (variable)

    // clean up the working buffer, it might be allocated anew in
    // the next run of preUse
    cleanUp();
}

void Sqlite3StandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}

void Sqlite3VectorUseTypeBackEnd::bindByPos(int & position,
                                            void * data, 
                                            eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void Sqlite3VectorUseTypeBackEnd::bindByName(std::string const & name, 
                                             void * data,
                                             eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = ":" + name;

    statement_.resetIfNeeded();
    position_ = sqlite3_bind_parameter_index(statement_.stmt_, name_.c_str());

    if (0 == position_)
    {
        std::ostringstream ss;
        ss << "Cannot bind (by name) to " << name_;
        throw SOCIError(ss.str());
    }
}

void Sqlite3VectorUseTypeBackEnd::preUse(eIndicator const * ind)
{
    std::size_t const vsize = size();

    // make sure that useData can hold enough rows
    if (statement_.useData_.size() != vsize)
        statement_.useData_.resize(vsize);

    int pos = position_ - 1;

    for (size_t i = 0; i != vsize; ++i)
    {
        char *buf = 0;

        // make sure that each row can accomodate the number of columns
        if (statement_.useData_[i].size() <
            static_cast<std::size_t>(position_))
            statement_.useData_[i].resize(position_);

        // the data in vector can be either eOK or eNull
        if (ind != NULL && ind[i] == eNull)
        {
            statement_.useData_[i][pos].isNull_ = true;
            statement_.useData_[i][pos].data_ = "";
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

            statement_.useData_[i][pos].isNull_ = false;
            statement_.useData_[i][pos].data_ = buf;
        }
        if (buf)
            delete[] buf;
    }
}

std::size_t Sqlite3VectorUseTypeBackEnd::size()
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

void Sqlite3VectorUseTypeBackEnd::cleanUp()
{
    // ...
}

Sqlite3RowIDBackEnd::Sqlite3RowIDBackEnd(
    Sqlite3SessionBackEnd & /* session */)
{
    // ...
}

Sqlite3RowIDBackEnd::~Sqlite3RowIDBackEnd()
{
    // ...
}

Sqlite3BLOBBackEnd::Sqlite3BLOBBackEnd(Sqlite3SessionBackEnd &session)
    : session_(session), tableName_(""), columnName_(""), buf_(0), len_(0)
{
#ifndef SQLITE_ENABLE_COLUMN_METADATA
    throw SOCIError("Blob not currently supported in this Sqlite3 backend. "
        "Compile both Sqlite3 and SOCI using -DSQLITE_ENABLE_COLUMN_METADATA"
    );
#endif
}

Sqlite3BLOBBackEnd::~Sqlite3BLOBBackEnd()
{
    if (buf_)
    {
        delete [] buf_;
        buf_ = 0;
        len_ = 0;
    }
}

void Sqlite3BLOBBackEnd::setData(
    const char *tableName, const char *columnName,
    const char *buf, size_t len)
{
    tableName_ = tableName;
    columnName_ = columnName;
    len_ = len;

    if (buf_)
    {
        delete [] buf_;
        buf_ = 0;
    }
    
    if (len_ > 0)
    {
        buf_ = new char[len_];
        memcpy(buf_, buf, len_);
    }
}

std::size_t Sqlite3BLOBBackEnd::getLen()
{
    return len_;
}

void Sqlite3BLOBBackEnd::updateBLOB()
{
    sqlite3_stmt *stmt = 0;

    std::ostringstream ss;
    ss << "update " << tableName_ 
       << " set " << columnName_ 
       << " = :blob";

    const char *tail; // unused;
    int res = sqlite3_prepare(session_.conn_, 
                              ss.str().c_str(), 
                              ss.str().size(), 
                              &stmt, 
                              &tail);

    if (res != SQLITE_OK)
    {
        sqlite3_finalize(stmt);

        const char *zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss2;
        ss2 << "updateBLOB: " << zErrMsg;        
        throw SOCIError(ss2.str());
    }

    res = sqlite3_bind_blob(stmt, 1, (void*)buf_, len_, 
                            SQLITE_STATIC);
    
    if (SQLITE_OK != res)
    {
        sqlite3_finalize(stmt);    
        throw SOCIError("Failure to bind BLOB");
    }

    res = sqlite3_step(stmt);

    if (SQLITE_DONE != res)
    {
        sqlite3_finalize(stmt);
        throw SOCIError("Failure to update BLOB");
    }

    sqlite3_finalize(stmt);
}

std::size_t Sqlite3BLOBBackEnd::read(
    std::size_t offset, char * buf, std::size_t toRead)
{
    size_t r = toRead;

    // make sure that we don't try to read
    // past the end of the data
    if (r > len_ - offset)
        r = len_ - offset;

    memcpy(buf, buf_ + offset, r);

    return r;   
}

std::size_t Sqlite3BLOBBackEnd::write(
    std::size_t offset, char const * buf,
    std::size_t toWrite)
{
    const char* oldBuf = buf_;
    std::size_t oldLen = len_;
    len_ = std::max(len_, offset + toWrite);

    buf_ = new char[len_];

    // we need to copy both old and new buffers
    // it is possible that the new does not 
    // completely cover the old
    memcpy(buf_, oldBuf, oldLen);
    memcpy(buf_ + offset, buf, len_);
    
    delete [] oldBuf;

    updateBLOB();

    return len_;
}

std::size_t Sqlite3BLOBBackEnd::append(
    char const * buf, std::size_t toWrite)
{
    const char* oldBuf = buf_;

    buf_ = new char[len_ + toWrite];

    memcpy(buf_, oldBuf, len_);

    memcpy(buf_ + len_, buf, toWrite);

    delete [] oldBuf;

    len_ += toWrite;

    updateBLOB();

    return len_;
}

void Sqlite3BLOBBackEnd::trim(std::size_t newLen)
{
    // ...
}
// concrete factory for Sqlite3 concrete strategies
struct Sqlite3BackEndFactory : BackEndFactory
{
    virtual ~Sqlite3BackEndFactory() {}

    virtual Sqlite3SessionBackEnd * makeSession(
        std::string const &connectString) const
    {
        return new Sqlite3SessionBackEnd(connectString);
    }

} sqlite3BEF;

namespace // anonymouse
{

// global object for automatic factory registration
struct Sqlite3AutoRegister
{
    Sqlite3AutoRegister()
    {
        theBEFRegistry().registerMe("sqlite3", &sqlite3BEF);
    }
} sqlite3AutoRegister;

} // namespace anonymous
