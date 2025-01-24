# Logging

SOCI provides a flexible, but requiring some effort to use, way to log all
queries done by the library and a more limited but very simple way to do it.

## Simple logging

The following members of the `session` class support the basic logging functionality:

* `void set_log_stream(std::ostream * s);`
* `std::ostream * get_log_stream() const;`
* `std::string get_last_query() const;`
* `std::string get_last_query_context() const;`

The first two functions allow to set the user-provided output stream object for logging.
The `NULL` value, which is the default, means that there is no logging.

An example use might be:

    session sql(oracle, "...");

    ofstream file("my_log.txt");
    sql.set_log_stream(&file);

    // ...

Each statement logs its query string before the preparation step (whether explicit or implicit) and therefore logging is effective whether the query succeeds or not.
Note that each prepared query is logged only once, independent on how many times it is executed.

The `get_last_query` function allows to retrieve the last used query. The associated `get_last_query_context` function allows to obtain a string
representation of the bound values (if any) used in the last query. That is, while `get_last_query` might return something like
`INSERT INTO dummy (val) VALUES (:val)`, `get_last_query_context` will return something like `:val=42` and together they can be used to get a detailed
understanding of what happened in the last query. Cached parameters are cleared at the beginning of each query and are **not** persisted across
multiple queries.

Logging of query parameters is **enabled by default** but can at any time be adjusted to your needs by using the `set_query_context_logging_mode`
function of the `session` object. For instance

    sql.set_query_context_logging_mode(log_context::on_error);

Possible values are

* `log_context::always` - Always cache the bound parameters of queries (the default)
* `log_context::never` - Never cache bound parameters. This also ensures that bound parameters are not part of exception messages. It is therefore
  suitable for queries that bind sensitive information that must not be leaked.
* `log_context::on_error` - Only caches bound parameters in case the query encounters an error. This is intended for cases in which you don't want to
  have the overhead of caching parameters during regular operations but still want this extra information in case of errors.

## Flexible logging using custom loggers

If the above is not enough, it is also possible to log the queries in exactly
the way you want by deriving your own `my_log_impl` class from
`soci::logger_impl` and implementing its pure virtual `start_query()` and
`do_clone()` methods:

    class my_log_impl : public soci::logger_impl
    {
    public:
        virtual void start_query(std::string const & query)
        {
            ... log the given query ...
        }

    private:
        virtual logger_impl* do_clone() const
        {
            return new my_log_impl(...);
        }
    };

Then simply pass a new, heap-allocated instance of this class to the `session`
object:

    soci::session sql(...);
    sql.set_logger(new my_log_impl(...));

and `start_query()` method of the logger will be called for all queries.
