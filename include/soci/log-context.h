//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_LOG_CONTEXT_H_INCLUDED
#define SOCI_LOG_CONTEXT_H_INCLUDED

namespace soci
{

enum class log_context
{
    never,
    always,
    on_error,
};

} // namespace soci

#endif // SOCI_LOG_CONTEXT_H_INCLUDED
