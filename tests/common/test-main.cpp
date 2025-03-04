//
// Copyright (C) 2024 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include "test-context.h"

using namespace soci;

tests::test_context_base* tests::test_context_base::the_test_context_ = NULL;

int main(int argc, char** argv)
{
    auto* const tc = tests::test_context_base::get_instance_pointer();
    if (!tc)
    {
        std::cerr << "Internal error: each test must create its test context.\n";
        return EXIT_FAILURE;
    }

#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    std::string argFromCommandLine;
    if (argc >= 2 && argv[1][0] != '-')
    {
        argFromCommandLine = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }

    if ( !tc->initialize_connect_string(std::move(argFromCommandLine)) )
    {
        std::cerr << "usage: " << argv[0]
            << " <connection-string> [test-arguments...]\n";

        auto const& dsn = tc->get_example_connection_string();
        if ( !dsn.empty() )
        {
            std::cerr << "example: " << argv[0] << " \'" << dsn << "\'\n";
        }

        return EXIT_FAILURE;
    }

    if ( !tc->start_testing() )
    {
        return EXIT_FAILURE;
    }

    return Catch::Session().run(argc, argv);
}
