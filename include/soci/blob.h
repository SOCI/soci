//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BLOB_H_INCLUDED
#define SOCI_BLOB_H_INCLUDED

#include "soci/soci-platform.h"
// std
#include <cstddef>
#include <memory>

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
    // Creates an invalid blob object
    blob() = default;
    explicit blob(session & s);
    ~blob();

    blob(blob &&other) = default;
    blob &operator=(blob &&other) = default;

    // Checks whether this blob is in a valid state
    bool is_valid() const;

    // (Re)initializes this blob
    void initialize(session &s);
    void initialize(details::blob_backend *backend);

    std::size_t get_len();

    // offset is backend-specific
#ifndef SOCI_SOURCE
    [[deprecated("Use read_from_start instead")]]
#endif
    std::size_t read(std::size_t offset, void * buf, std::size_t toRead);

    // Extracts data from this blob into the given buffer.
    // At most toRead bytes are extracted (and copied into buf).
    // The amount of actually read bytes is returned.
    //
    // Note: Using an offset > 0 on a blob whose size is less than
    // or equal to offset, will throw an exception.
    std::size_t read_from_start(void * buf, std::size_t toRead,
        std::size_t offset = 0);

    // offset is backend-specific
#ifndef SOCI_SOURCE
    [[deprecated("Use write_from_start instead")]]
#endif
    std::size_t write(std::size_t offset, const void * buf,
        std::size_t toWrite);

    // offset starts from 0
    std::size_t write_from_start(const void * buf, std::size_t toWrite,
        std::size_t offset = 0);

    std::size_t append(const void * buf, std::size_t toWrite);

    void trim(std::size_t newLen);

    details::blob_backend * get_backend() { return backEnd_.get(); }

private:
    SOCI_NOT_COPYABLE(blob)

    std::unique_ptr<details::blob_backend> backEnd_;

    void ensure_initialized();
};

} // namespace soci

#endif
