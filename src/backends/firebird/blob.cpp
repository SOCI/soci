//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci-firebird.h"
#include "error.h"

using namespace SOCI;
using namespace SOCI::details::Firebird;

FirebirdBLOBBackEnd::FirebirdBLOBBackEnd(FirebirdSessionBackEnd &session)
        : session_(session), from_db_(false), bhp_(0), loaded_(false),
        max_seg_size_(0)
{}

FirebirdBLOBBackEnd::~FirebirdBLOBBackEnd()
{
    cleanUp();
}

std::size_t FirebirdBLOBBackEnd::getLen()
{
    if (from_db_ && bhp_ == 0)
    {
        open();
    }

    return data_.size();
}

std::size_t FirebirdBLOBBackEnd::read(
    std::size_t offset, char * buf, std::size_t toRead)
{
    if (from_db_ && !loaded_)
    {
        // this is blob fetched from database, but not loaded yet
        load();
    }

    std::size_t size = data_.size();

    if (offset > size)
    {
        throw SOCIError("Can't read past-the-end of BLOB data");
    }

    char * itr = buf;
    std::size_t limit = size - offset < toRead ? size - offset : toRead;
    std::size_t index = 0;

    while (index < limit)
    {
        *itr = data_[offset+index];
        ++index;
        ++itr;
    }

    return limit;
}

std::size_t FirebirdBLOBBackEnd::write(std::size_t offset, char const * buf,
                                       std::size_t toWrite)
{
    if (from_db_ && !loaded_)
    {
        // this is blob fetched from database, but not loaded yet
        load();
    }

    std::size_t size = data_.size();

    if (offset > size)
    {
        throw SOCIError("Can't write past-the-end of BLOB data");
    }

    // make sure there is enough space in buffer
    if (toWrite > (size - offset))
    {
        data_.resize(size + (toWrite - (size - offset)));
    }

    writeBuffer(offset, buf, toWrite);

    return toWrite;
}

std::size_t FirebirdBLOBBackEnd::append(
    char const * buf, std::size_t toWrite)
{
    if (from_db_ && !loaded_)
    {
        // this is blob fetched from database, but not loaded yet
        load();
    }

    std::size_t size = data_.size();
    data_.resize(size + toWrite);

    writeBuffer(size, buf, toWrite);

    return toWrite;
}

void FirebirdBLOBBackEnd::trim(std::size_t newLen)
{
    if (from_db_ && !loaded_)
    {
        // this is blob fetched from database, but not loaded yet
        load();
    }

    data_.resize(newLen);
}

void FirebirdBLOBBackEnd::writeBuffer(std::size_t offset,
                                      char const * buf, std::size_t toWrite)
{
    char const * itr = buf;
    char const * end_itr = buf + toWrite;

    while (itr!=end_itr)
    {
        data_[offset++] = *itr++;
    }
}

void FirebirdBLOBBackEnd::open()
{
    if (bhp_ != 0)
    {
        // BLOB already opened
        return;
    }

    ISC_STATUS stat[20];

    if (isc_open_blob2(stat, &session_.dbhp_, &session_.trhp_, &bhp_,
                       &bid_, 0, NULL))
    {
        bhp_ = 0L;
        throwISCError(stat);
    }

    // get basic blob info
    long blob_size = getBLOBInfo();

    data_.resize(blob_size);
}

void FirebirdBLOBBackEnd::cleanUp()
{
    from_db_ = false;
    loaded_ = false;
    max_seg_size_ = 0;
    data_.resize(0);

    if (bhp_ != 0)
    {
        // close blob
        ISC_STATUS stat[20];
        if (isc_close_blob(stat, &bhp_))
        {
            throwISCError(stat);
        }
        bhp_ = 0;
    }
}

// loads blob data into internal buffer
void FirebirdBLOBBackEnd::load()
{
    if (bhp_ == 0)
    {
        open();
    }

    ISC_STATUS stat[20];
    unsigned short bytes;
    std::vector<char>::size_type total_bytes = 0;
    bool keep_reading = false;

    do
    {
        bytes = 0;
        // next segment of data
        // data_ is large-enough because we know total size of blob
        isc_get_segment(stat, &bhp_, &bytes, static_cast<short>(max_seg_size_),
                        &data_[total_bytes]);

        total_bytes += bytes;

        if (total_bytes == data_.size())
        {
            // we have all BLOB data
            keep_reading = false;
        }
        else if (stat[1] == 0 || stat[1] == isc_segment)
        {
            // there is more data to read from current segment (0)
            // or there is next segment (isc_segment)
            keep_reading = true;
        }
        else if (stat[1] == isc_segstr_eof)
        {
            // BLOB is shorter then we expected ???
            keep_reading = false;
        }
        else
        {
            // an error has occured
            throwISCError(stat);
        }
    }
    while (keep_reading);

    loaded_ = true;
}

// this method saves BLOB content to database
// (a new BLOB will be created at this point)
// BLOB will be closed after save.
void FirebirdBLOBBackEnd::save()
{
    // close old blob if necessary
    ISC_STATUS stat[20];
    if (bhp_ != 0)
    {
        if (isc_close_blob(stat, &bhp_))
        {
            throwISCError(stat);
        }
        bhp_ = 0;
    }

    // create new blob
    if (isc_create_blob(stat, &session_.dbhp_, &session_.trhp_,
                        &bhp_, &bid_))
    {
        throwISCError(stat);
    }

    if (data_.size() > 0)
    {
        // write data
        if (isc_put_segment(stat, &bhp_,
                            static_cast<unsigned short>(data_.size()), &data_[0]))
        {
            throwISCError(stat);
        }
    }

    cleanUp();
    from_db_ = true;
}

// retrives number of segments and total length of BLOB
// returns total length of BLOB
long FirebirdBLOBBackEnd::getBLOBInfo()
{
    char blob_items[] = {isc_info_blob_max_segment, isc_info_blob_total_length};
    char res_buffer[20], *p, item;
    short length;
    long total_length = 0;

    ISC_STATUS stat[20];

    if (isc_blob_info(stat, &bhp_, sizeof(blob_items), blob_items,
                      sizeof(res_buffer), res_buffer))
    {
        throwISCError(stat);
    }

    for (p = res_buffer; *p != isc_info_end ;)
    {
        item = *p++;
        length = static_cast<short>(isc_vax_integer(p, 2));
        p += 2;
        switch (item)
        {
            case isc_info_blob_max_segment:
                max_seg_size_ = isc_vax_integer(p, length);
                break;
            case isc_info_blob_total_length:
                total_length = isc_vax_integer(p, length);
                break;
            case isc_info_truncated:
                throw SOCIError("Fatal Error: BLOB info truncated!");
                break;
            default:
                break;
        }
        p += length;
    }

    return total_length;
}
