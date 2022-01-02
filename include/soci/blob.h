//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BLOB_H_INCLUDED
#define SOCI_BLOB_H_INCLUDED

#include "soci/soci-platform.h"
// std
#include <cstddef>
#include <vector>

namespace soci
{
// basic blob operations

class session;

namespace details
{
class blob_backend;
} // namespace details

class SOCI_DECL blob
{
public:
    blob();

    // Keeping it for back compatibility
    // [[deprecated]]
    explicit blob(session & s);

    blob(const blob& b) { this->backEnd_ = b.backEnd_; }
    ~blob();

    std::size_t get_len();

    std::size_t size() { return data_.size(); }
    void resize(std::size_t new_size) { data_.resize(new_size); }
    char& operator[](int& i) { return data_[i]; }
    const char& operator[](int& i) const { return data_[i]; }
    char& operator[](std::size_t i) { return data_[i]; }
    const char& operator[](std::size_t i) const { return data_[i]; }

    // offset is backend-specific
    std::size_t read(std::size_t offset, char * buf, std::size_t toRead);

    // offset starts from 0
    inline std::size_t read_from_start(char * buf, std::size_t toRead,
        std::size_t offset = 0)
    {
        return this->read(offset, buf, toRead);
    }

    // offset is backend-specific
    std::size_t write(std::size_t offset, char const * buf,
        std::size_t toWrite);

    inline std::size_t write_from_start(const char * buf, std::size_t toWrite,
        std::size_t offset = 0)
    {
        return this->write(offset, buf, toWrite);
    }

    std::size_t append(char const * buf, std::size_t toWrite);

    void trim(std::size_t newLen);

    // Keeping it for back compatibility
    // [[deprecated]]
    details::blob_backend* get_backend() { return backEnd_; }

private:
    details::blob_backend* backEnd_;

    // buffer for BLOB data
    std::vector<char> data_;
};

} // namespace soci

#endif
