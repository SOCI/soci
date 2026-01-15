//
// Copyright (C) 2026 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_FMT_H_INCLUDED
#define SOCI_FMT_H_INCLUDED

#include <string>

namespace soci
{

// Export a couple of overloads of format() used in public headers to allow
// using them there without requiring including fmt headers.
std::string SOCI_DECL format(const char* fmt, std::string const& arg);
std::string SOCI_DECL format(const char* fmt, int arg);
std::string SOCI_DECL format(const char* fmt, size_t arg);

} // namespace soci

#endif // SOCI_FMT_H_INCLUDED
