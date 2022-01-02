//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, Rafal Bobrowski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_FIREBIRD_SOURCE
#include "soci/firebird/soci-firebird.h"
#include "firebird/error-firebird.h"

using namespace soci;
using namespace soci::details::firebird;

firebird_blob_backend::firebird_blob_backend(firebird_session_backend &session)
	  : session_(session), bid_(), bhp_(0), max_seg_size_(0)
{ 
    bid_.gds_quad_high = 0; bid_.gds_quad_low = 0;
}

firebird_blob_backend::~firebird_blob_backend()
{
    close();
}

void firebird_blob_backend::open()
{
    if (bhp_ != 0)
    {
        // BLOB already opened
        return;
    }

    ISC_STATUS stat[20];

    if ((bid_.gds_quad_high == 0) && (bid_.gds_quad_low == 0))
    {
        if (bhp_ != 0)
        {
            if (isc_close_blob(stat, &bhp_))
            {
                throw_iscerror(stat);
            }
            bhp_ = 0;
        }
        
        // create new blob
        if (isc_create_blob(stat, &session_.dbhp_, session_.current_transaction(),
                            &bhp_, &bid_))
        {
            bhp_ = 0L;
            throw_iscerror(stat);
        }
        return;
    }

    // Or open an existent one
    if (isc_open_blob2(stat, &session_.dbhp_, session_.current_transaction(),
                       &bhp_, &bid_, 0, NULL))
    {
        bhp_ = 0L;
        throw_iscerror(stat);
    }
}

void firebird_blob_backend::close()
{
    if (bhp_ == 0)
    {
        return;
    }

    // close blob
    ISC_STATUS stat[20];
    if (isc_close_blob(stat, &bhp_))
    {
        throw_iscerror(stat);
    }
    bhp_ = 0;
    max_seg_size_ = 0;
}

void firebird_blob_backend::read(blob& b) 
{
    b.resize(this->get_len());
    this->read_from_start(&b[0], b.size());
}

void firebird_blob_backend::write(blob& b) 
{
    this->write_from_start(&b[0], b.size());
}

void firebird_blob_backend::assign(ISC_QUAD const & bid)
{
    close();

    bid_ = bid;
}

void firebird_blob_backend::assign(std::string data)
{
    char *tmp = new char[data.size()];
    
    char * itr = tmp;
    char * end_itr = tmp + static_cast<int>(data.size());
    int idx = 0;

    while (itr!=end_itr)
    {
        *itr++ = data[idx++];
    }
    this->assign(*reinterpret_cast<ISC_QUAD*>(tmp));
    delete[] tmp;
}

void firebird_blob_backend::assign(details::holder* h)
{
    this->assign(h->get<std::string>());
}

std::size_t firebird_blob_backend::get_len()
{
    this->open();
    std::size_t ret = static_cast<std::size_t>(this->getBLOBInfo());
    this->close();
    return ret;
}

std::size_t firebird_blob_backend::read(
    std::size_t /*offset*/, char * buf, std::size_t toRead)
{
    this->open();
    this->getBLOBInfo();

    // The blob is empty.
    if (toRead == 0)
    {
        return toRead;
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
                        buf  + total_bytes);

        total_bytes += bytes;

        if (total_bytes == toRead)
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
            throw_iscerror(stat);
        }
    }
    while (keep_reading);

    this->close();
    return total_bytes;
}

std::size_t firebird_blob_backend::write(
    std::size_t offset, char const * buf, std::size_t toWrite)
{
    this->open();
    this->getBLOBInfo();

    ISC_STATUS stat[20];

    if (toWrite > 0)
    {
        // Segment Size : Specifying the BLOB segment is throwback to times past, when applications for working
        // with BLOB data were written in C(Embedded SQL) with the help of the gpre pre - compiler.
        // Nowadays, it is effectively irrelevant.The segment size for BLOB data is determined by the client side and is usually larger than the data page size,
        // in any case.
        do
        {
            unsigned short segmentSize = 0xFFFF; //last unsigned short number
            if (toWrite - offset < segmentSize) //if content size is less than max segment size or last data segment is about to be written
                segmentSize = static_cast<unsigned short>(toWrite - offset);
            //write segment
            if (isc_put_segment(stat, &bhp_, segmentSize, buf + offset))
            {
                throw_iscerror(stat);
            }
            offset += segmentSize;
        }
        while (offset < toWrite);
    }

    this->close();
    return toWrite;
}

std::size_t firebird_blob_backend::append(
    char const * buf, std::size_t toWrite)
{
    this->open();
    std::size_t cur_size = static_cast<std::size_t>(this->getBLOBInfo());
    std::vector<char> tmp(cur_size + toWrite);

    this->read_from_start(&tmp[0], cur_size);

    memcpy(&tmp[cur_size], buf, toWrite);

    this->write_from_start(&tmp[0], tmp.size());

    this->close();
    return tmp.size();
}

void firebird_blob_backend::trim(std::size_t newLen)
{
    this->open();
    std::size_t cur_size = static_cast<std::size_t>(this->getBLOBInfo());

    if (cur_size < newLen)
    {
        throw soci_error("The trimmed size is bigger and the current blob size.");
    }

    std::vector<char> tmp(cur_size);
    this->read_from_start(&tmp[0], cur_size);
    tmp.resize(newLen);
    this->write_from_start(&tmp[0], tmp.size());

    this->close();
}

// retrives number of segments and total length of BLOB
// returns total length of BLOB
long firebird_blob_backend::getBLOBInfo()
{
    char blob_items[] = {isc_info_blob_max_segment, isc_info_blob_total_length};
    char res_buffer[20], *p, item;
    short length;
    long total_length = 0;

    ISC_STATUS stat[20];

    if (isc_blob_info(stat, &bhp_, sizeof(blob_items), blob_items,
                      sizeof(res_buffer), res_buffer))
    {
        throw_iscerror(stat);
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
                throw soci_error("Fatal Error: BLOB info truncated!");
                break;
            default:
                break;
        }
        p += length;
    }

    return total_length;
}
