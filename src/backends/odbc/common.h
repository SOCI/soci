// Copyright (C) 2024 Your Name or Project
// Distributed under the Boost Software License, Version 1.0.

#pragma once
#include <cstddef>

namespace soci
{
    namespace details
    {
        namespace odbc
        {

            // Safe string copy: copies up to n-1 chars, always null-terminates if n > 0.
            char* soci_strncpy_safe(char* dest, const char* src, std::size_t n);

        } // namespace odbc
    } // namespace details
} // namespace soci
