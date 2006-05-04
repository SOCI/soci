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
using namespace sqlite_api;

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
    const char* oldBuf = buf_;
    len_ = newLen;

    buf_ = new char[len_];

    memcpy(buf_, oldBuf, len_);

    delete [] oldBuf;

    updateBLOB();
}
