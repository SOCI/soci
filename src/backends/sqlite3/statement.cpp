//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SQLITE3_SOURCE
#include "soci/sqlite3/soci-sqlite3.h"
// std
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

// This is used instead of tolower() just to avoid warnings about int to char
// casts inside MSVS std::transform() implementation.
char toLowerCh(char c) {
    return static_cast<char>( std::tolower(c) );
}

using namespace soci;
using namespace soci::details;
using namespace sqlite_api;

sqlite3_statement_backend::sqlite3_statement_backend(
    sqlite3_session_backend &session)
    : session_(session)
    , stmt_(0)
    , dataCache_()
    , useData_(0)
    , databaseReady_(false)
    , boundByName_(false)
    , boundByPos_(false)
    , hasVectorIntoElements_(false)
    , rowsAffectedBulk_(-1LL)
{
}

void sqlite3_statement_backend::alloc()
{
    // ...
}

void sqlite3_statement_backend::clean_up()
{
    rowsAffectedBulk_ = -1LL;

    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
        databaseReady_ = false;
    }
}

void sqlite3_statement_backend::prepare(std::string const & query,
    statement_type /* eType */)
{
    clean_up();

    char const* tail = 0; // unused;
    int const res = sqlite3_prepare_v2(session_.conn_,
                              query.c_str(),
                              static_cast<int>(query.size()),
                              &stmt_,
                              &tail);
    if (res != SQLITE_OK)
    {
        char const* zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "sqlite3_statement_backend::prepare: "
           << zErrMsg;
        throw sqlite3_soci_error(ss.str(), res);
    }
    databaseReady_ = true;
}

// sqlite3_reset needs to be called before a prepared statment can
// be executed a second time.
void sqlite3_statement_backend::reset_if_needed()
{
    if (stmt_ && databaseReady_ == false)
    {
        reset();
    }
}

void sqlite3_statement_backend::reset()
{
    int const res = sqlite3_reset(stmt_);
    if (SQLITE_OK == res)
    {
        databaseReady_ = true;
    }
}

// This is used by bulk operations
statement_backend::exec_fetch_result
sqlite3_statement_backend::load_rowset(int totalRows)
{
    statement_backend::exec_fetch_result retVal = ef_success;

    int i = 0;
    int numCols = 0;

    // just a hack because in some case, describe() is not called, so columns_ is empty
    if (columns_.empty())
    {
        numCols = sqlite3_column_count(stmt_);
        db_type dbtype;
        std::string name;
        for (int c = 1; c <= numCols; ++c)
            describe_column(c, dbtype, name);
    }
    else
        numCols = static_cast<int>(columns_.size());


    if (!databaseReady_)
    {
        retVal = ef_no_data;
    }
    else
    {
        // make the vector big enough to hold the data we need
        dataCache_.resize(totalRows);
        for (sqlite3_recordset::iterator it = dataCache_.begin(),
            end = dataCache_.end(); it != end; ++it)
        {
            (*it).resize(numCols);
        }

        for (i = 0; i < totalRows && databaseReady_; ++i)
        {
            int const res = sqlite3_step(stmt_);

            if (SQLITE_DONE == res)
            {
                databaseReady_ = false;
                retVal = ef_no_data;
                break;
            }
            else if (SQLITE_ROW == res)
            {
                for (int c = 0; c < numCols; ++c)
                {
                    const sqlite3_column_info &coldef = columns_[c];
                    sqlite3_column &col = dataCache_[i][c];

                    if (sqlite3_column_type(stmt_, c) == SQLITE_NULL)
                    {
                        col.isNull_ = true;
                        continue;
                    }

                    col.isNull_ = false;
                    col.type_ = coldef.type_;
                    col.dataType_ = coldef.dataType_;

                    switch (coldef.dataType_)
                    {
                        case db_string:
                        case db_date:
                            col.buffer_.size_ = sqlite3_column_bytes(stmt_, c);
                            col.buffer_.data_ = new char[col.buffer_.size_+1];
                            memcpy(col.buffer_.data_, sqlite3_column_text(stmt_, c), col.buffer_.size_+1);
                            break;

                        case db_double:
                            col.double_ = sqlite3_column_double(stmt_, c);
                            break;

                        case db_int8:
                            col.int8_ = static_cast<int8_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_uint8:
                            col.uint8_ = static_cast<uint8_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_int16:
                            col.int16_ = static_cast<int16_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_uint16:
                            col.uint16_ = static_cast<uint16_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_int32:
                            col.int32_ = static_cast<int32_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_uint32:
                            col.uint32_ = static_cast<uint32_t>(sqlite3_column_int(stmt_, c));
                            break;
                        case db_int64:
                            col.int64_ = sqlite3_column_int64(stmt_, c);
                            break;
                        case db_uint64:
                            col.uint64_ = static_cast<sqlite_api::sqlite3_uint64>(sqlite3_column_int64(stmt_, c));
                            break;

                        case db_blob:
                            col.buffer_.size_ = sqlite3_column_bytes(stmt_, c);
                            col.buffer_.data_ = (col.buffer_.size_ > 0 ? new char[col.buffer_.size_] : NULL);
                            memcpy(col.buffer_.data_, sqlite3_column_blob(stmt_, c), col.buffer_.size_);
                            break;

                        case db_xml:
                            throw soci_error("XML data type is not supported");
                    }
                }
            }
            else
            {
                char const* zErrMsg = sqlite3_errmsg(session_.conn_);
                std::ostringstream ss;
                ss << "sqlite3_statement_backend::loadRS: "
                   << zErrMsg;
                throw sqlite3_soci_error(ss.str(), res);
            }
        }
    }
    // if we read less than requested then shrink the vector
    dataCache_.resize(i);

    return retVal;
}

// This is used for non-bulk operations
statement_backend::exec_fetch_result
sqlite3_statement_backend::load_one()
{
    if( !databaseReady_ )
        return ef_no_data;

    statement_backend::exec_fetch_result retVal = ef_success;
    int const res = sqlite3_step(stmt_);

    if (SQLITE_DONE == res)
    {
        databaseReady_ = false;
        retVal = ef_no_data;
    }
    else if (SQLITE_ROW == res)
    {
    }
    else
    {
        char const* zErrMsg = sqlite3_errmsg(session_.conn_);

        std::ostringstream ss;
        ss << "sqlite3_statement_backend::loadOne: "
            << zErrMsg;
        throw sqlite3_soci_error(ss.str(), res);
    }
    return retVal;
}

// Execute statements once for every row of useData
statement_backend::exec_fetch_result
sqlite3_statement_backend::bind_and_execute(int number)
{
    statement_backend::exec_fetch_result retVal = ef_no_data;

    long long rowsAffectedBulkTemp = 0;

    rowsAffectedBulk_ = -1;

    int const rows = static_cast<int>(useData_.size());
    for (int row = 0; row < rows; ++row)
    {
        sqlite3_reset(stmt_);

        int const totalPositions = static_cast<int>(useData_[0].size());
        for (int pos = 1; pos <= totalPositions; ++pos)
        {
            int bindRes = SQLITE_OK;
            const sqlite3_column &col = useData_[row][pos-1];
            if (col.isNull_)
            {
                bindRes = sqlite3_bind_null(stmt_, pos);
            }
            else
            {
                switch (col.dataType_)
                {
                    case db_string:
                        bindRes = sqlite3_bind_text(stmt_, pos, col.buffer_.constData_, static_cast<int>(col.buffer_.size_), NULL);
                        break;

                    case db_date:
                        bindRes = sqlite3_bind_text(stmt_, pos, col.buffer_.constData_, static_cast<int>(col.buffer_.size_), SQLITE_TRANSIENT);
                        break;

                    case db_double:
                        bindRes = sqlite3_bind_double(stmt_, pos, col.double_);
                        break;

                    case db_int8:
                        bindRes = sqlite3_bind_int(stmt_, pos, static_cast<int>(col.int8_));
                        break;
                    case db_uint8:
                        bindRes = sqlite3_bind_int(stmt_, pos, static_cast<int>(col.uint8_));
                        break;
                    case db_int16:
                        bindRes = sqlite3_bind_int(stmt_, pos, static_cast<int>(col.int16_));
                        break;
                    case db_uint16:
                        bindRes = sqlite3_bind_int(stmt_, pos, static_cast<int>(col.uint16_));
                        break;
                    case db_int32:
                        bindRes = sqlite3_bind_int(stmt_, pos, static_cast<int>(col.int32_));
                        break;
                    case db_uint32:
                        bindRes = sqlite3_bind_int64(stmt_, pos, static_cast<sqlite_api::sqlite3_int64>(col.uint32_));
                        break;
                    case db_int64:
                        bindRes = sqlite3_bind_int64(stmt_, pos, col.int64_);
                        break;
                    case db_uint64:
                        bindRes = sqlite3_bind_int64(stmt_, pos, static_cast<sqlite_api::sqlite3_int64>(col.int64_));
                        break;

                    case db_blob:
                        bindRes = sqlite3_bind_blob(stmt_, pos, col.buffer_.constData_, static_cast<int>(col.buffer_.size_), NULL);
                        break;

                    case db_xml:
                        throw soci_error("XML data type is not supported");
                }
            }

            if (SQLITE_OK != bindRes)
            {
                // preserve the number of rows affected so far.
                rowsAffectedBulk_ = rowsAffectedBulkTemp;
                throw sqlite3_soci_error("Failure to bind on bulk operations", bindRes);
            }
        }

        // Handle the case where there are both into and use elements
        // in the same query and one of the into binds to a vector object.
        if (1 == rows && number != rows)
        {
            return load_rowset(number);
        }

        databaseReady_=true; // Mark sqlite engine is ready to perform sqlite3_step
        retVal = load_one(); // execute each bound line
        rowsAffectedBulkTemp += get_affected_rows();
    }

    rowsAffectedBulk_ = rowsAffectedBulkTemp;
    return retVal;
}

statement_backend::exec_fetch_result
sqlite3_statement_backend::execute(int number)
{
    if (stmt_ == NULL)
    {
        throw soci_error("No sqlite statement created");
    }

    sqlite3_reset(stmt_);
    databaseReady_ = true;

    statement_backend::exec_fetch_result retVal = ef_no_data;

    if (useData_.empty() == false)
    {
           retVal = bind_and_execute(number);
    }
    else
    {
        retVal = fetch(number);
    }

    return retVal;
}

statement_backend::exec_fetch_result
sqlite3_statement_backend::fetch(int number)
{
    if (hasVectorIntoElements_ || number == 0)
        return load_rowset(number);
    else
        return load_one();

}

long long sqlite3_statement_backend::get_affected_rows()
{
    if (rowsAffectedBulk_ >= 0)
    {
        return rowsAffectedBulk_;
    }
    return sqlite3_changes(session_.conn_);
}

int sqlite3_statement_backend::get_number_of_rows()
{
    return static_cast<int>(dataCache_.size());
}

std::string sqlite3_statement_backend::get_parameter_name(int index) const
{
    // Notice that SQLite host parameters are counted from 1, not 0.
    char const* name = sqlite3_bind_parameter_name(stmt_, index + 1);
    if (!name)
        return std::string();

    // SQLite returns parameters with the leading colon which is inconsistent
    // with the other backends, so get rid of it as well several other
    // characters which can be used for named parameters with SQLite.
    switch (*name)
    {
        case ':':
        case '?':
        case '@':
        case '$':
            name++;
            break;
    }

    return name;
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

typedef std::map<std::string, db_type> sqlite3_data_type_map;
static sqlite3_data_type_map get_data_type_map()
{
    sqlite3_data_type_map m;

    // Spaces are removed from decltype before looking up in this map, so we don't use them here as well

    // db_blob
    m["blob"]               = db_blob;

    // db_date
    m["date"]               = db_date;
    m["time"]               = db_date;
    m["datetime"]           = db_date;
    m["timestamp"]          = db_date;

    // db_double
    m["decimal"]            = db_double;
    m["double"]             = db_double;
    m["doubleprecision"]    = db_double;
    m["float"]              = db_double;
    m["number"]             = db_double;
    m["numeric"]            = db_double;
    m["real"]               = db_double;

    // integer types
    m["tinyint"]            = db_int8;

    m["smallint"]           = db_int16;

    m["boolean"]            = db_int32;
    m["int"]                = db_int32;
    m["integer"]            = db_int32;
    m["int2"]               = db_int32;
    m["mediumint"]          = db_int32;

    m["bigint"]             = db_int64;
    m["int8"]               = db_int64;

    m["unsignedbigint"]   = db_uint64;

    // db_string
    m["char"]               = db_string;
    m["character"]          = db_string;
    m["clob"]               = db_string;
    m["nativecharacter"]    = db_string;
    m["nchar"]              = db_string;
    m["nvarchar"]           = db_string;
    m["text"]               = db_string;
    m["varchar"]            = db_string;
    m["varyingcharacter"]   = db_string;

    return m;
}

void sqlite3_statement_backend::describe_column(int colNum,
                                                db_type & dbtype,
                                                std::string & columnName)
{
    static const sqlite3_data_type_map dataTypeMap = get_data_type_map();

    if (columns_.size() < (size_t)colNum)
        columns_.resize(colNum);
    sqlite3_column_info &coldef = columns_[colNum - 1];

    if (!coldef.name_.empty())
    {
        columnName = coldef.name_;
        dbtype = coldef.dataType_;
        return;
    }

    coldef.name_ = columnName = sqlite3_column_name(stmt_, colNum - 1);

    // This is a hack, but the sqlite3 type system does not
    // have a date or time field.  Also it does not reliably
    // id other data types.  It has a tendency to see everything
    // as text.  sqlite3_column_decltype returns the text that is
    // used in the create table statement
    char const* declType = sqlite3_column_decltype(stmt_, colNum-1);

    if ( declType == NULL )
    {
        static char const* s_char = "char";
        declType = s_char;
    }

    std::string dt = declType;

    // remove extra characters for example "(20)" in "varchar(20)" and all spaces
    dt.erase(std::remove_if(dt.begin(), dt.end(), [](char const c) { return std::isspace(c); }), dt.end());

    std::string::iterator siter = std::find_if(dt.begin(), dt.end(), [](char const c) { return !std::isalnum(c); });
    if (siter != dt.end())
        dt.resize(siter - dt.begin());

    // do all comparisons in lower case
    std::transform(dt.begin(), dt.end(), dt.begin(), toLowerCh);

    sqlite3_data_type_map::const_iterator iter = dataTypeMap.find(dt);
    if (iter != dataTypeMap.end())
    {
        coldef.dataType_ = dbtype = iter->second;
        coldef.type_ = to_data_type(dbtype);
        return;
    }

    // try to get it from the weak ass type system

    // total hack - execute the statment once to get the column types
    // then clear so it can be executed again
    sqlite3_step(stmt_);

    int const sqlite3_type = sqlite3_column_type(stmt_, colNum-1);
    switch (sqlite3_type)
    {
    case SQLITE_INTEGER:
        dbtype = db_int32;
        break;
    case SQLITE_FLOAT:
        dbtype = db_double;
        break;
    case SQLITE_BLOB:
    case SQLITE_TEXT:
        dbtype = db_string;
        break;
    default:
        dbtype = db_string;
        break;
    }
    coldef.dataType_ = dbtype;
    coldef.type_ = to_data_type(dbtype);

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
    hasVectorIntoElements_ = true;
    return new sqlite3_vector_into_type_backend(*this);
}

sqlite3_vector_use_type_backend *
sqlite3_statement_backend::make_vector_use_type_backend()
{
    return new sqlite3_vector_use_type_backend(*this);
}
