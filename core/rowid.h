//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ROWID_H_INCLUDED
#define SOCI_ROWID_H_INCLUDED

#include "soci-backend.h"
#include "into-type.h"
#include "use-type.h"

namespace soci
{
class session;

// ROWID support

class SOCI_DECL rowid
{
public:
    rowid(session &s);
    ~rowid();

    details::rowid_backend * get_backend() { return backEnd_; }

private:
    details::rowid_backend *backEnd_;
};

namespace details
{

template <>
class use_type<rowid> : public standard_use_type
{
public:
    use_type(rowid &rid, std::string const &name = std::string())
        : standard_use_type(&rid, eXRowID, name) {}
    use_type(rowid &rid, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&rid, eXRowID, ind, name) {}
};

template <>
class into_type<rowid> : public standard_into_type
{
public:
    into_type(rowid &rid) : standard_into_type(&rid, eXRowID) {}
    into_type(rowid &rid, eIndicator &ind)
        :standard_into_type(&rid, eXRowID, ind) {}
};

template <>
struct exchange_traits<soci::rowid>
{
    typedef basic_type_tag type_family;
};

} // namespace details
} // namespace soci

#endif
