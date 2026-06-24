//
// Copyright (C) 2026 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_DEFS_H_INCLUDED
#define SOCI_DEFS_H_INCLUDED

// This header contains miscellaneous simple definitions used by SOCI.

namespace soci
{

// Type of the database engine used by SOCI session.
enum class database_engine
{
    unknown = 0,
    db2,
    firebird,
    mssql,
    mysql,
    oracle,
    postgresql,
    sqlite3,
};

} // namespace soci

#endif // SOCI_DEFS_H_INCLUDED
