//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci-mysql.h"
#include <ciso646>
#include <cstring>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355 4702)
#endif

using namespace soci;
using namespace soci::details;

mysql_blob_backend::mysql_blob_backend(mysql_session_backend &session)
    : session_(session), buf_(0), len_(0)
{
}

mysql_blob_backend::~mysql_blob_backend()
{
	if (buf_)
	{
		delete [] buf_;
		buf_ = 0;
		len_ = 0;
	}
}

std::size_t mysql_blob_backend::get_len()
{
	return len_;
}

std::size_t mysql_blob_backend::read(
	    std::size_t offset, char * buf, std::size_t toRead)
{
	size_t r = toRead;

	// make sure that we don't try to read
	// past the end of the data
	if (r > len_ - offset)
	{
		r = len_ - offset;
	}

	memcpy(buf, buf_ + offset, r);

	return r;
}

std::size_t mysql_blob_backend::write(
	    std::size_t offset, char const * buf,
	    std::size_t toWrite)
{
	const char* oldBuf = buf_;
	std::size_t oldLen = len_;
	len_ = (std::max)(len_, offset + toWrite);

	buf_ = new char[len_];

	if (oldBuf)
	{
		// we need to copy both old and new buffers
		// it is possible that the new does not
		// completely cover the old
		memcpy(buf_, oldBuf, oldLen);
		delete [] oldBuf;
	}
	memcpy(buf_ + offset, buf, len_);

	return len_;
}

std::size_t mysql_blob_backend::append(
	    char const * buf, std::size_t toWrite)
{
    const char* oldBuf = buf_;

    buf_ = new char[len_ + toWrite];

    memcpy(buf_, oldBuf, len_);

    memcpy(buf_ + len_, buf, toWrite);

    delete [] oldBuf;

    len_ += toWrite;

    return len_;
}

void mysql_blob_backend::trim(std::size_t newLen)
{
    const char* oldBuf = buf_;
    len_ = newLen;

    buf_ = new char[len_];

    memcpy(buf_, oldBuf, len_);

    delete [] oldBuf;
}

std::size_t mysql_blob_backend::set_data(char const *buf, std::size_t toWrite)
{
    if (buf_)
    {
        delete [] buf_;
        buf_ = 0;
        len_ = 0;
    }
    return write(0, buf, toWrite);
}

char *mysql_blob_backend::get_data()
{
	return buf_;
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
