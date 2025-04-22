//
// Copyright (C) 2004-2008 Maciej Sobczak
// Copyright (C) 2015 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ERROR_H_INCLUDED
#define SOCI_ERROR_H_INCLUDED

#include "soci/soci-platform.h"
// std
#include <stdexcept>
#include <string>

namespace soci
{

class SOCI_DECL soci_error : public std::runtime_error
{
public:
    explicit soci_error(std::string const & msg);

    soci_error(soci_error const& e) noexcept;
    soci_error& operator=(soci_error const& e) noexcept;

    ~soci_error() noexcept override;

    // Returns just the error message itself, without the context.
    std::string get_error_message() const;

    // Returns the full error message combining the message given to the ctor
    // with all the available context records.
    char const* what() const noexcept override;

    // This is used only by SOCI itself to provide more information about the
    // exception as it bubbles up. It can be called multiple times, with the
    // first call adding the lowest level context and the last one -- the
    // highest level context.
    void add_context(std::string const& context);

    // Basic error classes.
    enum error_category
    {
        connection_error,
        invalid_statement,
        no_privilege,
        no_data,
        constraint_violation,
        unknown_transaction_state,
        system_error,
        unknown
    };

    // Basic error classification support
    virtual error_category get_error_category() const { return unknown; }


    // The base class allows to access backend-specific error information,
    // which can be useful to avoid linking with the backend libraries.

    // Return the backend name or empty string if this is a core SOCI error.
    virtual std::string get_backend_name() const { return {}; }

    // Return the backend-specific error code as integer or 0 if inapplicable.
    virtual int get_backend_error_code() const { return 0; }

    // Return the 5 character SQL state or empty if inapplicable.
    virtual std::string get_sqlstate() const { return {}; }

private:
    // Optional extra information (currently just the context data).
    class soci_error_extra_info* info_;
};

} // namespace soci

#endif // SOCI_ERROR_H_INCLUDED
