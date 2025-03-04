//
// Copyright (C) 2013 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_CONNECTION_PARAMETERS_H_INCLUDED
#define SOCI_CONNECTION_PARAMETERS_H_INCLUDED

#include "soci/soci-platform.h"

#include <map>
#include <string>

namespace soci
{

// Names of some predefined options and their values.
extern SOCI_DECL char const * option_reconnect;
extern SOCI_DECL char const * option_true;

namespace details
{

class dynamic_backend_ref;

} // namespace details

class backend_factory;

// Simple container for the information used when opening a session.
class SOCI_DECL connection_parameters
{
public:
    connection_parameters();
    connection_parameters(backend_factory const & factory, std::string const & connectString);
    connection_parameters(std::string const & backendName, std::string const & connectString);
    explicit connection_parameters(std::string const & fullConnectString);

    connection_parameters(connection_parameters const& other);
    connection_parameters(connection_parameters && other);
    connection_parameters& operator=(connection_parameters const& other);
    connection_parameters& operator=(connection_parameters && other);

    ~connection_parameters();


    // Retrieve the backend and the connection strings specified in the ctor.
    backend_factory const * get_factory() const { return factory_; }
    void set_connect_string(const std::string & connectString) { connectString_ = connectString; }
    std::string const & get_connect_string() const { return connectString_; }


    // For some (but not all) backends the connection string consists of
    // space-separated name=value pairs. This function parses the string
    // assuming it uses this syntax and sets the options accordingly.
    //
    // If it detects invalid syntax, e.g. a name without a corresponding value,
    // it throws an exception.
    //
    // Note that currently unknown options are simply ignored.
    void extract_options_from_space_separated_string();

    // Build a space-separated string from the options, quoting the options
    // values using the provided quote character.
    std::string build_string_from_options(char quote) const;


    // Set the value of the given option, overwriting any previous value.
    void set_option(const char * name, std::string const & value)
    {
        options_[name] = value;
    }

    // Return true if the option with the given name was found and fill the
    // provided parameter with its value.
    bool get_option(const char * name, std::string & value) const
    {
        Options::const_iterator const it = options_.find(name);
        if (it == options_.end())
            return false;

        value = it->second;

        return true;
    }

    // Same as get_option(), but also removes the option from the connection
    // string if it was present in it.
    bool extract_option(const char * name, std::string & value)
    {
        Options::iterator const it = options_.find(name);
        if (it == options_.end())
            return false;

        value = it->second;
        options_.erase(it);

        return true;
    }

    // Return true if the option with the given name has one of the values
    // considered to be true, i.e. "1", "yes", "true" or "on" or is empty.
    // Return false if the value is one of "0", "no", "false" or "off" or the
    // option was not specified at all.
    //
    // Throw an exception if the option was given but the value is none of
    // the above, comparing case-insensitively.
    static bool is_true_value(const char * name, std::string const & value);

    // Return true if the option with the given name was found with a "true"
    // value in the sense of is_true_value() above.
    bool is_option_on(const char * name) const
    {
      std::string value;
      return get_option(name, value) && is_true_value(name, value);
    }

private:
    void reset_after_move();

    // The backend and connection string specified in our ctor.
    backend_factory const * factory_;
    std::string connectString_;

    // References the backend name used for obtaining the factor from
    // dynamic_backends.
    details::dynamic_backend_ref * backendRef_;

    // We store all the values as strings for simplicity.
    typedef std::map<std::string, std::string> Options;
    Options options_;
};

} // namespace soci

#endif // SOCI_CONNECTION_PARAMETERS_H_INCLUDED
