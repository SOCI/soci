//
// Copyright (C) 2013 Vadim Zeitlin
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/connection-parameters.h"
#include "soci/soci-backend.h"
#include "soci/backend-loader.h"

#include "soci-case.h"

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

connection_parameters::connection_parameters(connection_parameters && other)
    : factory_(std::move(other.factory_)),
      connectString_(std::move(other.connectString_)),
      backendRef_(std::move(other.backendRef_)),
      options_(std::move(other.options_))
{
    other.reset_after_move();
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

connection_parameters& connection_parameters::operator=(connection_parameters && other)
{
    if (backendRef_ && backendRef_ != other.backendRef_)
        backendRef_->dec_ref();

    factory_ = std::move (other.factory_);
    connectString_ = std::move(other.connectString_);
    backendRef_ = std::move(other.backendRef_);
    options_ = std::move(other.options_);

    other.reset_after_move();

    return *this;
}

connection_parameters::~connection_parameters()
{
    if (backendRef_)
      backendRef_->dec_ref();
}

void connection_parameters::reset_after_move()
{
    factory_ = nullptr;
    backendRef_ = nullptr;
}

/* static */
bool
connection_parameters::is_true_value(char const* name, std::string const& value)
{
    // At least for compatibility (but also because this is convenient and
    // makes sense), we accept "readonly" as synonym for "readonly=1" etc.
    if (value.empty())
        return true;

    std::string const val = details::string_tolower(value);

    if (val == "1" || val == "yes" || val == "true" || val == "on")
        return true;

    if (val == "0" || val == "no" || val == "false" || val == "off")
        return false;

    std::ostringstream os;
    os << R"(Invalid value ")"
       << value
       << R"(" for boolean option ")"
       << name
       << '"';
    throw soci_error(os.str());
}

namespace
{

// Helpers of extract_options_from_space_separated_string() for reading words
// from a string. "Word" here is defined very loosely as just a sequence of
// non-space characters.

// We could use std::isspace() but it doesn't seem worth it to create a locale
// just for this.
inline bool isSpace(std::string::const_iterator i)
{
    return *i == ' ' || *i == '\t';
}

// All the functions below update the input iterator to point to the first
// character not consumed by them.

// Advance the input iterator until the first non-space character or end of the
// string.
void
skipWhiteSpace(std::string::const_iterator& i,
               std::string::const_iterator const& end)
{
    for (; i != end; ++i)
    {
        if (!isSpace(i))
            break;
    }
}

// Return a possibly quoted word, i.e. either just a sequence of non-space
// characters or everything inside a double-quoted string.
//
// Throws if the word is quoted and the closing quote is not found. However
// doesn't throw, just returns an empty string if there is nothing left.
std::string
getPossiblyQuotedWord(std::string const &s, std::string::const_iterator &i)
{
    auto const startPos = i - s.begin() + 1;

    std::string::const_iterator const end = s.end();
    skipWhiteSpace(i, end);

    if (i == end)
        return {};

    char quote = '\0';
    if (*i == '"')
    {
        quote = '"';
    }
    else if (*i == '\'')
    {
        quote = '\'';
    }

    std::string word;

    if (quote != '\0')
    {
        for (;;)
        {
            if (++i == end)
            {
                std::ostringstream os;
                os << "Expected closing quote '" << quote << "' "
                      "matching opening quote at position "
                   << startPos
                   << " not found before the end of the string "
                      "in the connection string \""
                   << s << "\".";

                throw soci_error(os.str());
            }

            if (*i == quote)
            {
                ++i;
                break;
            }

            word += *i;
        }
    }
    else // Not quoted.
    {
        for (; i != end; ++i)
        {
            if (isSpace(i))
                break;

            word += *i;
        }
    }

    return word;
}

} // namespace anonymous

void connection_parameters::extract_options_from_space_separated_string()
{
    constexpr char delim = '=';

    std::string::const_iterator const end = connectString_.end();
    for (std::string::const_iterator i = connectString_.begin(); ; )
    {
        skipWhiteSpace(i, end);

        // Anything until the delimiter or space is the name.
        std::string name;
        std::string value;
        for (;;)
        {
            if (i == end || isSpace(i))
                break;

            if (*i == delim)
            {
                if (name.empty())
                {
                    std::ostringstream os;
                    os << "Unexpected '"
                       << delim
                       << "' without a name at position "
                       << (i - connectString_.begin() + 1)
                       << " in the connection string \""
                       << connectString_
                       << "\".";

                    throw soci_error(os.str());
                }

                ++i;    // Skip the delimiter itself.

                // And get the option value which follows it.
                value = getPossiblyQuotedWord(connectString_, i);
                break;
            }

            name += *i++;
        }

        if (name.empty())
        {
            // We've reached the end of the string and there is nothing left.
            break;
        }

        // Note that value may be empty here, we intentionally allow specifying
        // options without values, e.g. just "switch" instead of "switch=1".
        options_[name] = value;
    }
}

std::string
connection_parameters::build_string_from_options(char quote) const
{
    std::string res;

    for (auto const& option : options_)
    {
        if (!res.empty())
        {
            res += ' ';
        }

        res += option.first;
        res += '=';

        // Quote the value if it contains spaces or the quote character itself.
        auto const& value = option.second;
        if (value.empty() ||
                value.find(' ') != std::string::npos ||
                    value.find(quote) != std::string::npos)
        {
            res += quote;

            for (char c : value)
            {
                if (c == quote)
                {
                    res += '\\';
                }

                res += c;
            }

            res += quote;
        }
        else
        {
            res += value;
        }
    }

    return res;
}

} // namespace soci
