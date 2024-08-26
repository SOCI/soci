//
// Copyright (C) 2004-2016 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_SESSION_H_INCLUDED
#define SOCI_SESSION_H_INCLUDED

#include "soci/soci-platform.h"
#include "soci/once-temp-type.h"
#include "soci/query_transformation.h"
#include "soci/connection-parameters.h"
#include "soci/logger.h"

// std
#include <cstddef>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <forward_list>

namespace soci
{
class values;
class backend_factory;
struct schema_table_name;

namespace details
{

class session_backend;
class statement_backend;
class rowid_backend;
class blob_backend;

} // namespace details

class connection_pool;
class failover_callback;

class SOCI_DECL session
{
private:

    void set_query_transformation_(std::unique_ptr<details::query_transformation_function>&& qtf);



public:
    session();
    explicit session(connection_parameters const & parameters);
    session(backend_factory const & factory, std::string const & connectString);
    session(std::string const & backendName, std::string const & connectString);
    explicit session(std::string const & connectString);
    explicit session(connection_pool & pool);

    session(session &&other);
    session &operator=(session &&other);

    ~session();

    void open(connection_parameters const & parameters);
    void open(backend_factory const & factory, std::string const & connectString);
    void open(std::string const & backendName, std::string const & connectString);
    void open(std::string const & connectString);
    void close();
    void reconnect();

    // check if we have a working connection to the database
    bool is_connected() const noexcept;

    void begin();
    void commit();
    void rollback();

    // once and prepare are for syntax sugar only
    details::once_type once;
    details::prepare_type prepare;

    // even more sugar
    template <typename T>
    details::once_temp_type operator<<(T const & t) { return once << t; }

    std::ostringstream & get_query_stream();
    std::string get_query() const;

    template <typename T>
    void set_query_transformation(T callback)
    {
        set_query_transformation_(std::make_unique<details::query_transformation<T>>(callback));
    }

    // Support for custom logging of database operations.

    // Set the custom logger to use.
    void set_logger(logger const & logger);

    // Return the currently used logger, by default, this is an instance of a
    // standard SOCI logger.
    logger const & get_logger() const;


    // support for basic logging (use set_logger() for more control).
    void set_log_stream(std::ostream * s);
    std::ostream * get_log_stream() const;

    void log_query(std::string const & query);
    std::string get_last_query() const;

    void set_got_data(bool gotData);
    bool got_data() const;

    void uppercase_column_names(bool forceToUpper);

    bool get_uppercase_column_names() const;

    // Functions for dealing with sequence/auto-increment values.

    // If true is returned, value is filled with the next value from the given
    // sequence. Otherwise either the sequence is invalid (doesn't exist) or
    // the current backend doesn't support sequences. If you use sequences for
    // automatically generating primary key values, you should use
    // get_last_insert_id() after the insertion in this case.
    bool get_next_sequence_value(std::string const & sequence, long long & value);

    // If true is returned, value is filled with the last auto-generated value
    // for this table (although some backends ignore the table argument and
    // return the last value auto-generated in this session).
    bool get_last_insert_id(std::string const & table, long long & value);

    // Returns once_temp_type for the internally composed query
    // for the list of tables in the current schema.
    // Since this query usually returns multiple results (for multiple tables),
    // it makes sense to bind std::vector<std::string> for the single output field.
    details::once_temp_type get_table_names();

    // Returns prepare_temp_type for the internally composed query
    // for the list of tables in the current schema.
    // Since this is intended for use with statement objects, where results are obtained one row after another,
    // it makes sense to bind std::string for the output field.
    details::prepare_temp_type prepare_table_names();

    // Returns prepare_temp_type for the internally composed query
    // for the list of column descriptions.
    // Since this is intended for use with statement objects, where results are obtained one row after another,
    // it makes sense to bind either std::string for each output field or soci::column_info for the whole row.
    // Note: table_name is a non-const reference to prevent temporary objects,
    // this argument is bound as a regular "use" element. The table_name can consist of both a schema name and
    // a table_name separated by a dot.
    details::prepare_temp_type prepare_column_descriptions(std::string & table_name);

    // Functions for basic portable DDL statements.

    ddl_type create_table(const std::string & tableName);
    void drop_table(const std::string & tableName);
    void truncate_table(const std::string & tableName);
    ddl_type add_column(const std::string & tableName,
        const std::string & columnName, db_type dt,
        int precision = 0, int scale = 0);
    ddl_type alter_column(const std::string & tableName,
        const std::string & columnName, db_type dt,
        int precision = 0, int scale = 0);
    ddl_type drop_column(const std::string & tableName,
        const std::string & columnName);
    std::string empty_blob();
    std::string nvl();

    // And some functions to help with writing portable DML statements.

    // Get the name of the dummy table that needs to be used in the FROM clause
    // of a SELECT statement not operating on any tables, e.g. "dual" for
    // Oracle. The returned string is empty if no such table is needed.
    std::string get_dummy_from_table() const;

    // Returns a possibly empty string that needs to be used as a FROM clause
    // of a SELECT statement not operating on any tables, e.g. " FROM DUAL"
    // (notice the leading space).
    std::string get_dummy_from_clause() const;


    // Sets the failover callback object.
    void set_failover_callback(failover_callback & callback);

    // for diagnostics and advanced users
    // (downcast it to expected back-end session class)
    details::session_backend * get_backend() { return backEnd_; }

    std::string get_backend_name() const;

    // The functions below still work but are deprecated (but we don't give
    // deprecation warnings for them because there is no real harm in using
    // them).
    //
    // Use the overloads taking db_type instead in the new code.
    ddl_type add_column(const std::string & tableName,
        const std::string & columnName, data_type dt,
        int precision = 0, int scale = 0)
    {
        return add_column(tableName, columnName, details::to_db_type(dt),
            precision, scale);
    }
    ddl_type alter_column(const std::string & tableName,
        const std::string & columnName, data_type dt,
        int precision = 0, int scale = 0)
    {
        return alter_column(tableName, columnName, details::to_db_type(dt),
            precision, scale);
    }

    details::statement_backend * make_statement_backend();
    details::rowid_backend * make_rowid_backend();
    details::blob_backend * make_blob_backend();

private:
    SOCI_NOT_COPYABLE(session)

    void reset_after_move();
    schema_table_name& alloc_schema_table_name(const std::string& tableName);

    std::ostringstream query_stream_;
    std::unique_ptr<details::query_transformation_function> query_transformation_;

    logger logger_;

    connection_parameters lastConnectParameters_;

    bool uppercaseColumnNames_;

    details::session_backend * backEnd_;

    bool gotData_;

    bool isFromPool_;
    std::size_t poolPosition_;
    connection_pool * pool_;

    // Storing schema_table_names in a forward list as these are required
    // as persistent input to prepare_temp_type object during their life-
    // span. The prepare_temp_type uses the addresses of the content of the
    // schema_table_name_ thus, a container which doesn't move data is used.
    std::forward_list<schema_table_name> schema_table_name_;
};

} // namespace soci

#endif // SOCI_SESSION_H_INCLUDED
