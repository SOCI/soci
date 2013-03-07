//
// Copyright (C) 2013 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_CONNECTION_PARAMETERS_H_INCLUDED
#define SOCI_CONNECTION_PARAMETERS_H_INCLUDED

#include "soci-config.h"

#include <string>

namespace soci
{

class backend_factory;

// Simple container for the information used when opening a session.
class SOCI_DECL connection_parameters
{
public:
    connection_parameters();
    connection_parameters(backend_factory const & factory, std::string const & connectString);
    connection_parameters(std::string const & backendName, std::string const & connectString);
    explicit connection_parameters(std::string const & fullConnectString);

    // Default copy ctor, assignment operator and dtor are all OK for us.

private:
    // The backend and connection string specified in our ctor.
    backend_factory const * factory_;
    std::string connectString_;
};

} // namespace soci

#endif // SOCI_CONNECTION_PARAMETERS_H_INCLUDED
