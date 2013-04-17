//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE

#include "error.h"

using namespace soci;

soci_error::soci_error(std::string const & msg)
     : std::runtime_error(msg)
{
}

log_stream::log_stream(std::ostream *log /*= NULL*/)
    : log_(log)
    , log_params_(false)
    , log_flush_(false)
{
}

void log_stream::write(const char * val, std::size_t size)
{
    if (!is_null())
        log().write(val, size);
}

void log_stream::log_flush(bool enabled /*= true*/)
{
    log_flush_ = enabled;
}

void log_stream::log_params(bool enabled)
{
    log_params_ = enabled;
}

log_stream& log_stream::for_params()
{
    if (log_params_)
    {
        return *this;
    }
    static log_stream null_log;
    return null_log;
}

void log_stream::end_line()
{
    if (!is_null())
    {
        flush();
    }
}

inline bool log_stream::is_null() const
{
    return (log_ == NULL);
}

void log_stream::flush()
{
    if (log_flush_)
    {
        log() << std::endl;
    }
    else
    {
        log() << '\n';
    }
}

inline std::ostream & log_stream::log()
{
    return *log_;
}