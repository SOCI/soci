//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci.h"
#include "soci-mysql.h"

#include <cerrno>

//#include <iostream>
//using std::cerr;
//using std::cout;
//using std::endl;

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace SOCI;
using namespace SOCI::details;
using std::string;


namespace { // anonymous

void skipWhite(string::const_iterator *i,
    string::const_iterator const & end, bool endok)
{
    for (;;)
    {
        if (*i == end)
        {
            if (endok)
            {
                return;
            }
            else
            {
                throw SOCIError("Unexpected end of connection string.");
            }
        }
        if (std::isspace(**i))
        {
            ++*i;
        }
        else
        {
            return;
        }
    }
}

string paramName(string::const_iterator *i,
    string::const_iterator const & end)
{
    string val("");
    for (;;)
    {
        if (*i == end or (!std::isalpha(**i) and **i != '_'))
        {
            break;
        }
        val += **i;
        ++*i;
    }
    return val;
}

string paramValue(string::const_iterator *i,
    string::const_iterator const & end)
{
    string err = "Malformed connection string.";
    bool quot;
    if (**i == '\'')
    {
        quot = true;
        ++*i;
    }
    else
    {
        quot = false;
    }
    string val("");
    for (;;)
    {
        if (*i == end)
        {
            if (quot)
            {
                throw SOCIError(err);
            }
            else
            {
                break;
            }
        }
        if (**i == '\'')
        {
            if (quot)
            {
                ++*i;
                break;
            }
            else
            {
                throw SOCIError(err);
            }
        }
        if (!quot and std::isspace(**i))
        {
            break;
        }
        if (**i == '\\')
        {
            ++*i;
            if (*i == end)
            {
                throw SOCIError(err);
            }
        }
        val += **i;
        ++*i;
    }
    return val;
}

bool validInt(const string & s)
{
    char *tail;
    const char *cstr = s.c_str();
    errno = 0;
    long l = std::strtol(cstr, &tail, 10);
    if (errno != 0 or l > INT_MAX or l < INT_MIN)
    {
        return false;
    }
    if (*tail != '\0')
    {
        return false;
    }
    return true;
}

void parseConnectString(const string & connectString,
    string *host, bool *host_p,
    string *user, bool *user_p,
    string *password, bool *password_p,
    string *db, bool *db_p,
    string *unix_socket, bool *unix_socket_p,
    int *port, bool *port_p)
{
    *host_p = false;
    *user_p = false;
    *password_p = false;
    *db_p = false;
    *unix_socket_p = false;
    *port_p = false;
    string err = "Malformed connection string.";
    string::const_iterator i = connectString.begin(),
        end = connectString.end();
    while (i != end)
    {
        skipWhite(&i, end, true);
        if (i == end)
        {
            return;
        }
        string par = paramName(&i, end);
        skipWhite(&i, end, false);
        if (*i == '=')
        {
            ++i;
        }
        else
        {
            throw SOCIError(err);
        }
        skipWhite(&i, end, false);
        string val = paramValue(&i, end);
        if (par == "port" and !*port_p)
        {
            if (!validInt(val))
            {
                throw SOCIError(err);
            }
            *port = std::atoi(val.c_str());
            if (port < 0)
            {
                throw SOCIError(err);
            }
            *port_p = true;
        }
        else if (par == "host" and !*host_p)
        {
            *host = val;
            *host_p = true;
        }
        else if (par == "user" and !*user_p)
        {
            *user = val;
            *user_p = true;
        }
        else if ((par == "pass" or par == "password") and !*password_p)
        {
            *password = val;
            *password_p = true;
        }
        else if ((par == "db" or par == "dbname") and !*db_p)
        {
            *db = val;
            *db_p = true;
        }
        else if (par == "unix_socket" && !*unix_socket_p)
        {
            *unix_socket = val;
            *unix_socket_p = true;
        }
        else
        {
            throw SOCIError(err);
        }
    }
}

} // namespace anonymous

MySQLSessionBackEnd::MySQLSessionBackEnd(
    std::string const & connectString)
{
    string host, user, password, db, unix_socket;
    int port;
    bool host_p, user_p, password_p, db_p, unix_socket_p, port_p;
    parseConnectString(connectString, &host, &host_p, &user, &user_p,
        &password, &password_p, &db, &db_p,
        &unix_socket, &unix_socket_p, &port, &port_p);
    conn_ = mysql_init(NULL);
    if (conn_ == NULL)
    {
        throw SOCIError("mysql_init() failed.");
    }
    if (!mysql_real_connect(conn_,
            host_p ? host.c_str() : NULL,
            user_p ? user.c_str() : NULL,
            password_p ? password.c_str() : NULL,
            db_p ? db.c_str() : NULL,
            port_p ? port : 0,
            unix_socket_p ? unix_socket.c_str() : NULL,
            0)) {
        string err = mysql_error(conn_);
        cleanUp();
        throw SOCIError(err);
    }
}

MySQLSessionBackEnd::~MySQLSessionBackEnd()
{
    cleanUp();
}

namespace { // anonymous

// helper function for hardcoded queries
void hardExec(MYSQL *conn, const string & query)
{
    //cerr << query << endl;
    if (0 != mysql_real_query(conn, query.c_str(), query.size()))
    {
        throw SOCIError(mysql_error(conn));
    }
}

}  // namespace anonymous

void MySQLSessionBackEnd::begin()
{
    hardExec(conn_, "BEGIN");
}

void MySQLSessionBackEnd::commit()
{
    hardExec(conn_, "COMMIT");
}

void MySQLSessionBackEnd::rollback()
{
    hardExec(conn_, "ROLLBACK");
}

void MySQLSessionBackEnd::cleanUp()
{
    if (conn_ != NULL)
    {
        mysql_close(conn_);
        conn_ = NULL;
    }
}

MySQLStatementBackEnd * MySQLSessionBackEnd::makeStatementBackEnd()
{
    return new MySQLStatementBackEnd(*this);
}

MySQLRowIDBackEnd * MySQLSessionBackEnd::makeRowIDBackEnd()
{
    return new MySQLRowIDBackEnd(*this);
}

MySQLBLOBBackEnd * MySQLSessionBackEnd::makeBLOBBackEnd()
{
    return new MySQLBLOBBackEnd(*this);
}

MySQLStatementBackEnd::MySQLStatementBackEnd(MySQLSessionBackEnd &session)
    : session_(session), result_(NULL), justDescribed_(false)
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
        std::string query;
        if (!useByPosBuffers_.empty() || !useByNameBuffers_.empty())
        {
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
                if (number > 1)
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
                            "any data but it did.");
                    }
                    query.clear();
                }
            }
            if (number > 1)
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
    return new MySQLStandardIntoTypeBackEnd(*this);
}

MySQLStandardUseTypeBackEnd * MySQLStatementBackEnd::makeUseTypeBackEnd()
{
    return new MySQLStandardUseTypeBackEnd(*this);
}

MySQLVectorIntoTypeBackEnd *
MySQLStatementBackEnd::makeVectorIntoTypeBackEnd()
{
    return new MySQLVectorIntoTypeBackEnd(*this);
}

MySQLVectorUseTypeBackEnd * MySQLStatementBackEnd::makeVectorUseTypeBackEnd()
{
    return new MySQLVectorUseTypeBackEnd(*this);
}

void MySQLStandardIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void MySQLStandardIntoTypeBackEnd::preFetch()
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

void MySQLStandardIntoTypeBackEnd::postFetch(
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
        int pos = position_ - 1;
        mysql_data_seek(statement_.result_, statement_.currentRow_);
        MYSQL_ROW row = mysql_fetch_row(statement_.result_);
        if (row[pos] == NULL)
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
        const char *buf = row[pos] != NULL ? row[pos] : "";
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

void MySQLStandardIntoTypeBackEnd::cleanUp()
{
    // nothing to do here
}

void MySQLVectorIntoTypeBackEnd::defineByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void MySQLVectorIntoTypeBackEnd::preFetch()
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

void MySQLVectorIntoTypeBackEnd::postFetch(bool gotData, eIndicator *ind)
{
    if (gotData)
    {
        // Here, rowsToConsume_ in the Statement object designates
        // the number of rows that need to be put in the user's buffers.

        // PostgreSQL column positions start at 0
        int pos = position_ - 1;

        int const endRow = statement_.currentRow_ + statement_.rowsToConsume_;
	
        mysql_data_seek(statement_.result_, statement_.currentRow_);
        for (int curRow = statement_.currentRow_, i = 0;
             curRow != endRow; ++curRow, ++i)
        {
            MYSQL_ROW row = mysql_fetch_row(statement_.result_);
            // first, deal with indicators
            if (row[pos] == NULL)
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
            const char *buf = row[pos] != NULL ? row[pos] : "";

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

void MySQLVectorIntoTypeBackEnd::resize(std::size_t sz)
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

std::size_t MySQLVectorIntoTypeBackEnd::size()
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

void MySQLVectorIntoTypeBackEnd::cleanUp()
{
    // nothing to do here
}

void MySQLStandardUseTypeBackEnd::bindByPos(
    int &position, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void MySQLStandardUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

namespace { // anonymous

char * quote(MYSQL * conn, const char *s, int l)
{
    char *retv = new char[2 * l + 3];
    retv[0] = '\'';
    int le = mysql_real_escape_string(conn, retv + 1, s, l);
    retv[le + 1] = '\'';
    retv[le + 2] = '\0';

    return retv;
}

} // namespace anonymous

void MySQLStandardUseTypeBackEnd::preUse(eIndicator const *ind)
{
    if (ind != NULL && *ind == eNull)
    {
        buf_ = new char[5];
        strcpy(buf_, "NULL");
    }
    else
    {
        // allocate and fill the buffer with text-formatted client data
        switch (type_)
        {
        case eXChar:
            {
                char buf[] = { *static_cast<char*>(data_), '\0' };
                buf_ = quote(statement_.session_.conn_, buf, 1);
            }
            break;
        case eXCString:
            {
                CStringDescriptor *strDescr
                    = static_cast<CStringDescriptor *>(data_);
                buf_ = quote(statement_.session_.conn_, strDescr->str_,
			     std::strlen(strDescr->str_));
            }
            break;
        case eXStdString:
            {
                std::string *s = static_cast<std::string *>(data_);
                buf_ = quote(statement_.session_.conn_,
			     s->c_str(), s->size());
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
                std::size_t const bufSize = 22;
                buf_ = new char[bufSize];

                std::tm *t = static_cast<std::tm *>(data_);
                std::snprintf(buf_, bufSize,
                    "\'%d-%02d-%02d %02d:%02d:%02d\'",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec);
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

void MySQLStandardUseTypeBackEnd::postUse(bool gotData, eIndicator *ind)
{
    cleanUp();
}

void MySQLStandardUseTypeBackEnd::cleanUp()
{
    if (buf_ != NULL)
    {
        delete [] buf_;
        buf_ = NULL;
    }
}

void MySQLVectorUseTypeBackEnd::bindByPos(int &position, void *data,
    eExchangeType type)
{
    data_ = data;
    type_ = type;
    position_ = position++;
}

void MySQLVectorUseTypeBackEnd::bindByName(
    std::string const &name, void *data, eExchangeType type)
{
    data_ = data;
    type_ = type;
    name_ = name;
}

void MySQLVectorUseTypeBackEnd::preUse(eIndicator const *ind)
{
    std::size_t const vsize = size();
    for (size_t i = 0; i != vsize; ++i)
    {
        char *buf;

        // the data in vector can be either eOK or eNull
        if (ind != NULL && ind[i] == eNull)
        {
            buf = new char[5];
            strcpy(buf, "NULL");
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
		    
                    char tmp[] = { v[i], '\0' };
                    buf = quote(statement_.session_.conn_, tmp, 1);
                }
                break;
            case eXStdString:
                {
                    std::vector<std::string> *pv
                        = static_cast<std::vector<std::string> *>(data_);
                    std::vector<std::string> &v = *pv;
		    
                    buf = quote(statement_.session_.conn_,
                        v[i].c_str(), v[i].size());
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

                    std::size_t const bufSize = 22;
                    buf = new char[bufSize];

                    std::snprintf(
                        buf, bufSize, "\'%d-%02d-%02d %02d:%02d:%02d\'",
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

std::size_t MySQLVectorUseTypeBackEnd::size()
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

void MySQLVectorUseTypeBackEnd::cleanUp()
{
    std::size_t const bsize = buffers_.size();
    for (std::size_t i = 0; i != bsize; ++i)
    {
        delete [] buffers_[i];
    }
}

MySQLRowIDBackEnd::MySQLRowIDBackEnd(MySQLSessionBackEnd & /* session */)
{
    throw SOCIError("RowIDs are not supported.");
}

MySQLRowIDBackEnd::~MySQLRowIDBackEnd()
{
    throw SOCIError("RowIDs are not supported.");
}

MySQLBLOBBackEnd::MySQLBLOBBackEnd(MySQLSessionBackEnd &session)
    : session_(session)
{
    throw SOCIError("BLOBs are not supported.");
}

MySQLBLOBBackEnd::~MySQLBLOBBackEnd()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::getLen()
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

std::size_t MySQLBLOBBackEnd::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    throw SOCIError("BLOBs are not supported.");
}

void MySQLBLOBBackEnd::trim(std::size_t /* newLen */)
{
    throw SOCIError("BLOBs are not supported.");
}


// concrete factory for MySQL concrete strategies
struct MySQLBackEndFactory : BackEndFactory
{
    virtual MySQLSessionBackEnd * makeSession(
        std::string const &connectString) const
    {
        return new MySQLSessionBackEnd(connectString);
    }

} mysqlBEF;

namespace
{

// global object for automatic factory registration
    struct MySQLAutoRegister
    {
        MySQLAutoRegister()
        {
            theBEFRegistry().registerMe("mysql", &mysqlBEF);
        }
    } mysqlAutoRegister;

} // namespace anonymous
