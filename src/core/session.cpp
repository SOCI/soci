//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "soci/session.h"
#include "soci/connection-parameters.h"
#include "soci/connection-pool.h"
#include "soci/soci-backend.h"
#include "soci/query_transformation.h"
#include "soci/log-context.h"

using namespace soci;
using namespace soci::details;

namespace soci
{
    struct schema_table_name
    {
        std::string schema;
        std::string table;
        soci::indicator ind;
    };
} // soci

void ensureConnected(session_backend * backEnd)
{
    if (backEnd == NULL)
    {
        throw soci_error("Session is not connected.");
    }
}

// Standard logger class used by default.
class standard_logger_impl : public logger_impl
{
public:
    standard_logger_impl()
    {
        logStream_ = NULL;
    }

    virtual void start_query(std::string const & query) override
    {
        logger_impl::start_query(query);

        if (logStream_ != NULL)
        {
            *logStream_ << query << '\n';
        }

        lastQuery_ = query;
    }

    virtual void set_stream(std::ostream * s) override
    {
        logStream_ = s;
    }

    virtual std::ostream * get_stream() const override
    {
        return logStream_;
    }

    virtual std::string get_last_query() const override
    {
        return lastQuery_;
    }

private:
    virtual logger_impl* do_clone() const override
    {
        return new standard_logger_impl;
    }

    std::ostream * logStream_;
    std::string lastQuery_;
};

session::session()
    : once(this), prepare(this),
      logger_(new standard_logger_impl)
{
}

session::session(connection_parameters const & parameters)
    : once(this), prepare(this),
      logger_(new standard_logger_impl),
      lastConnectParameters_(parameters)
{
    open(lastConnectParameters_);
}

session::session(backend_factory const & factory,
    std::string const & connectString)
    : once(this), prepare(this),
      logger_(new standard_logger_impl),
      lastConnectParameters_(factory, connectString)
{
    open(lastConnectParameters_);
}

session::session(std::string const & backendName,
    std::string const & connectString)
    : once(this), prepare(this),
      logger_(new standard_logger_impl),
      lastConnectParameters_(backendName, connectString)
{
    open(lastConnectParameters_);
}

session::session(std::string const & connectString)
    : once(this), prepare(this),
      logger_(new standard_logger_impl),
      lastConnectParameters_(connectString)
{
    open(lastConnectParameters_);
}

session::session(connection_pool & pool)
    : logger_(new standard_logger_impl),
      isFromPool_(true), pool_(&pool)
{
    poolPosition_ = pool.lease();
    session & pooledSession = pool.at(poolPosition_);

    once.set_session(&pooledSession);
    prepare.set_session(&pooledSession);
    backEnd_ = pooledSession.get_backend();
}

session::session(session && other)
    : once(std::move(other.once)),
      prepare(std::move(other.prepare)),
      query_stream_(std::move(other.query_stream_)),
      query_transformation_(std::move(other.query_transformation_)),
      logger_(std::move(other.logger_)),
      lastConnectParameters_(std::move(other.lastConnectParameters_)),
      uppercaseColumnNames_(std::move(other.uppercaseColumnNames_)),
      backEnd_(std::move(other.backEnd_)),
      gotData_(std::move(other.gotData_)),
      isFromPool_(std::move(other.isFromPool_)),
      poolPosition_(std::move(other.poolPosition_)),
      pool_(std::move(other.pool_))
{
    if (!isFromPool_)
    {
        // If 'other' session was from a pool, 'once' and 'prepare'
        // will have been already setup to point at the pooled session above.
        // Otherwise reset them to reference 'this'.
        once.set_session(this);
        prepare.set_session(this);
    }

    other.reset_after_move ();
}

session& session::operator=(session && other)
{
    if (this != &other)
    {
        if (isFromPool_)
        {
            pool_->give_back(poolPosition_);
        }
        else if (backEnd_ != other.backEnd_)
        {
            delete backEnd_;
        }

        if (other.isFromPool_)
        {
            // If 'other' session was from a pool, 'once' and 'prepare'
            // will have been already setup reference the pooled session above.
            // Otherwise leave them alone and let them continue referencing 'this'.
            once = std::move(other.once);
            prepare = std::move(other.prepare);
        }

        query_stream_ = std::move(other.query_stream_);
        query_transformation_ = std::move(other.query_transformation_);
        logger_ = std::move(other.logger_);
        lastConnectParameters_ = std::move(other.lastConnectParameters_);
        uppercaseColumnNames_ = std::move(other.uppercaseColumnNames_);
        backEnd_ = std::move(other.backEnd_);
        gotData_ = std::move(other.gotData_);
        isFromPool_ = std::move(other.isFromPool_);
        poolPosition_ = std::move(other.poolPosition_);
        pool_ = std::move(other.pool_);

        other.reset_after_move();
    }

    return *this;
}

void session::reset_after_move()
{
    backEnd_ = nullptr;
    gotData_ = false;
    isFromPool_ = false;
    poolPosition_ = 0;
    pool_ = nullptr;
}

schema_table_name& session::alloc_schema_table_name(const std::string & tableName)
{
    std::size_t dot_pos = tableName.find( '.' );
    schema_table_name stn;

    if (dot_pos == std::string::npos)
    {
        stn.ind   = soci::i_null;
        stn.table = tableName;
    }
    else
    {
        stn.ind    = soci::i_ok;
        stn.schema = tableName.substr(0, dot_pos);
        stn.table  = tableName.substr(dot_pos + 1);
    }

    // Store the memory at the front of the list of schema_table_names.
    // This memory is stored to allow for the schema, table, and ind
    // parameters to be stored for the lifetime of the prepare_temp_type
    // created in prepare_column_descriptions.
    // These will remain in memory until the session object is deleted.
    schema_table_name_.push_front(stn);

    return schema_table_name_.front();
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

void session::open(connection_parameters const & parameters)
{
    if (isFromPool_)
    {
        session & pooledSession = pool_->at(poolPosition_);
        pooledSession.open(parameters);
        backEnd_ = pooledSession.get_backend();
    }
    else
    {
        if (backEnd_ != NULL)
        {
            throw soci_error("Cannot open already connected session.");
        }

        backend_factory const * const factory = parameters.get_factory();
        if (factory == NULL)
        {
            throw soci_error("Cannot connect without a valid backend.");
        }

        backEnd_ = factory->make_session(parameters);
        lastConnectParameters_ = parameters;
    }
}

void session::open(backend_factory const & factory,
    std::string const & connectString)
{
    open(connection_parameters(factory, connectString));
}

void session::open(std::string const & backendName,
    std::string const & connectString)
{
    open(connection_parameters(backendName, connectString));
}

void session::open(std::string const & connectString)
{
    open(connection_parameters(connectString));
}

void session::close()
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).close();
        backEnd_ = NULL;
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
        session & pooledSession = pool_->at(poolPosition_);
        pooledSession.reconnect();
        backEnd_ = pooledSession.get_backend();
    }
    else
    {
        backend_factory const * const lastFactory = lastConnectParameters_.get_factory();
        if (lastFactory == NULL)
        {
            throw soci_error("Cannot reconnect without previous connection.");
        }

        if (backEnd_ != NULL)
        {
            close();
        }

        // Indicate that we're reconnecting using a special parameter which can
        // be used by some backends (currently only ODBC) that interactive
        // prompts should be suppressed, as they would be unexpected during
        // reconnection, which may happen automatically and not in the result
        // of a user action.
        connection_parameters reconnectParameters(lastConnectParameters_);
        reconnectParameters.set_option(option_reconnect, option_true);
        backEnd_ = lastFactory->make_session(reconnectParameters);
    }
}

bool session::is_connected() const noexcept
{
    try
    {
        return backEnd_ && backEnd_->is_connected();
    }
    catch (...)
    {
        // We must not throw from here, so just handle any exception as an
        // indication that the database connection is not available any longer.
        return false;
    }
}

void session::begin()
{
    ensureConnected(backEnd_);

    backEnd_->begin();
}

void session::commit()
{
    ensureConnected(backEnd_);

    backEnd_->commit();
}

void session::rollback()
{
    ensureConnected(backEnd_);

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

std::string session::get_query() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_query();
    }
    else
    {
        // preserve logical constness of get_query,
        // stream used as read-only here,
        session* pthis = const_cast<session*>(this);

        // sole place where any user-defined query transformation is applied
        if (query_transformation_)
        {
            return (*query_transformation_)(pthis->get_query_stream().str());
        }
        return pthis->get_query_stream().str();
    }
}


void session::set_query_transformation_(std::unique_ptr<details::query_transformation_function>&& qtf)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).set_query_transformation_(std::move(qtf));
    }
    else
    {
        query_transformation_= std::move(qtf);
    }
}

void session::set_logger(logger const & logger)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).set_logger(logger);
    }
    else
    {
        logger_ = logger;
    }
}

logger const & session::get_logger() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_logger();
    }
    else
    {
        return logger_;
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
        logger_.set_stream(s);
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
        return logger_.get_stream();
    }
}

void session::set_query_context_logging_mode(log_context ctx)
{
    query_ctx_logging_mode_ = ctx;
}

log_context session::get_query_context_logging_mode() const
{
    return query_ctx_logging_mode_;
}

void session::log_query(std::string const & query)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).log_query(query);
    }
    else
    {
        logger_.start_query(query);
    }
}

void session::clear_query_parameters()
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).clear_query_parameters();
    }
    else
    {
        logger_.clear_query_parameters();
    }
}

void session::add_query_parameter(std::string name, std::string value)
{
    if (isFromPool_)
    {
        pool_->at(poolPosition_).add_query_parameter(std::move(name), std::move(value));
    }
    else
    {
        logger_.add_query_parameter(std::move(name), std::move(value));
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
        return logger_.get_last_query();
    }
}

std::string session::get_last_query_context() const
{
    if (isFromPool_)
    {
        return pool_->at(poolPosition_).get_last_query_context();
    }
    else
    {
        return logger_.get_last_query_context();
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

bool session::get_next_sequence_value(std::string const & sequence, long long & value)
{
    ensureConnected(backEnd_);

    return backEnd_->get_next_sequence_value(*this, sequence, value);
}

bool session::get_last_insert_id(std::string const & sequence, long long & value)
{
    ensureConnected(backEnd_);

    return backEnd_->get_last_insert_id(*this, sequence, value);
}

details::once_temp_type session::get_table_names()
{
    ensureConnected(backEnd_);

    return once << backEnd_->get_table_names_query();
}

details::prepare_temp_type session::prepare_table_names()
{
    ensureConnected(backEnd_);

    return prepare << backEnd_->get_table_names_query();
}

details::prepare_temp_type session::prepare_column_descriptions(std::string & table_name)
{
    ensureConnected(backEnd_);

    std::string column_description_query = backEnd_->get_column_descriptions_query();
    if (column_description_query.find(":s") == std::string::npos)
    {
        return prepare << column_description_query, use(table_name, "t");
    }
    else
    {
        schema_table_name& stn = alloc_schema_table_name(table_name);

        return prepare << column_description_query, use(stn.schema, stn.ind, "s"), use(stn.table, "t");
    }
}

ddl_type session::create_table(const std::string & tableName)
{
    ddl_type ddl(*this);

    ddl.create_table(tableName);
    ddl.set_tail(")");

    return ddl;
}

void session::drop_table(const std::string & tableName)
{
    ensureConnected(backEnd_);

    once << backEnd_->drop_table(tableName);
}

void session::truncate_table(const std::string & tableName)
{
    ensureConnected(backEnd_);

    once << backEnd_->truncate_table(tableName);
}

ddl_type session::add_column(const std::string & tableName,
    const std::string & columnName, db_type dt,
    int precision, int scale)
{
    ddl_type ddl(*this);

    ddl.add_column(tableName, columnName, dt, precision, scale);

    return ddl;
}

ddl_type session::alter_column(const std::string & tableName,
    const std::string & columnName, db_type dt,
    int precision, int scale)
{
    ddl_type ddl(*this);

    ddl.alter_column(tableName, columnName, dt, precision, scale);

    return ddl;
}

ddl_type session::drop_column(const std::string & tableName,
    const std::string & columnName)
{
    ddl_type ddl(*this);

    ddl.drop_column(tableName, columnName);

    return ddl;
}

std::string session::empty_blob()
{
    ensureConnected(backEnd_);

    return backEnd_->empty_blob();
}

std::string session::nvl()
{
    ensureConnected(backEnd_);

    return backEnd_->nvl();
}

std::string session::get_dummy_from_table() const
{
    ensureConnected(backEnd_);

    return backEnd_->get_dummy_from_table();
}

std::string session::get_dummy_from_clause() const
{
    std::string clause = get_dummy_from_table();
    if (!clause.empty())
        clause.insert(0, " from ");

    return clause;
}

void session::set_failover_callback(failover_callback & callback)
{
    ensureConnected(backEnd_);

    backEnd_->set_failover_callback(callback, *this);
}

std::string session::get_backend_name() const
{
    ensureConnected(backEnd_);

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
