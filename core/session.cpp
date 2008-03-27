//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "session.h"
#include "soci-backend.h"
#include "backend-loader.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

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
	throw soci_error("No backend name found in " + connectString);
    }

    backendName = connectString.substr(0, p);
    connectionParameters = connectString.substr(p + protocolSeparator.size());
}

void ensureConnected(session_backend *backEnd)
{
    if (backEnd == NULL)
    {
        throw soci_error("Session is not connected.");
    }
}

} // namespace anonymous

session::session()
    : once(this), prepare(this), logStream_(NULL),
      lastFactory_(NULL), backEnd_(NULL)
{
}

session::session(backend_factory const & factory,
    std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL),
      lastFactory_(&factory), lastConnectString_(connectString)
{
    backEnd_ = factory.make_session(connectString);
}

session::session(std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL)
{
    std::string backendName;
    std::string connectionParameters;

    parseConnectString(connectString, backendName, connectionParameters);

    backend_factory const & factory = dynamic_backends::get(backendName);

    lastFactory_ = &factory;
    lastConnectString_ = connectionParameters;

    backEnd_ = factory.make_session(connectionParameters);
}

session::~session()
{
    delete backEnd_;
}

void session::open(backend_factory const &factory,
    std::string const & connectString)
{
    if (backEnd_ != NULL)
    {
        throw soci_error("Cannot open already connected session.");
    }

    backEnd_ = factory.make_session(connectString);
    lastFactory_ = &factory;
    lastConnectString_ = connectString;
}

void session::open(std::string const & connectString)
{
    if (backEnd_ != NULL)
    {
        throw soci_error("Cannot open already connected session.");
    }

    std::string backendName;
    std::string connectionParameters;

    parseConnectString(connectString, backendName, connectionParameters);

    backend_factory const & factory = dynamic_backends::get(backendName);

    backEnd_ = factory.make_session(connectionParameters);
    lastFactory_ = &factory;
    lastConnectString_ = connectionParameters;
}

void session::close()
{
    delete backEnd_;
    backEnd_ = NULL;
}

void session::reconnect()
{
    if (lastFactory_ == NULL)
    {
        throw soci_error("Cannot reconnect without previous connection.");
    }

    if (backEnd_ != NULL)
    {
        close();
    }

    backEnd_ = lastFactory_->make_session(lastConnectString_);
}

void session::begin()
{
    backEnd_->begin();
}

void session::commit()
{
    backEnd_->commit();
}

void session::rollback()
{
    backEnd_->rollback();
}

std::ostringstream & session::get_query_stream()
{
    return query_stream_;
}

void session::set_log_stream(std::ostream *s)
{
    logStream_ = s;
}

std::ostream * session::get_log_stream() const
{
    return logStream_;
}

void session::log_query(std::string const &query)
{
    if (logStream_ != NULL)
    {
        *logStream_ << query << '\n';
    }

    lastQuery_ = query;
}

std::string session::get_last_query() const
{
    return lastQuery_;
}

statement_backend * session::make_statement_backend()
{
    ensureConnected(backEnd_);

    return backEnd_->make_statement_backend();
}

rowid_backend * session::make_rowid_backend()
{
    ensureConnected(backEnd_);

    return backEnd_->make_rowid_backend();
}

blob_backend * session::make_blob_backend()
{
    ensureConnected(backEnd_);

    return backEnd_->make_blob_backend();
}
