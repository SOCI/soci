//
// Copyright (C) 2013 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/connection-parameters.h"
#include "soci/soci-backend.h"
#include "soci/backend-loader.h"

char const * soci::option_reconnect = "reconnect";

char const * soci::option_true = "1";

namespace // anonymous
{

void parseConnectString(std::string const & connectString,
    std::string & backendName,
    std::string & connectionParameters)
{
    std::string const protocolSeparator = "://";

    std::string::size_type const p = connectString.find(protocolSeparator);
    if (p == std::string::npos)
    {
        throw soci::soci_error("No backend name found in " + connectString);
    }

    backendName = connectString.substr(0, p);
    connectionParameters = connectString.substr(p + protocolSeparator.size());
}

} // namespace anonymous

namespace soci
{

namespace details
{

// Reference-counted dynamic backend reference which is used to ensure that we
// call dynamic_backends::unget() the same number of times as we call get().
class dynamic_backend_ref
{
public:
    explicit dynamic_backend_ref(std::string const & backendName)
      : backendName_(backendName),
        refCount_(1)
    {
    }

    ~dynamic_backend_ref()
    {
        dynamic_backends::unget(backendName_);
    }

    void inc_ref() { ++refCount_; }
    void dec_ref()
    {
        if (--refCount_ == 0)
        {
            delete this;
        }
    }

private:
    friend class soci::connection_parameters;

    // Empty when using the user-provided factory only, otherwise contains the
    // backend name when the factory was obtained from dynamic_backends.
    std::string backendName_;

    int refCount_;
};

} // namespace details

connection_parameters::connection_parameters()
    : factory_(NULL), backendRef_(NULL)
{
}

connection_parameters::connection_parameters(backend_factory const & factory,
    std::string const & connectString)
    : factory_(&factory), connectString_(connectString), backendRef_(NULL)
{
}

connection_parameters::connection_parameters(std::string const & backendName,
    std::string const & connectString)
    : factory_(&dynamic_backends::get(backendName)),
      connectString_(connectString),
      backendRef_(new details::dynamic_backend_ref(backendName))
{
}

connection_parameters::connection_parameters(std::string const & fullConnectString)
{
    std::string backendName;
    std::string connectString;

    parseConnectString(fullConnectString, backendName, connectString);

    factory_ = &dynamic_backends::get(backendName);
    connectString_ = connectString;
    backendRef_ = new details::dynamic_backend_ref(backendName);
}

connection_parameters::connection_parameters(connection_parameters const& other)
    : factory_(other.factory_),
      connectString_(other.connectString_),
      backendRef_(other.backendRef_),
      options_(other.options_)
{
    if (backendRef_)
        backendRef_->inc_ref();
}

connection_parameters& connection_parameters::operator=(connection_parameters const& other)
{
    // Order is important in case of self-assignment.
    if (other.backendRef_)
        other.backendRef_->inc_ref();
    if (backendRef_)
        backendRef_->dec_ref();

    factory_ = other.factory_;
    connectString_ = other.connectString_;
    backendRef_ = other.backendRef_;
    options_ = other.options_;

    return *this;
}

connection_parameters::~connection_parameters()
{
    if (backendRef_)
      backendRef_->dec_ref();
}

} // namespace soci
