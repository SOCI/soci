//
// Copyright (C) 2004-2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TRANSACTION_H_INCLUDED
#define SOCI_TRANSACTION_H_INCLUDED

#include "soci/soci-platform.h"
#include "soci/session.h"

namespace soci
{

class SOCI_DECL transaction
{
public:
    explicit transaction(session& sql);

    ~transaction();

    void commit();
    void rollback();

    inline session* current_session();

    bool is_active();
    bool by_session();

private:
    //Private constructor use from session object in case not assigned transaction.
    //Used in src/core/session.cpp:343
    transaction(session& sql, bool by_session);

    bool handled_;
    session& sql_;
    bool by_session_;

    friend class session;

    SOCI_NOT_COPYABLE(transaction)
};

} // namespace soci

#endif // SOCI_TRANSACTION_H_INCLUDED
