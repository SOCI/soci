//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BLOB_EXCHANGE_H_INCLUDED
#define BLOB_EXCHANGE_H_INCLUDED

#include "blob.h"
#include "into-type.h"
#include "use-type.h"

namespace soci
{

namespace details
{

template <>
class into_type<blob> : public standard_into_type
{
public:
    into_type(blob &b) : standard_into_type(&b, eXBLOB) {}
    into_type(blob &b, eIndicator &ind)
        : standard_into_type(&b, eXBLOB, ind) {}
};

template <>
class use_type<blob> : public standard_use_type
{
public:
    use_type(blob &b, std::string const &name = std::string())
        : standard_use_type(&b, eXBLOB, false, name) {}
    use_type(const blob &b, std::string const &name = std::string())
        : standard_use_type(const_cast<blob *>(&b), eXBLOB, true, name) {}
    use_type(blob &b, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&b, eXBLOB, ind, false, name) {}
    use_type(const blob &b, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(const_cast<blob *>(&b), eXBLOB, ind, true, name) {}
};

template <>
struct exchange_traits<soci::blob>
{
    typedef basic_type_tag type_family;
};

} // namespace details

} // namespace soci

#endif // BLOB_EXCHANGE_H_INCLUDED
