//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE

#include "error.h"

soci::soci_error::soci_error(std::string const & msg)
     : std::runtime_error(msg)
{
}

/*static*/ const std::string& soci::log_stream::nothing()
{
	static std::string value = "";
	return value;
}

/*static*/ const std::string& soci::log_stream::params_next_line()
{
	static std::string value = "\n";
	return value;
}

/*static*/ const std::string& soci::log_stream::params_same_line()
{
	static std::string value = "; ";
	return value;
}

soci::log_stream::log_stream(std::ostream *log /*= NULL*/)
    : log_(log)
    , params_sep_(soci::log_stream::nothing())
    , log_flush_(false)
{
}

void soci::log_stream::write(const char * val, std::size_t size)
{
	if (!is_null())
        log().write(val, size);
}

void soci::log_stream::log_flush(bool enabled /*= true*/)
{
    log_flush_ = enabled;
}

void soci::log_stream::log_params(const std::string& separator /*= log_stream::params_next_line*/)
{
    params_sep_ = separator;
}

soci::log_stream& soci::log_stream::for_params()
{
    if (params_sep_.empty())
    {
        static log_stream null_log;
        return null_log;
    }
    return *this;
}

void soci::log_stream::start_params()
{
    if (params_sep_.empty() || params_sep_ == params_next_line())
    {
        end_line();
    }
    else if (log_ != NULL)
    {
        log() << params_sep_;
    }
}

void soci::log_stream::end_line(const std::string& line_end_sufix /*= log_stream::nothing*/)
{
	if (!is_null())
    {
        if (!line_end_sufix.empty())
        {
            log() << line_end_sufix;
        }

        if (log_flush_)
        {
            log() << std::endl;
        }
        else
        {
            log() << '\n';
        }
    }
}

inline bool soci::log_stream::is_null() const
{
	return (log_ == NULL);
}

inline std::ostream & soci::log_stream::log()
{
	return *log_;
}