//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "session.h"
#include "connection-pool.h"
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

void ensureConnected(session_backend * backEnd)
{
    if (backEnd == NULL)
    {
        throw soci_error("Session is not connected.");
    }
}

} // namespace anonymous

session::session()
    : once(this), prepare(this), logStream_(NULL),
      lastFactory_(NULL), uppercaseColumnNames_(false), backEnd_(NULL),
      isFromPool_(false), pool_(NULL)
{
}

session::session(backend_factory const & factory,
    std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL),
      lastFactory_(&factory), lastConnectString_(connectString),
      uppercaseColumnNames_(false),
      isFromPool_(false), pool_(NULL)
{
    backEnd_ = factory.make_session(connectString);
}

session::session(std::string const & backendName,
    std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL),
      uppercaseColumnNames_(false),
      isFromPool_(false), pool_(NULL)
{
    backend_factory const & factory = dynamic_backends::get(backendName);

    lastFactory_ = &factory;
    lastConnectString_ = connectString;

    backEnd_ = factory.make_session(connectString);
}

session::session(std::string const & connectString)
    : once(this), prepare(this), logStream_(NULL),
      uppercaseColumnNames_(false),
      isFromPool_(false), pool_(NULL)
{
    std::string backendName;
    std::string connectionParameters;

    parseConnectString(connectString, backendName, connectionParameters);

    backend_factory const & factory = dynamic_backends::get(backendName);

    lastFactory_ = &factory;
    lastConnectString_ = connectionParameters;

    backEnd_ = factory.make_session(connectionParameters);
}

session::session(connection_pool & pool)
    : isFromPool_(true), pool_(&pool)
{
    poolPosition_ = pool.lease();
    session & pooledSession = pool.at(poolPosition_);

    once.set_session(&pooledSession);
    prepare.set_session(&pooledSession);
    backEnd_ = pooledSession.get_backend();
}

session::~session()
{
    if (isFromPool_)
    {
        pool_->give_back(poolPosition_);
    }
    else
    {
        delete backEnd_;
    }
}

void session::open(backend_factory const & factory,
    std::string const & connectString)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).open(factory, connectString);
    }
    else
    {
        if (backEnd_ != NULL)
        {
            throw soci_error("Cannot open already connected session.");
        }

        backEnd_ = factory.make_session(connectString);
        lastFactory_ = &factory;
        lastConnectString_ = connectString;
    }
}

void session::open(std::string const & backendName,
    std::string const & connectString)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).open(backendName, connectString);
    }
    else
    {
        if (backEnd_ != NULL)
        {
            throw soci_error("Cannot open already connected session.");
        }

        backend_factory const & factory = dynamic_backends::get(backendName);

        backEnd_ = factory.make_session(connectString);
        lastFactory_ = &factory;
        lastConnectString_ = connectString;
    }
}

void session::open(std::string const & connectString)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).open(connectString);
    }
    else
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
}

void session::close()
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).close();
    }
    else
    {
        delete backEnd_;
        backEnd_ = NULL;
    }
}

void session::reconnect()
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).reconnect();
    }
    else
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
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_query_stream();
    }
    else
    {
        return query_stream_;
    }
}

void session::set_log_stream(std::ostream * s)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).set_log_stream(s);
    }
    else
    {
        logStream_ = s;
    }
}

std::ostream * session::get_log_stream() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_log_stream();
    }
    else
    {
        return logStream_;
    }
}

void session::log_query(std::string const & query)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).log_query(query);
    }
    else
    {
        if (logStream_ != NULL)
        {
            *logStream_ << query << '\n';
        }

        lastQuery_ = query;
    }
}

std::string session::get_last_query() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_last_query();
    }
    else
    {
        return lastQuery_;
    }
}

void session::set_got_data(bool gotData)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).set_got_data(gotData);
    }
    else
    {
        gotData_ = gotData;
    }
}

bool session::got_data() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).got_data();
    }
    else
    {
        return gotData_;
    }
}

void session::uppercase_column_names(bool forceToUpper)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).uppercase_column_names(forceToUpper);
    }
    else
    {
        uppercaseColumnNames_ = forceToUpper;
    }
}

bool session::get_uppercase_column_names() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_uppercase_column_names();
    }
    else
    {
        return uppercaseColumnNames_;
    }
}

std::string session::get_backend_name() const
{
    return backEnd_->get_backend_name();
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
