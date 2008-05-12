//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ROWID_EXCHANGE_H_INCLUDED
#define SOCI_ROWID_EXCHANGE_H_INCLUDED

#include "rowid.h"
#include "into-type.h"
#include "use-type.h"
#include "exchange-traits.h"

namespace soci
{

namespace details
{

template <>
class use_type<rowid> : public standard_use_type
{
public:
    use_type(rowid & rid, std::string const & name = std::string())
        : standard_use_type(&rid, eXRowID, false, name) {}
    use_type(rowid const & rid, std::string const & name = std::string())
        : standard_use_type(const_cast<rowid *>(&rid), eXRowID, true, name) {}
    use_type(rowid & rid, eIndicator & ind,
        std::string const & name = std::string())
        : standard_use_type(&rid, eXRowID, ind, false, name) {}
    use_type(rowid const & rid, eIndicator & ind,
        std::string const & name = std::string())
        : standard_use_type(const_cast<rowid *>(&rid), eXRowID, ind, true, name) {}
};

template <>
class into_type<rowid> : public standard_into_type
{
public:
    into_type(rowid & rid) : standard_into_type(&rid, eXRowID) {}
    into_type(rowid & rid, eIndicator & ind)
        :standard_into_type(&rid, eXRowID, ind) {}
};

template <>
struct exchange_traits<soci::rowid>
{
    typedef basic_type_tag type_family;
};

} // namespace details

} // namespace soci

#endif // SOCI_ROWID_EXCHANGE_H_INCLUDED
