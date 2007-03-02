//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_BLOB_H_INCLUDED
#define SOCI_BLOB_H_INCLUDED

#include "into-type.h"
#include "use-type.h"
#include "soci-backend.h"

namespace soci
{
// basic blob  operations

class SOCI_DECL blob
{
public:
    blob(session &s);
    ~blob();

    std::size_t get_len();
    std::size_t read(std::size_t offset, char *buf, std::size_t toRead);
    std::size_t write(std::size_t offset, char const *buf,
        std::size_t toWrite);
    std::size_t append(char const *buf, std::size_t toWrite);
    void trim(std::size_t newLen);

    details::blob_backend * get_backend() { return backEnd_; }

private:
    details::blob_backend *backEnd_;
};

namespace details
{

template <>
class into_type<blob > : public standard_into_type
{
public:
    into_type(blob  &b) : standard_into_type(&b, eXBLOB) {}
    into_type(blob  &b, eIndicator &ind)
        : standard_into_type(&b, eXBLOB , ind) {}
};

template <>
class use_type<blob > : public standard_use_type
{
public:
    use_type(blob  &b, std::string const &name = std::string())
        : standard_use_type(&b, eXBLOB , name) {}
    use_type(blob  &b, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&b, eXBLOB , ind, name) {}
};

} // namespace details

} // namespace soci

#endif
