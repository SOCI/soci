//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TEST_FOREIGNKEYS_H_INCLUDED
#define SOCI_TEST_FOREIGNKEYS_H_INCLUDED

#include "soci/soci.h"

// Creates a pair of tables with a foreign key relationship between them in
// ctor and destroys them in dtor, allowing to check what happens when the
// foreign key constraint is violated.
class SetupForeignKeys
{
public:
    explicit SetupForeignKeys(soci::session& sql)
        : m_sql(sql)
    {
        m_sql <<
        "create table parent ("
        "    id integer primary key"
        ")";

        m_sql <<
        "create table child ("
        "    id integer primary key,"
        "    parent integer,"
        "    foreign key(parent) references parent(id)"
        ")";

        m_sql << "insert into parent(id) values(1)";
        m_sql << "insert into child(id, parent) values(100, 1)";
    }

    ~SetupForeignKeys()
    {
        m_sql << "drop table child";
        m_sql << "drop table parent";
    }

private:
    SetupForeignKeys(const SetupForeignKeys&);
    SetupForeignKeys& operator=(const SetupForeignKeys&);

    soci::session& m_sql;
};

#endif // SOCI_TEST_FOREIGNKEYS_H_INCLUDED
