//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/blob.h"
#include "soci/blob-exchange.h"
#include "soci/statement.h"
#include "soci/session.h"
#include "soci/into-type.h"
#include "soci/use-type.h"
#include "soci/values.h"
#include "soci-compiler.h"
#include "soci-ssize.h"
#include "soci/log-context.h"
#include <ctime>
#include <cctype>
#include <cstdint>
#include <string>

#include <fmt/format.h>

using namespace soci;
using namespace soci::details;


std::string get_name(const details::use_type_base &param, std::size_t position,
        const statement_backend *backend)
{
    // Use the name specified in the "use()" call if any,
    // otherwise get the name of the matching parameter from
    // the query itself, as parsed by the backend.
    std::string name = param.get_name();
    if (backend && name.empty())
        name = backend->get_parameter_name(static_cast<int>(position));

    return name.empty() ? std::to_string(position + 1) : name;
}


statement_impl::statement_impl(session & s)
    : session_(s), refCount_(1), row_(nullptr),
      fetchSize_(1), initialFetchSize_(1),
      alreadyDescribed_(false)
{
    backEnd_ = s.make_statement_backend();
}

statement_impl::statement_impl(prepare_temp_type const & prep)
    : session_(prep.get_prepare_info()->session_),
      refCount_(1), row_(nullptr), fetchSize_(1), alreadyDescribed_(false)
{
    backEnd_ = session_.make_statement_backend();

    ref_counted_prepare_info * prepInfo = prep.get_prepare_info();

    // take all bind/define info
    intos_.swap(prepInfo->intos_);
    uses_.swap(prepInfo->uses_);

    // allocate handle
    alloc();

    // prepare the statement
    query_ = prepInfo->get_query();
    try
    {
        prepare(query_);
    }
    catch(...)
    {
        clean_up();
        throw;
    }

    define_and_bind();
}

statement_impl::~statement_impl()
{
    clean_up();
}

void statement_impl::alloc()
{
    backEnd_->alloc();
}

void statement_impl::bind(values & values)
{
    std::size_t cnt = 0;

    try
    {
        for (std::vector<details::standard_use_type*>::iterator it =
            values.uses_.begin(); it != values.uses_.end(); ++it)
        {
            // only bind those variables which are:
            // - either named and actually referenced in the statement,
            // - or positional

            std::string const& useName = (*it)->get_name();
            if (useName.empty())
            {
                // positional use element

                int position = isize(uses_);
                (*it)->bind(*this, position);
                uses_.push_back(*it);
                indicators_.push_back(values.indicators_[cnt]);
            }
            else
            {
                // named use element - check if it is used
                std::string const placeholder = ":" + useName;

                std::size_t pos = query_.find(placeholder);
                while (pos != std::string::npos)
                {
                    // Retrieve next char after placeholder
                    // make sure we do not go out of range on the string
                    const char nextChar = (pos + placeholder.size()) < query_.size() ?
                                          query_[pos + placeholder.size()] : '\0';

                    if (std::isalnum(nextChar))
                    {
                        // We got a partial match only,
                        // keep looking for the placeholder
                        pos = query_.find(placeholder, pos + placeholder.size());
                    }
                    else
                    {
                        int position = isize(uses_);
                        (*it)->bind(*this, position);
                        uses_.push_back(*it);
                        indicators_.push_back(values.indicators_[cnt]);
                        // Ok we found it, done
                        break;
                    }
                }
                // In case we couldn't find the placeholder
                if (pos == std::string::npos)
                {
                    values.add_unused(*it, values.indicators_[cnt]);
                }
            }

            cnt++;
        }
    }
    catch (...)
    {
        for (std::size_t i = ++cnt; i != values.uses_.size(); ++i)
        {
            values.add_unused(values.uses_[i], values.indicators_[i]);
        }

        rethrow_current_exception_with_context("binding parameters of");
    }
}

void statement_impl::bind_clean_up()
{
    // deallocate all bind and define objects
    std::size_t const isize = intos_.size();
    for (std::size_t i = isize; i != 0; --i)
    {
        intos_[i - 1]->clean_up();
        delete intos_[i - 1];
        intos_.resize(i - 1);
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = ifrsize; i != 0; --i)
    {
        intosForRow_[i - 1]->clean_up();
        delete intosForRow_[i - 1];
        intosForRow_.resize(i - 1);
    }

    std::size_t const usize = uses_.size();
    for (std::size_t i = usize; i != 0; --i)
    {
        uses_[i - 1]->clean_up();
        delete uses_[i - 1];
        uses_.resize(i - 1);
    }

    std::size_t const indsize = indicators_.size();
    for (std::size_t i = 0; i != indsize; ++i)
    {
        delete indicators_[i];
        indicators_[i] = nullptr;
    }
    indicators_.clear();

    row_ = nullptr;
    alreadyDescribed_ = false;
}

void statement_impl::clean_up()
{
    bind_clean_up();
    if (backEnd_ != nullptr)
    {
        backEnd_->clean_up();
        delete backEnd_;
        backEnd_ = nullptr;
    }
}

void statement_impl::prepare(std::string const & query,
    statement_type eType)
{
    try
    {
        query_ = query;
        session_.log_query(query);

        backEnd_->prepare(query, eType);
    }
    catch (...)
    {
        rethrow_current_exception_with_context("preparing");
    }
}

void statement_impl::define_and_bind()
{
    const char* context = "defining output parameters";
    try
    {
      int definePosition = 1;
      std::size_t const isize = intos_.size();
      for (std::size_t i = 0; i != isize; ++i)
      {
          intos_[i]->define(*this, definePosition);
      }

      // if there are some implicit into elements
      // injected by the row description process,
      // they should be defined in the later phase,
      // starting at the position where the above loop finished
      definePositionForRow_ = definePosition;

      context = "binding input parameters";

      int bindPosition = 1;
      std::size_t const usize = uses_.size();
      for (std::size_t i = 0; i != usize; ++i)
      {
          uses_[i]->bind(*this, bindPosition);
      }
    }
    catch (...)
    {
        rethrow_current_exception_with_context(context);
    }
}

void statement_impl::undefine_and_bind()
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = isize; i != 0; --i)
    {
        intos_[i - 1]->clean_up();
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = ifrsize; i != 0; --i)
    {
        intosForRow_[i - 1]->clean_up();
    }

    std::size_t const usize = uses_.size();
    for (std::size_t i = usize; i != 0; --i)
    {
        uses_[i - 1]->clean_up();
    }
}

bool statement_impl::execute(bool withDataExchange)
{
    try
    {
        initialFetchSize_ = intos_size();

        if (intos_.empty() == false && initialFetchSize_ == 0)
        {
            // this can happen only with into-vectors elements
            // and is not allowed when calling execute
            throw soci_error("Vectors of size 0 are not allowed.");
        }

        fetchSize_ = initialFetchSize_;

        // pre-use should be executed before inspecting the sizes of use
        // elements, as they can be resized in type conversion routines

        pre_use();

        std::size_t const bindSize = uses_size();

        if (bindSize > 1 && fetchSize_ > 1)
        {
            throw soci_error(
                 "Bulk insert/update and bulk select not allowed in same query");
        }

        // looks like a hack and it is - row description should happen
        // *after* the use elements were completely prepared
        // and *before* the into elements are touched, so that the row
        // description process can inject more into elements for
        // implicit data exchange
        if (row_ != nullptr && alreadyDescribed_ == false)
        {
            describe();
        }

        int num = 0;
        if (withDataExchange)
        {
            num = 1;

            pre_fetch();

            if (static_cast<int>(fetchSize_) > num)
            {
                num = static_cast<int>(fetchSize_);
            }
            if (static_cast<int>(bindSize) > num)
            {
                num = static_cast<int>(bindSize);
            }
        }

        pre_exec(num);

        statement_backend::exec_fetch_result res = backEnd_->execute(num);

        // another hack related to description: the first call to describe()
        // above may not have done anything if we didn't have the correct
        // number of columns before calling execute() as happens with at least
        // the ODBC backend for some complex queries (see #1151), so call it
        // again in this case
        if (row_ != nullptr && alreadyDescribed_ == false)
        {
            describe();
        }

        bool gotData = false;

        if (res == statement_backend::ef_success)
        {
            // the "success" means that the statement executed correctly
            // and for select statement this also means that some rows were read

            if (num > 0)
            {
                gotData = true;

                // ensure into vectors have correct size
                resize_intos(static_cast<std::size_t>(num));
            }
        }
        else // res == ef_no_data
        {
            // the "no data" means that the end-of-rowset condition was hit
            // but still some rows might have been read (the last bunch of rows)
            // it can also mean that the statement did not produce any results

            gotData = fetchSize_ > 1 ? resize_intos() : false;
        }

        if (num > 0)
        {
            post_fetch(gotData, false);
        }

        post_use(gotData);

        session_.set_got_data(gotData);
        return gotData;
    }
    catch (...)
    {
        rethrow_current_exception_with_context("executing");
    }
}

long long statement_impl::get_affected_rows()
{
    try
    {
        return backEnd_->get_affected_rows();
    }
    catch (...)
    {
        rethrow_current_exception_with_context("getting the number of rows affected by");
    }
}

bool statement_impl::fetch()
{
    try
    {
        if (fetchSize_ == 0)
        {
            truncate_intos();
            session_.set_got_data(false);
            return false;
        }

        bool gotData = false;

        // vectors might have been resized between fetches
        std::size_t const newFetchSize = intos_size();
        if (newFetchSize > initialFetchSize_)
        {
            // this is not allowed, because most likely caused reallocation
            // of the vector - this would require complete re-bind

            throw soci_error(
                "Increasing the size of the output vector is not supported.");
        }
        else if (newFetchSize == 0)
        {
            session_.set_got_data(false);
            return false;
        }
        else
        {
            // the output vector was downsized or remains the same as before
            fetchSize_ = newFetchSize;
        }

        statement_backend::exec_fetch_result const res = backEnd_->fetch(static_cast<int>(fetchSize_));
        if (res == statement_backend::ef_success)
        {
            // the "success" means that some number of rows was read
            // and that it is not yet the end-of-rowset (there are more rows)

            gotData = true;

            // ensure into vectors have correct size
            resize_intos(fetchSize_);
        }
        else // res == ef_no_data
        {
            // end-of-rowset condition

            if (fetchSize_ > 1)
            {
                // but still the last bunch of rows might have been read
                gotData = resize_intos();
                fetchSize_ = 0;
            }
            else
            {
                truncate_intos();
                gotData = false;
            }
        }

        post_fetch(gotData, true);
        session_.set_got_data(gotData);
        return gotData;
    }
    catch (...)
    {
        rethrow_current_exception_with_context("fetching data from");
    }
}

std::size_t statement_impl::intos_size()
{
    // this function does not need to take into account intosForRow_ elements,
    // since their sizes are always 1 (which is the same and the primary
    // into(row) element, which has injected them)

    std::size_t intos_size = 0;
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        if (i==0)
        {
            intos_size = intos_[i]->size();
        }
        else if (intos_size != intos_[i]->size())
        {
            throw soci_error(fmt::format("Bind variable size mismatch (into[{}] has size {}, into[0] has size {})",
                                         i, intos_[i]->size(), intos_size));
        }
    }
    return intos_size;
}

std::size_t statement_impl::uses_size()
{
    std::size_t usesSize = 0;
    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        if (i==0)
        {
            usesSize = uses_[i]->size();
            if (usesSize == 0)
            {
                 // this can happen only for vectors
                 throw soci_error("Vectors of size 0 are not allowed.");
            }
        }
        else if (usesSize != uses_[i]->size())
        {
            throw soci_error(fmt::format("Bind variable size mismatch (use[{}] has size {}, use[0] has size {})",
                                         i, uses_[i]->size(), usesSize));
        }
    }
    return usesSize;
}

bool statement_impl::resize_intos(std::size_t upperBound)
{
    // this function does not need to take into account the intosForRow_
    // elements, since they are never used for bulk operations

    int rows = backEnd_->get_number_of_rows();
    if (rows < 0)
    {
        rows = 0;
    }
    if (upperBound != 0 && upperBound < static_cast<std::size_t>(rows))
    {
        rows = static_cast<int>(upperBound);
    }

    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->resize((std::size_t)rows);
    }

    return rows > 0 ? true : false;
}

void statement_impl::truncate_intos()
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->resize(0);
    }
}

void statement_impl::pre_exec(int num)
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->pre_exec(num);
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = 0; i != ifrsize; ++i)
    {
        intosForRow_[i]->pre_exec(num);
    }

    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        uses_[i]->pre_exec(num);
    }
}

void statement_impl::pre_fetch()
{
    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intos_[i]->pre_fetch();
    }

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = 0; i != ifrsize; ++i)
    {
        intosForRow_[i]->pre_fetch();
    }
}

void statement_impl::do_add_query_parameters()
{
    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        std::string name = get_name(*uses_[i], i, backEnd_);
        std::stringstream value;
        uses_[i]->dump_value(value, backEnd_->get_row_to_dump());
        session_.add_query_parameter(std::move(name), value.str());
    }
}

void statement_impl::pre_use()
{
    session_.clear_query_parameters();

    std::size_t const usize = uses_.size();
    for (std::size_t i = 0; i != usize; ++i)
    {
        uses_[i]->pre_use();
    }

    if (session_.get_query_context_logging_mode() == log_context::always)
    {
        do_add_query_parameters();
    }
}

void statement_impl::post_fetch(bool gotData, bool calledFromFetch)
{
    // first iterate over intosForRow_ elements, since the Row element
    // (which is among the intos_ elements) might depend on the
    // values of those implicitly injected elements

    std::size_t const ifrsize = intosForRow_.size();
    for (std::size_t i = 0; i != ifrsize; ++i)
    {
        intosForRow_[i]->post_fetch(gotData, calledFromFetch);
    }

    std::size_t const isize = intos_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        try
        {
            intos_[i]->post_fetch(gotData, calledFromFetch);
        }
        catch (soci_error& e)
        {
            // Provide the parameter number in the error message as the
            // exceptions thrown by the backend only say what went wrong, but
            // not where.
            e.add_context(fmt::format("for the parameter number {}", i + 1));

            throw;
        }
    }
}

void statement_impl::post_use(bool gotData)
{
    // iterate in reverse order here in case the first item
    // is an UseType<Values> (since it depends on the other UseTypes)
    for (std::size_t i = uses_.size(); i != 0; --i)
    {
        uses_[i-1]->post_use(gotData);
    }
}

namespace soci
{
namespace details
{

// Map data_types to stock types for dynamic result set support

template<>
void statement_impl::bind_into<db_string>()
{
    into_row<std::string>();
}

template<>
void statement_impl::bind_into<db_wstring>()
{
    into_row<std::wstring>();
}

template<>
void statement_impl::bind_into<db_double>()
{
    into_row<double>();
}

template<>
void statement_impl::bind_into<db_int8>()
{
    into_row<int8_t>();
}

template<>
void statement_impl::bind_into<db_uint8>()
{
    into_row<uint8_t>();
}

template<>
void statement_impl::bind_into<db_int16>()
{
    into_row<int16_t>();
}

template<>
void statement_impl::bind_into<db_uint16>()
{
    into_row<uint16_t>();
}

template<>
void statement_impl::bind_into<db_int32>()
{
    into_row<int32_t>();
}

template<>
void statement_impl::bind_into<db_uint32>()
{
    into_row<uint32_t>();
}

template<>
void statement_impl::bind_into<db_int64>()
{
    into_row<int64_t>();
}

template<>
void statement_impl::bind_into<db_uint64>()
{
    into_row<uint64_t>();
}

template<>
void statement_impl::bind_into<db_date>()
{
    into_row<std::tm>();
}

template<>
void statement_impl::bind_into<db_blob>()
{
    into_row<blob>();
}

void statement_impl::describe()
{
    row_->clean_up();

    int const numcols = backEnd_->prepare_for_describe();
    if (!numcols)
    {
        // Return without setting alreadyDescribed_ to true, we'll be called
        // again in this case.
        return;
    }

    for (int i = 1; i <= numcols; ++i)
    {
        db_type dbtype;
        std::string columnName;

        backEnd_->describe_column(i, dbtype, columnName);

        column_properties props;
        props.set_name(columnName);
        props.set_db_type(dbtype);
        props.set_data_type(backEnd_->to_data_type(dbtype));

        switch (backEnd_->exchange_dbtype_for(dbtype))
        {
        case db_string:
        case db_xml:
            bind_into<db_string>();
            break;
        case db_wstring:
            bind_into<db_wstring>();
            break;
        case db_blob:
            bind_into<db_blob>();
            break;
        case db_double:
            bind_into<db_double>();
            break;
        case db_int8:
            bind_into<db_int8>();
            break;
        case db_uint8:
            bind_into<db_uint8>();
            break;
        case db_int16:
            bind_into<db_int16>();
            break;
        case db_uint16:
            bind_into<db_uint16>();
            break;
        case db_int32:
            bind_into<db_int32>();
            break;
        case db_uint32:
            bind_into<db_uint32>();
            break;
        case db_int64:
            bind_into<db_int64>();
            break;
        case db_uint64:
            bind_into<db_uint64>();
            break;
        case db_date:
            bind_into<db_date>();
            break;
        default:
            throw soci_error(fmt::format("db column type {} not supported for dynamic selects\n",
                             fmt::underlying(dbtype)));
        }
        row_->add_properties(props);
    }

    alreadyDescribed_ = true;

    // Calling bind_into() above could have added row into elements, so
    // initialize them.
    std::size_t const isize = intosForRow_.size();
    for (std::size_t i = 0; i != isize; ++i)
    {
        intosForRow_[i]->define(*this, definePositionForRow_);
    }
}

} // namespace details
} // namespace soci

void statement_impl::set_row(row * r)
{
    if (row_ != nullptr)
    {
        throw soci_error(
            "Only one Row element allowed in a single statement.");
    }

    row_ = r;
    row_->uppercase_column_names(session_.get_uppercase_column_names());
}

std::string statement_impl::rewrite_for_procedure_call(std::string const & query)
{
    return backEnd_->rewrite_for_procedure_call(query);
}

void statement_impl::inc_ref()
{
    ++refCount_;
}

void statement_impl::dec_ref()
{
    if (--refCount_ == 0)
    {
        delete this;
    }
}

standard_into_type_backend *
statement_impl::make_into_type_backend()
{
    backEnd_->hasIntoElements_ = true;

    return backEnd_->make_into_type_backend();
}

standard_use_type_backend *
statement_impl::make_use_type_backend()
{
    backEnd_->hasUseElements_ = true;

    return backEnd_->make_use_type_backend();
}

vector_into_type_backend *
statement_impl::make_vector_into_type_backend()
{
    backEnd_->hasVectorIntoElements_ = true;

    return backEnd_->make_vector_into_type_backend();
}

vector_use_type_backend *
statement_impl::make_vector_use_type_backend()
{
    backEnd_->hasVectorUseElements_ = true;

    return backEnd_->make_vector_use_type_backend();
}

[[noreturn]]
void
statement_impl::rethrow_current_exception_with_context(char const* operation)
{
    try
    {
        throw;
    }
    catch (soci_error& e)
    {
        if (!query_.empty())
        {
            std::string ctx = fmt::format("while {} \"{}\"", operation, query_);

            if (!uses_.empty() && session_.get_query_context_logging_mode() != log_context::never)
            {
                // We have to clear previously cached query parameters as different backends behave differently
                // in whether they error before or after the caching has happened. Therefore, we can't rely on
                // the parameters to be or to not be cached at the point of error in a cross-backend way (that
                // works for all kinds of errors).
                session_.clear_query_parameters();

                do_add_query_parameters();

                ctx += fmt::format(" with {}", session_.get_last_query_context());
            }

            e.add_context(ctx);
        }

        throw;
    }
}

template<>
void statement_impl::into_row<blob>()
{
    blob * b = new blob(session_);
    indicator * ind = new indicator(i_ok);
    row_->add_holder(b, ind);
    exchange_for_row(into(*b, *ind));
}
