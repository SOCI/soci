//
// Copyright (C) 2014 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/logger.h"
#include "soci/error.h"

using namespace soci;

namespace // anonymous
{

// Helper to throw from not implemented logger_impl methods.
[[noreturn]] void throw_not_supported()
{
    throw soci_error("Legacy method not supported by this logger.");
}

} // namespace anonymous


void logger_impl::start_query(const std::string &)
{
    clear_query_parameters();
}

void logger_impl::add_query_parameter(std::string name, std::string value)
{
    queryParams_.emplace_back(std::move(name), std::move(value));
}

void logger_impl::clear_query_parameters()
{
    queryParams_.clear();
}

logger_impl * logger_impl::clone() const
{
    logger_impl * const impl = do_clone();
    if (!impl)
    {
        throw soci_error("Cloning a logger implementation must work.");
    }

    return impl;
}

logger_impl::~logger_impl()
{
}

void logger_impl::set_stream(std::ostream *)
{
    throw_not_supported();
}

std::ostream * logger_impl::get_stream() const
{
    throw_not_supported();
}

std::string logger_impl::get_last_query() const
{
    throw_not_supported();
}

std::string logger_impl::get_last_query_context() const
{
    std::string context;

    bool first = true;
    for (const query_parameter &param : queryParams_)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            context += ", ";
        }

        context += ":" + param.name + "=" + param.value;
    }

    return context;
}

logger::logger(logger_impl * impl)
    : m_impl(impl)
{
    if (!m_impl)
    {
        throw soci_error("Null logger implementation not allowed.");
    }
}

logger::logger(logger const & other)
    : m_impl(other.m_impl->clone())
{
}

logger& logger::operator=(logger const & other)
{
    logger_impl * const implOld = m_impl;
    m_impl = other.m_impl->clone();
    delete implOld;

    return *this;
}

logger::~logger()
{
    delete m_impl;
}
