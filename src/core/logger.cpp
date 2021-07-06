//
// Copyright (C) 2014 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/logger.h"
#include "soci/error.h"

using namespace soci;

namespace // anonymous
{

// Helper to throw from not implemented logger_impl methods.
void throw_not_supported()
{
    throw soci_error("Legacy method not supported by this logger.");
}

} // namespace anonymous


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

    SOCI_DUMMY_RETURN(NULL);
}

std::string logger_impl::get_last_query() const
{
    throw_not_supported();

    SOCI_DUMMY_RETURN(std::string());
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
