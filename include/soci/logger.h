//
// Copyright (C) 2014 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_LOGGER_H_INCLUDED
#define SOCI_LOGGER_H_INCLUDED

#include "soci/soci-platform.h"

#include <ostream>
#include <vector>

namespace soci
{


struct SOCI_DECL query_parameter
{
    explicit query_parameter(std::string name = {}, std::string value = {})
        : name(std::move(name)), value(std::move(value)) {}

    std::string name;
    std::string value;
};

// Allows to customize the logging of database operations performed by SOCI.
//
// To do it, derive your own class from logger_impl and override its pure
// virtual start_query() and do_clone() methods (overriding the other methods
// is optional), then call session::set_logger() with a logger object using
// your implementation.
class SOCI_DECL logger_impl
{
public:
    logger_impl() {}
    virtual ~logger_impl();

    // Called to indicate that a new query is about to be executed.
    virtual void start_query(std::string const & query);

    // Called to log a parameter that is bound to the currently active query
    virtual void add_query_parameter(std::string name, std::string value);

    // Clears all currently logged query parameters
    virtual void clear_query_parameters();

    logger_impl * clone() const;

    // These methods are for compatibility only as they're used to implement
    // session basic logging support, you should only override them if you want
    // to use session::set_stream() and similar methods with your custom logger.
    virtual void set_stream(std::ostream * s);
    virtual std::ostream * get_stream() const;
    virtual std::string get_last_query() const;
    virtual std::string get_last_query_context() const;

protected:
    std::vector<query_parameter> queryParams_;

private:
    // Override to return a new heap-allocated copy of this object.
    virtual logger_impl * do_clone() const = 0;

    // Non-copyable
    logger_impl(logger_impl const &);
    logger_impl & operator=(logger_impl const &);
};


// A wrapper class representing a logger.
//
// Unlike logger_impl, this class has value semantics and can be manipulated
// easily without any danger of memory leaks or dereferencing a NULL pointer.
class SOCI_DECL logger
{
public:
    // No default constructor, must always have an associated logger_impl.

    // Create a logger using the provided non-NULL implementation (an exception
    // is thrown if the pointer is NULL). The logger object takes ownership of
    // the pointer and will delete it.
    logger(logger_impl * impl);
    logger(logger const & other);
    logger& operator=(logger const & other);
    ~logger();

    void start_query(std::string const & query) { m_impl->start_query(query); }

    virtual void add_query_parameter(std::string name, std::string value)
    {
        m_impl->add_query_parameter(std::move(name), std::move(value));
    }

    virtual void clear_query_parameters() { m_impl->clear_query_parameters(); }

    // Methods used for the implementation of session basic logging support.
    void set_stream(std::ostream * s) { m_impl->set_stream(s); }
    std::ostream * get_stream() const { return m_impl->get_stream(); }
    std::string get_last_query() const { return m_impl->get_last_query(); }
    std::string get_last_query_context() const { return m_impl->get_last_query_context(); }

private:
    logger_impl * m_impl;
};

} // namespace soci

#endif // SOCI_LOGGER_H_INCLUDED
