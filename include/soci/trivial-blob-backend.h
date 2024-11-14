#ifndef SOCI_TRIVIAL_BLOB_BACKEND_H_INCLUDED
#define SOCI_TRIVIAL_BLOB_BACKEND_H_INCLUDED

#include "soci/soci-backend.h"
#include "soci/session.h"

#include <vector>
#include <cstring>
#include <cstdint>

namespace soci
{

namespace details
{

/**
 * This Blob implementation uses an explicit buffer that is read from and written to, instead of
 * directly communicating with the underlying database.
 * Thus, it is intended to be used whenever the underlying database does not offer a more efficient
 * way of dealing with BLOBs.
 */
class trivial_blob_backend : public details::blob_backend
{
public:
    trivial_blob_backend(details::session_backend &backend) : session_(backend) {}

    std::size_t get_len() override { return buffer_.size(); }

    std::size_t read_from_start(void* buf, std::size_t toRead,
        std::size_t offset = 0) override
    {
        if (offset > buffer_.size() || (offset ==  buffer_.size() && offset > 0))
        {
            throw soci_error("Can't read past-the-end of BLOB data.");
        }

        // make sure that we don't try to read past the end of the data
        toRead = std::min<decltype(toRead)>(toRead, buffer_.size() - offset);

        // copy the data if there is anything to copy: note that not doing it
        // when toRead == 0 is more than an optimization, as we could pass an
        // invalid source pointer to memcpy() if we didn't check for this case
        if (toRead)
            memcpy(buf, buffer_.data() + offset, toRead);

        return toRead;
    }

    std::size_t write_from_start(const void* buf, std::size_t toWrite,
        std::size_t offset = 0) override
    {
        if (offset > buffer_.size())
        {
            throw soci_error("Can't start writing far past-the-end of BLOB data.");
        }

        buffer_.resize(std::max<std::size_t>(buffer_.size(), offset + toWrite));

        if (toWrite)
            memcpy(buffer_.data() + offset, buf, toWrite);

        return toWrite;
    }

    std::size_t append(void const* buf, std::size_t toWrite) override
    {
        return write_from_start(buf, toWrite, buffer_.size());
    }

    void trim(std::size_t newLen) override { buffer_.resize(newLen); }

    std::size_t set_data(void const* buf, std::size_t toWrite)
    {
        buffer_.clear();
        return write_from_start(buf, toWrite);
    }

    const std::uint8_t *get_buffer() const { return buffer_.data(); }

    details::session_backend &get_session_backend() override { return session_; }

protected:
    details::session_backend &session_;
    std::vector< std::uint8_t > buffer_;
};

}

}

#endif // SOCI_TRIVIAL_BLOB_BACKEND_H_INCLUDED
