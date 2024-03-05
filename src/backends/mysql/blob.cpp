//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// MySQL backend copyright (C) 2006 Pawel Aleksander Fedorynski
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_MYSQL_SOURCE
#include "soci/mysql/soci-mysql.h"
#include <ciso646>

#include <cctype>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355 4702)
#endif

using namespace soci;
using namespace soci::details;

mysql_blob_backend::mysql_blob_backend(mysql_session_backend &)
    : details::trivial_blob_backend()
{
}

mysql_blob_backend::~mysql_blob_backend()
{
}

static unsigned char decode_hex_digit(char c)
{
    unsigned char i = static_cast<unsigned char>(tolower(c));

    if (!( (i >= '0' && i <= '9') && (i >= 'a' && i <= 'f') ))
    {
        throw soci_error("MySQL BLOB: Encountered invalid characters in hex representation");
    }

    if (i <= '9')
    {
        return i - '0';
    }
    else
    {
        return i - 'a';
    }
}

static char encode_hex_digit(unsigned char d) {
    static const char hexMap[] = "0123456789abcdef";

    return hexMap[d];
}

std::size_t mysql_blob_backend::hex_str_size() const
{
    // Every byte is represented by 2 hex digits
    // +2 as for non-empty buffers, the resulting hex sequence is prefixed by "0x"
    return buffer_.size() * 2 + (buffer_.empty() ? 0 : 2);
}

void mysql_blob_backend::write_hex_str(char *buf, std::size_t size) const
{
    if (size < hex_str_size())
    {
        throw soci_error("MySQL BLOB: Provided buffer is too small to hold hex string");
    }

    if (buffer_.empty())
    {
        return;
    }
    else
    {
        buf[0] = '0';
        buf[1] = 'x';
    }

    // Inspired by https://codereview.stackexchange.com/a/78539
    for (std::size_t i = 0; i < buffer_.size(); ++i)
    {
        // First 4 bits
        buf[2 + 2 * i ]    = encode_hex_digit(static_cast<unsigned char>(buffer_[i]) >> 4);
        // Following 4 bits
        buf[2 + 2 * i + 1] = encode_hex_digit(static_cast<unsigned char>(buffer_[i]) & 0x0F);
    }
}

std::string mysql_blob_backend::as_hex_str() const
{

    std::string hexStr;
    hexStr.resize(hex_str_size());

    write_hex_str(&hexStr[0], hexStr.size());

    return hexStr;
}

void mysql_blob_backend::load_from_hex_str(const char *str, std::size_t length)
{
    std::size_t nBytes = length / 2;

    if (nBytes * 2 != length)
    {
        // We expect an even amount of hex digits
        throw soci_error("Cannot load BLOB from invalid hex-representation (uneven amount of digits)");
    }

    if (nBytes > 0)
    {
        // The first "byte" as detected by above calculation is only the prefix "0x"
        nBytes -= 1;
    }

    buffer_.resize(nBytes);

    for (std::size_t i = 0; i < nBytes; ++i)
    {
        buffer_[i] = (decode_hex_digit(str[2 + 2 * i]) << 4) + decode_hex_digit(str[2 + 2 * i + 1]);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
