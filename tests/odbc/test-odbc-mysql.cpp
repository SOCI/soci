//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/odbc/soci-odbc.h"
#include "mysql/test-mysql.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

std::string connectString;
backend_factory const &backEnd = *soci::factory_odbc();

class test_context_odbc : public test_context
{
public:
    using test_context::test_context;

    bool truncates_uint64_to_int64() const override
    {
        // The ODBC driver of MySQL truncates values bigger then INT64_MAX.
        // There are open bugs related to this issue:
        // - https://bugs.mysql.com/bug.php?id=95978
        // - https://bugs.mysql.com/bug.php?id=61114
        // Driver version 8.0.31 seems to have fixed this (https://github.com/mysql/mysql-connector-odbc/commit/e78da1344247752f76a082de51cfd36d5d2dd98f),
        // but we use an older version in the AppVeyor builds.
        return true;
    }
};

int main(int argc, char** argv)
{
#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc >= 2 && argv[1][0] != '-')
    {
        connectString = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }
    else
    {
        connectString = "FILEDSN=./test-mysql.dsn";
    }

    test_context_odbc tc(backEnd, connectString);

    return Catch::Session().run(argc, argv);
}
