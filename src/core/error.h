//
// Copyright (C) 2004-2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ERROR_H_INCLUDED
#define SOCI_ERROR_H_INCLUDED

#include "soci-config.h"
// std
#include <stdexcept>
#include <string>
#include <ostream>

namespace soci
{

class SOCI_DECL soci_error : public std::runtime_error
{
public:
    explicit soci_error(std::string const & msg);
};

// simple class for conditional logging

class SOCI_DECL log_stream
{
public:
    static const std::string& nothing();
    static const std::string& params_next_line();
    static const std::string& params_same_line();

public:
    log_stream(std::ostream *log = NULL);

    template <typename T>
    log_stream& operator<<(const T& val);

    void write(const char * val, std::size_t size);

    void log_flush(bool enabled = true);

    void log_params(const std::string& separator = params_next_line());

    log_stream& for_params();

    void start_params();

    void end_line(const std::string& line_end_sufix = nothing());

	bool is_null() const;

private:
	std::ostream & log();

private:
    bool log_flush_;
    std::string params_sep_;
    std::ostream * log_;
};

template <typename T>
log_stream& log_stream::operator<<(const T& val)
{
    if (!is_null())
        log() << val;
    return *this;
}

} // namespace soci

#endif // SOCI_ERROR_H_INCLUDED
