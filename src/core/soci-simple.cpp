//
// Copyright (C) 2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci-simple.h"
#include "soci/soci.h"
#include "soci-ssize.h"

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <exception>
#include <map>
#include <string>
#include <vector>

using namespace soci;

namespace // unnamed
{

struct session_wrapper
{
    session sql;

    bool is_ok;
    std::string error_message;
};

} // namespace unnamed


SOCI_DECL session_handle soci_create_session(char const * connection_string)
{
    session_wrapper * wrapper = nullptr;
    try
    {
        wrapper = new session_wrapper();
    }
    catch (...)
    {
        return nullptr;
    }

    try
    {
        wrapper->sql.open(connection_string);
        wrapper->is_ok = true;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();
    }

    return wrapper;
}

SOCI_DECL void soci_destroy_session(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    delete wrapper;
}

SOCI_DECL void soci_begin(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    try
    {
        wrapper->sql.begin();
        wrapper->is_ok = true;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();
    }
}

SOCI_DECL void soci_commit(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    try
    {
        wrapper->sql.commit();
        wrapper->is_ok = true;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();
    }
}

SOCI_DECL void soci_rollback(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    try
    {
        wrapper->sql.rollback();
        wrapper->is_ok = true;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();
    }
}

// this will not be needed until dynamic row is exposed
// SOCI_DECL void soci_uppercase_column_names(session_handle s, bool forceToUpper)
// {
//     session_wrapper * wrapper = static_cast<session_wrapper *>(s);
//     wrapper->sql.uppercase_column_names(forceToUpper);
//     wrapper->is_ok = true;
// }

SOCI_DECL int soci_session_state(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);

    return wrapper->is_ok ? 1 : 0;
}

SOCI_DECL char const * soci_session_error_message(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);

    return wrapper->error_message.c_str();
}


// blob
struct blob_wrapper
{
    blob_wrapper(session &sql)
        : blob_(sql), is_ok(true) {}

    blob blob_;
    bool is_ok;
    std::string error_message;
};

blob_wrapper *soci_create_blob_session(soci::session &sql)
{
    blob_wrapper *bw = nullptr;
    try
    {
        bw = new blob_wrapper(sql);
    }
    catch (...)
    {
        delete bw;
        return nullptr;
    }

    return bw;
}

SOCI_DECL blob_handle soci_create_blob(session_handle s)
{
    session_wrapper * session = static_cast<session_wrapper *>(s);
    if (!session->is_ok)
        return nullptr;

    return soci_create_blob_session(session->sql);
}

SOCI_DECL void soci_destroy_blob(blob_handle b)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        delete blob;
    }
    catch (...)
    {}
}

SOCI_DECL int soci_blob_get_len(blob_handle b)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    return static_cast<int>(blob->blob_.get_len());
}

SOCI_DECL int soci_blob_read(blob_handle b, int offset, char *buf, int toRead)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        return static_cast<int>(blob->blob_.read(offset, buf, toRead));
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }
}

SOCI_DECL int soci_blob_read_from_start(blob_handle b, char *buf, int toRead, int offset)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        return static_cast<int>(blob->blob_.read_from_start(buf, toRead, offset));
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }
}

SOCI_DECL int soci_blob_write(blob_handle b, int offset, char const *buf, int toWrite)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        return static_cast<int>(blob->blob_.write(offset, buf, toWrite));
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }
}

SOCI_DECL int soci_blob_write_from_start(blob_handle b, char const *buf, int toWrite, int offset)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        return static_cast<int>(blob->blob_.write_from_start(buf, toWrite, offset));
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }
}

SOCI_DECL int soci_blob_append(blob_handle b, char const *buf, int toWrite)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        return static_cast<int>(blob->blob_.append(buf, toWrite));
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }
}

SOCI_DECL int soci_blob_trim(blob_handle b, int newLen)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    try
    {
        blob->blob_.trim(newLen);
    }
    catch (std::exception &e)
    {
        blob->is_ok = false;
        blob->error_message = e.what();
        return -1;
    }
    catch (...)
    {
        blob->is_ok = false;
        blob->error_message = "unknown exception";
        return -1;
    }

    return 0;
}

SOCI_DECL int soci_blob_state(blob_handle b)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    return (blob->is_ok ? 1 : 0);
}

SOCI_DECL char const * soci_blob_error_message(blob_handle b)
{
    blob_wrapper *blob = static_cast<blob_wrapper *>(b);
    return blob->error_message.c_str();
}

// statement


namespace // unnamed
{

struct statement_wrapper
{
    statement_wrapper(session & _sql)
        : sql(_sql), st(sql), statement_state(clean), into_kind(empty), use_kind(empty),
          next_position(0), is_ok(true) {}

    ~statement_wrapper();

    session  &sql;
    statement st;

    enum state { clean, defining, executing } statement_state;
    enum kind { empty, single, bulk } into_kind, use_kind;

    // into elements
    int next_position;
    std::vector<db_type> into_types; // for both single and bulk
    std::vector<indicator> into_indicators;
    std::map<int, std::string> into_strings;
    std::map<int, std::wstring> into_wstrings;
    std::map<int, int8_t> into_int8;
    std::map<int, uint8_t> into_uint8;
    std::map<int, int16_t> into_int16;
    std::map<int, uint16_t> into_uint16;
    std::map<int, int32_t> into_int32;
    std::map<int, uint32_t> into_uint32;
    std::map<int, int64_t> into_int64;
    std::map<int, uint64_t> into_uint64;
    std::map<int, double> into_doubles;
    std::map<int, std::tm> into_dates;
    std::map<int, blob_wrapper *> into_blob;

    std::vector<std::vector<indicator> > into_indicators_v;
    std::map<int, std::vector<std::string> > into_strings_v;
    std::map<int, std::vector<std::wstring> > into_wstrings_v;
    std::map<int, std::vector<int8_t> > into_int8_v;
    std::map<int, std::vector<uint8_t> > into_uint8_v;
    std::map<int, std::vector<int16_t> > into_int16_v;
    std::map<int, std::vector<uint16_t> > into_uint16_v;
    std::map<int, std::vector<int32_t> > into_int32_v;
    std::map<int, std::vector<uint32_t> > into_uint32_v;
    std::map<int, std::vector<int64_t> > into_int64_v;
    std::map<int, std::vector<uint64_t> > into_uint64_v;
    std::map<int, std::vector<double> > into_doubles_v;
    std::map<int, std::vector<std::tm> > into_dates_v;

    // use elements
    std::map<std::string, indicator> use_indicators;
    std::map<std::string, std::string> use_strings;
    std::map<std::string, std::wstring> use_wstrings;
    std::map<std::string, int8_t> use_int8;
    std::map<std::string, uint8_t> use_uint8;
    std::map<std::string, int16_t> use_int16;
    std::map<std::string, uint16_t> use_uint16;
    std::map<std::string, int32_t> use_int32;
    std::map<std::string, uint32_t> use_uint32;
    std::map<std::string, int64_t> use_int64;
    std::map<std::string, uint64_t> use_uint64;
    std::map<std::string, double> use_doubles;
    std::map<std::string, std::tm> use_dates;
    std::map<std::string, blob_wrapper *> use_blob;

    std::map<std::string, std::vector<indicator> > use_indicators_v;
    std::map<std::string, std::vector<std::string> > use_strings_v;
    std::map<std::string, std::vector<std::wstring> > use_wstrings_v;
    std::map<std::string, std::vector<int8_t> > use_int8_v;
    std::map<std::string, std::vector<uint8_t> > use_uint8_v;
    std::map<std::string, std::vector<int16_t> > use_int16_v;
    std::map<std::string, std::vector<uint16_t> > use_uint16_v;
    std::map<std::string, std::vector<int32_t> > use_int32_v;
    std::map<std::string, std::vector<uint32_t> > use_uint32_v;
    std::map<std::string, std::vector<int64_t> > use_int64_v;
    std::map<std::string, std::vector<uint64_t> > use_uint64_v;
    std::map<std::string, std::vector<double> > use_doubles_v;
    std::map<std::string, std::vector<std::tm> > use_dates_v;

    // format is: "YYYY MM DD hh mm ss", but we make the buffer bigger to
    // avoid gcc -Wformat-truncation warnings as it considers that the output
    // could be up to 72 bytes if the integers had maximal values
    char date_formatted[80];

    bool is_ok;
    std::string error_message;

private:
    SOCI_NOT_COPYABLE(statement_wrapper)
};

statement_wrapper::~statement_wrapper()
{
    for (auto& kv : into_blob)
    {
        soci_destroy_blob(kv.second);
    }

    for (auto& kv : use_blob)
    {
        soci::indicator &ind = use_indicators[kv.first];
        blob_wrapper *&blob = kv.second;
        if (ind == i_null && blob != nullptr)
            soci_destroy_blob(blob);
    }
}

// helper for checking if the attempt was made to add more into/use elements
// after the statement was set for execution
bool cannot_add_elements(statement_wrapper & wrapper, statement_wrapper::kind k, bool into)
{
    if (wrapper.statement_state == statement_wrapper::executing)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Cannot add more data items.";
        return true;
    }

    if (into)
    {
        if (k == statement_wrapper::single && wrapper.into_kind == statement_wrapper::bulk)
        {
            wrapper.is_ok = false;
            wrapper.error_message = "Cannot add single into data items.";
            return true;
        }
        if (k == statement_wrapper::bulk && wrapper.into_kind == statement_wrapper::single)
        {
            wrapper.is_ok = false;
            wrapper.error_message = "Cannot add vector into data items.";
            return true;
        }
    }
    else
    {
        // trying to add use elements
        if (k == statement_wrapper::single && wrapper.use_kind == statement_wrapper::bulk)
        {
            wrapper.is_ok = false;
            wrapper.error_message = "Cannot add single use data items.";
            return true;
        }
        if (k == statement_wrapper::bulk && wrapper.use_kind == statement_wrapper::single)
        {
            wrapper.is_ok = false;
            wrapper.error_message = "Cannot add vector use data items.";
            return true;
        }
    }

    wrapper.is_ok = true;
    return false;
}

// helper for checking if the expected into element exists on the given position
bool position_check_failed(statement_wrapper & wrapper, statement_wrapper::kind k,
    int position, db_type expected_type, char const * type_name)
{
    if (position < 0 || position >= wrapper.next_position)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Invalid position.";
        return true;
    }

    if (wrapper.into_types[position] != expected_type)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "No into ";
        if (k == statement_wrapper::bulk)
        {
            wrapper.error_message += "vector ";
        }
        wrapper.error_message += type_name;
        wrapper.error_message += " element at this position.";
        return true;
    }

    wrapper.is_ok = true;
    return false;
}

// helper for checking if the into element on the given position
// is not null
bool not_null_check_failed(statement_wrapper & wrapper, int position)
{
    if (wrapper.into_indicators[position] == i_null)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Element is null.";
        return true;
    }

    wrapper.is_ok = true;
    return false;
}

// overloaded version for vectors
bool not_null_check_failed(statement_wrapper & wrapper, int position, int index)
{
    if (wrapper.into_indicators_v[position][index] == i_null)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Element is null.";
        return true;
    }

    wrapper.is_ok = true;
    return false;
}

// helper for checking the index value
template <typename T>
bool index_check_failed(std::vector<T> const & v,
    statement_wrapper & wrapper, int index)
{
    if (index < 0 || index >= ssize(v))
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Invalid index.";
        return true;
    }

    wrapper.is_ok = true;
    return false;
}

// helper for checking the uniqueness of the use element's name
bool name_unique_check_failed(statement_wrapper & wrapper,
    statement_wrapper::kind k, char const * name)
{
    bool is_unique;
    if (k == statement_wrapper::single)
    {
        typedef std::map<std::string, indicator>::const_iterator iterator;
        iterator const it = wrapper.use_indicators.find(name);
        is_unique = it == wrapper.use_indicators.end();
    }
    else
    {
        // vector version

        typedef std::map
            <
                std::string,
                std::vector<indicator>
            >::const_iterator iterator;

        iterator const it = wrapper.use_indicators_v.find(name);
        is_unique = it == wrapper.use_indicators_v.end();
    }

    if (is_unique)
    {
        wrapper.is_ok = true;
        return false;
    }
    else
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Name of use element should be unique.";
        return true;
    }
}

// helper for checking if the use element with the given name exists
bool name_exists_check_failed(statement_wrapper & wrapper,
    char const * name, db_type expected_type,
    statement_wrapper::kind k, char const * type_name)
{
    bool name_exists = false;
    if (k == statement_wrapper::single)
    {
        switch (expected_type)
        {
        case db_string:
            {
                typedef std::map
                    <
                        std::string,
                        std::string
                    >::const_iterator iterator;
                iterator const it = wrapper.use_strings.find(name);
                name_exists = (it != wrapper.use_strings.end());
            }
            break;
        case db_wstring:
            {
                typedef std::map
                    <
                        std::string,
                        std::wstring
                    >::const_iterator iterator;
                iterator const it = wrapper.use_wstrings.find(name);
                name_exists = (it != wrapper.use_wstrings.end());
            }
            break;
        case db_int8:
            {
                typedef std::map<std::string, int8_t>::const_iterator iterator;
                iterator const it = wrapper.use_int8.find(name);
                name_exists = (it != wrapper.use_int8.end());
            }
            break;
        case db_uint8:
            {
                typedef std::map<std::string, uint8_t>::const_iterator iterator;
                iterator const it = wrapper.use_uint8.find(name);
                name_exists = (it != wrapper.use_uint8.end());
            }
            break;
        case db_int16:
            {
                typedef std::map<std::string, int16_t>::const_iterator iterator;
                iterator const it = wrapper.use_int16.find(name);
                name_exists = (it != wrapper.use_int16.end());
            }
            break;
        case db_uint16:
            {
                typedef std::map<std::string, uint16_t>::const_iterator iterator;
                iterator const it = wrapper.use_uint16.find(name);
                name_exists = (it != wrapper.use_uint16.end());
            }
            break;
        case db_int32:
            {
                typedef std::map<std::string, int32_t>::const_iterator iterator;
                iterator const it = wrapper.use_int32.find(name);
                name_exists = (it != wrapper.use_int32.end());
            }
            break;
        case db_uint32:
            {
                typedef std::map<std::string, uint32_t>::const_iterator iterator;
                iterator const it = wrapper.use_uint32.find(name);
                name_exists = (it != wrapper.use_uint32.end());
            }
            break;
        case db_int64:
            {
                typedef std::map<std::string, int64_t>::const_iterator
                    iterator;
                iterator const it = wrapper.use_int64.find(name);
                name_exists = (it != wrapper.use_int64.end());
            }
            break;
        case db_uint64:
            {
                typedef std::map<std::string, uint64_t>::const_iterator
                    iterator;
                iterator const it = wrapper.use_uint64.find(name);
                name_exists = (it != wrapper.use_uint64.end());
            }
            break;
        case db_double:
            {
                typedef std::map<std::string, double>::const_iterator iterator;
                iterator const it = wrapper.use_doubles.find(name);
                name_exists = (it != wrapper.use_doubles.end());
            }
            break;
        case db_date:
            {
                typedef std::map<std::string, std::tm>::const_iterator iterator;
                iterator const it = wrapper.use_dates.find(name);
                name_exists = (it != wrapper.use_dates.end());
            }
            break;
        case db_blob:
            {
                typedef std::map<std::string, blob_wrapper *>::const_iterator iterator;
                iterator const it = wrapper.use_blob.find(name);
                name_exists = (it != wrapper.use_blob.end());
            }
        case db_xml:
            // no support for xml
            break;
        }
    }
    else
    {
        // vector version

        switch (expected_type)
        {
        case db_string:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<std::string>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_strings_v.find(name);
                name_exists = (it != wrapper.use_strings_v.end());
            }
            break;
        case db_wstring:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<std::wstring>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_wstrings_v.find(name);
                name_exists = (it != wrapper.use_wstrings_v.end());
            }
            break;
        case db_int8:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<int8_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_int8_v.find(name);
                name_exists = (it != wrapper.use_int8_v.end());
            }
            break;
        case db_uint8:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<uint8_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_uint8_v.find(name);
                name_exists = (it != wrapper.use_uint8_v.end());
            }
            break;
        case db_int16:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<int16_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_int16_v.find(name);
                name_exists = (it != wrapper.use_int16_v.end());
            }
            break;
        case db_uint16:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<uint16_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_uint16_v.find(name);
                name_exists = (it != wrapper.use_uint16_v.end());
            }
            break;
        case db_int32:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<int32_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_int32_v.find(name);
                name_exists = (it != wrapper.use_int32_v.end());
            }
            break;
        case db_uint32:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<uint32_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_uint32_v.find(name);
                name_exists = (it != wrapper.use_uint32_v.end());
            }
            break;
        case db_int64:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<int64_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_int64_v.find(name);
                name_exists = (it != wrapper.use_int64_v.end());
            }
            break;
        case db_uint64:
            {
                typedef std::map
                    <
                        std::string,
                        std::vector<uint64_t>
                    >::const_iterator iterator;
                iterator const it = wrapper.use_uint64_v.find(name);
                name_exists = (it != wrapper.use_uint64_v.end());
            }
            break;
        case db_double:
            {
                typedef std::map<std::string,
                    std::vector<double> >::const_iterator iterator;
                iterator const it = wrapper.use_doubles_v.find(name);
                name_exists = (it != wrapper.use_doubles_v.end());
            }
            break;
        case db_date:
            {
                typedef std::map<std::string,
                        std::vector<std::tm> >::const_iterator iterator;
                iterator const it = wrapper.use_dates_v.find(name);
                name_exists = (it != wrapper.use_dates_v.end());
            }
            break;
        case db_blob:
        case db_xml:
            // no support for bulk and xml load
            break;
        }
    }

    if (name_exists)
    {
        wrapper.is_ok = true;
        return false;
    }
    else
    {
        wrapper.is_ok = false;
        wrapper.error_message = "No use ";
        wrapper.error_message += type_name;
        wrapper.error_message += " element with this name.";
        return true;
    }
}

// helper function for resizing all vectors<T> in the map
template <typename T>
void resize_in_map(std::map<std::string, std::vector<T> > & m, int new_size)
{
    for (auto& kv : m)
    {
        std::vector<T> & v = kv.second;
        v.resize(new_size);
    }
}

// helper for formatting date values
char const * format_date(statement_wrapper & wrapper, std::tm const & d)
{
    snprintf(wrapper.date_formatted, sizeof(wrapper.date_formatted),
        "%d %d %d %d %d %d",
        d.tm_year + 1900, d.tm_mon + 1, d.tm_mday,
        d.tm_hour, d.tm_min, d.tm_sec);

    return wrapper.date_formatted;
}

bool string_to_date(char const * val, std::tm & /* out */ dt,
    statement_wrapper & wrapper)
{
    // format is: "YYYY MM DD hh mm ss"
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int const converted = soci::sscanf(val, "%d %d %d %d %d %d",
        &year, &month, &day, &hour, &minute, &second);
    if (converted != 6)
    {
        wrapper.is_ok = false;
        wrapper.error_message = "Cannot convert date.";
        return false;
    }

    wrapper.is_ok = true;

    dt.tm_year = year - 1900;
    dt.tm_mon = month - 1;
    dt.tm_mday = day;
    dt.tm_hour = hour;
    dt.tm_min = minute;
    dt.tm_sec = second;

return true;
}

} // namespace unnamed


SOCI_DECL statement_handle soci_create_statement(session_handle s)
{
    session_wrapper * session_w = static_cast<session_wrapper *>(s);
    try
    {
        statement_wrapper * statement_w = new statement_wrapper(session_w->sql);
        return statement_w;
    }
    catch (std::exception const & e)
    {
        session_w->is_ok = false;
        session_w->error_message = e.what();
        return nullptr;
    }
}

SOCI_DECL void soci_destroy_statement(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);
    delete wrapper;
}

SOCI_DECL int soci_into_string(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_string);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_strings[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int(statement_handle st)
{
    return soci_into_int32(st);
}

SOCI_DECL int soci_into_long_long(statement_handle st)
{
    return soci_into_int64(st);
}

SOCI_DECL int soci_into_int8(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_int8);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_int8[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint8(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_uint8);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_uint8[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int16(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_int16);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_int16[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint16(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_uint16);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_uint16[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int32(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_int32);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_int32[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint32(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_uint32);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_uint32[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int64(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_int64);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_int64[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint64(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_uint64);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_uint64[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_double(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_double);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_doubles[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_date(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_date);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_dates[wrapper->next_position]; // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_blob(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::single;

    wrapper->into_types.push_back(db_blob);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_blob[wrapper->next_position] = soci_create_blob_session(wrapper->sql); // create new entry
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_string_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_string);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_strings_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int_v(statement_handle st)
{
    return soci_into_int32_v(st);
}

SOCI_DECL int soci_into_long_long_v(statement_handle st)
{
    return soci_into_int64_v(st);
}

SOCI_DECL int soci_into_int8_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_int8);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_int8_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint8_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_uint8);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_uint8_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int16_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_int16);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_int16_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint16_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_uint16);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_uint16_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int32_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_int32);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_int32_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint32_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_uint32);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_uint32_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_int64_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_int64);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_int64_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_uint64_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_uint64);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_uint64_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_double_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_double);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_doubles_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_into_date_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, true))
    {
        return -1;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->into_kind = statement_wrapper::bulk;

    wrapper->into_types.push_back(db_date);
    wrapper->into_indicators_v.push_back(std::vector<indicator>());
    wrapper->into_dates_v[wrapper->next_position];
    return wrapper->next_position++;
}

SOCI_DECL int soci_get_into_state(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position < 0 || position >= wrapper->next_position)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid position.";
        return 0;
    }

    wrapper->is_ok = true;
    return wrapper->into_indicators[position] == i_ok ? 1 : 0;
}

SOCI_DECL char const * soci_get_into_string(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_string, "string") ||
        not_null_check_failed(*wrapper, position))
    {
        return "";
    }

    return wrapper->into_strings[position].c_str();
}

SOCI_DECL int soci_get_into_int(statement_handle st, int position)
{
    return static_cast<int>(soci_get_into_int32(st, position));
}

SOCI_DECL long long soci_get_into_long_long(statement_handle st, int position)
{
    return static_cast<long long>(soci_get_into_int64(st, position));
}

SOCI_DECL int8_t soci_get_into_int8(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_int8, "int8") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_int8[position];
}

SOCI_DECL uint8_t soci_get_into_uint8(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_uint8, "uint8") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_uint8[position];
}

SOCI_DECL int16_t soci_get_into_int16(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_int16, "int16") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_int16[position];
}

SOCI_DECL uint16_t soci_get_into_uint16(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_uint16, "uint16") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_uint16[position];
}

SOCI_DECL int32_t soci_get_into_int32(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_int32, "int32") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_int32[position];
}

SOCI_DECL uint32_t soci_get_into_uint32(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_uint32, "uint32") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0;
    }

    return wrapper->into_uint32[position];
}

SOCI_DECL int64_t soci_get_into_int64(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_int64, "int64") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0LL;
    }

    return wrapper->into_int64[position];
}

SOCI_DECL uint64_t soci_get_into_uint64(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_uint64, "uint64") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0LL;
    }

    return wrapper->into_uint64[position];
}

SOCI_DECL double soci_get_into_double(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_double, "double") ||
        not_null_check_failed(*wrapper, position))
    {
        return 0.0;
    }

    return wrapper->into_doubles[position];
}

SOCI_DECL char const * soci_get_into_date(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_date, "date") ||
        not_null_check_failed(*wrapper, position))
    {
        return "";
    }

    // format is: "YYYY MM DD hh mm ss"
    std::tm const & d = wrapper->into_dates[position];
    return format_date(*wrapper, d);
}

SOCI_DECL blob_handle soci_get_into_blob(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::single, position, db_blob, "blob") ||
        not_null_check_failed(*wrapper, position))
    {
        return nullptr;
    }

    return wrapper->into_blob[position];
}

SOCI_DECL int soci_into_get_size_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (wrapper->into_kind != statement_wrapper::bulk)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "No vector into elements.";
        return -1;
    }

    return isize(wrapper->into_indicators_v[0]);
}

SOCI_DECL void soci_into_resize_v(statement_handle st, int new_size)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (new_size <= 0)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid size.";
        return;
    }

    if (wrapper->into_kind != statement_wrapper::bulk)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "No vector into elements.";
        return;
    }

    for (int i = 0; i != wrapper->next_position; ++i)
    {
        wrapper->into_indicators_v[i].resize(new_size);

        switch (wrapper->into_types[i])
        {
        case db_string:
            wrapper->into_strings_v[i].resize(new_size);
            break;
        case db_wstring:
            wrapper->into_wstrings_v[i].resize(new_size);
            break;
        case db_int8:
            wrapper->into_int8_v[i].resize(new_size);
            break;
        case db_uint8:
            wrapper->into_uint8_v[i].resize(new_size);
            break;
        case db_int16:
            wrapper->into_int16_v[i].resize(new_size);
            break;
        case db_uint16:
            wrapper->into_uint16_v[i].resize(new_size);
            break;
        case db_int32:
            wrapper->into_int32_v[i].resize(new_size);
            break;
        case db_uint32:
            wrapper->into_uint32_v[i].resize(new_size);
            break;
        case db_int64:
            wrapper->into_int64_v[i].resize(new_size);
            break;
        case db_uint64:
            wrapper->into_uint64_v[i].resize(new_size);
            break;
        case db_double:
            wrapper->into_doubles_v[i].resize(new_size);
            break;
        case db_date:
            wrapper->into_dates_v[i].resize(new_size);
            break;
        case db_blob:
        case db_xml:
            // no support for bulk blob
            break;
        }
    }

    wrapper->is_ok = true;
}

SOCI_DECL int soci_get_into_state_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position < 0 || position >= wrapper->next_position)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid position.";
        return 0;
    }

    std::vector<indicator> const & v = wrapper->into_indicators_v[position];
    if (index_check_failed(v, *wrapper, index))
    {
        return 0;
    }

    return v[index] == i_ok ? 1 : 0;
}

SOCI_DECL char const * soci_get_into_string_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_string, "string"))
    {
        return "";
    }

    std::vector<std::string> const & v = wrapper->into_strings_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return "";
    }

    return v[index].c_str();
}

SOCI_DECL int soci_get_into_int_v(statement_handle st, int position, int index)
{
    return static_cast<int>(soci_get_into_int32_v(st, position, index));
}

SOCI_DECL long long soci_get_into_long_long_v(statement_handle st, int position, int index)
{
    return static_cast<long long>(soci_get_into_int64_v(st, position, index));
}

SOCI_DECL int8_t soci_get_into_int8_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_int8, "int8"))
    {
        return 0;
    }

    std::vector<int8_t> const & v = wrapper->into_int8_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL uint8_t soci_get_into_uint8_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_uint8, "uint8"))
    {
        return 0;
    }

    std::vector<uint8_t> const & v = wrapper->into_uint8_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL int16_t soci_get_into_int16_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_int16, "int16"))
    {
        return 0;
    }

    std::vector<int16_t> const & v = wrapper->into_int16_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL uint16_t soci_get_into_uint16_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_uint16, "uint16"))
    {
        return 0;
    }

    std::vector<uint16_t> const & v = wrapper->into_uint16_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL int32_t soci_get_into_int32_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_int32, "int32"))
    {
        return 0;
    }

    std::vector<int32_t> const & v = wrapper->into_int32_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL uint32_t soci_get_into_uint32_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_uint32, "uint32"))
    {
        return 0;
    }

    std::vector<uint32_t> const & v = wrapper->into_uint32_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL int64_t soci_get_into_int64_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_int64, "int64"))
    {
        return 0;
    }

    std::vector<int64_t> const & v = wrapper->into_int64_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL uint64_t soci_get_into_uint64_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_uint64, "uint64"))
    {
        return 0;
    }

    std::vector<uint64_t> const & v = wrapper->into_uint64_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0;
    }

    return v[index];
}

SOCI_DECL double soci_get_into_double_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_double, "double"))
    {
        return 0.0;
    }

    std::vector<double> const & v = wrapper->into_doubles_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return 0.0;
    }

    return v[index];
}

SOCI_DECL char const * soci_get_into_date_v(statement_handle st, int position, int index)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper,
            statement_wrapper::bulk, position, db_date, "date"))
    {
        return "";
    }

    std::vector<std::tm> const & v = wrapper->into_dates_v[position];
    if (index_check_failed(v, *wrapper, index) ||
        not_null_check_failed(*wrapper, position, index))
    {
        return "";
    }

    return format_date(*wrapper, v[index]);
}

SOCI_DECL void soci_use_string(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_strings[name]; // create new entry
}

SOCI_DECL void soci_use_int(statement_handle st, char const * name)
{
    soci_use_int32(st, name);
}

SOCI_DECL void soci_use_long_long(statement_handle st, char const * name)
{
    soci_use_int64(st, name);
}

SOCI_DECL void soci_use_int8(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_int8[name]; // create new entry
}

SOCI_DECL void soci_use_uint8(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_uint8[name]; // create new entry
}

SOCI_DECL void soci_use_int16(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_int16[name]; // create new entry
}

SOCI_DECL void soci_use_uint16(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_uint16[name]; // create new entry
}

SOCI_DECL void soci_use_int32(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_int32[name]; // create new entry
}

SOCI_DECL void soci_use_uint32(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_uint32[name]; // create new entry
}

SOCI_DECL void soci_use_int64(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_int64[name]; // create new entry
}

SOCI_DECL void soci_use_uint64(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_uint64[name]; // create new entry
}

SOCI_DECL void soci_use_double(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_doubles[name]; // create new entry
}

SOCI_DECL void soci_use_date(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_dates[name]; // create new entry
}

SOCI_DECL void soci_use_blob(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::single, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::single, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::single;

    wrapper->use_indicators[name] = i_null; // create new entry
    wrapper->use_blob[name] = soci_create_blob_session(wrapper->sql); // create new entry
}

SOCI_DECL void soci_use_string_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_strings_v[name]; // create new entry
}

SOCI_DECL void soci_use_int_v(statement_handle st, char const * name)
{
    soci_use_int32_v(st, name);
}

SOCI_DECL void soci_use_long_long_v(statement_handle st, char const * name)
{
    soci_use_int64_v(st, name);
}

SOCI_DECL void soci_use_int8_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_int8_v[name]; // create new entry
}

SOCI_DECL void soci_use_uint8_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_uint8_v[name]; // create new entry
}

SOCI_DECL void soci_use_int16_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_int16_v[name]; // create new entry
}

SOCI_DECL void soci_use_uint16_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_uint16_v[name]; // create new entry
}

SOCI_DECL void soci_use_int32_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_int32_v[name]; // create new entry
}

SOCI_DECL void soci_use_uint32_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_uint32_v[name]; // create new entry
}

SOCI_DECL void soci_use_int64_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_int64_v[name]; // create new entry
}

SOCI_DECL void soci_use_uint64_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_uint64_v[name]; // create new entry
}

SOCI_DECL void soci_use_double_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_doubles_v[name]; // create new entry
}

SOCI_DECL void soci_use_date_v(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (cannot_add_elements(*wrapper, statement_wrapper::bulk, false) ||
        name_unique_check_failed(*wrapper, statement_wrapper::bulk, name))
    {
        return;
    }

    wrapper->statement_state = statement_wrapper::defining;
    wrapper->use_kind = statement_wrapper::bulk;

    wrapper->use_indicators_v[name]; // create new entry
    wrapper->use_dates_v[name]; // create new entry
}

SOCI_DECL void soci_set_use_state(statement_handle st, char const * name, int state)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    typedef std::map<std::string, indicator>::const_iterator iterator;
    iterator const it = wrapper->use_indicators.find(name);
    if (it == wrapper->use_indicators.end())
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid name.";
        return;
    }

    wrapper->is_ok = true;
    wrapper->use_indicators[name] = (state != 0 ? i_ok : i_null);
}

SOCI_DECL void soci_set_use_string(statement_handle st, char const * name, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_string, statement_wrapper::single, "string"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_strings[name] = val;
}

SOCI_DECL void soci_set_use_int(statement_handle st, char const * name, int32_t val)
{
    soci_set_use_int32(st, name, static_cast<int32_t>(val));
}

SOCI_DECL void soci_set_use_long_long(statement_handle st, char const * name, long long val)
{
    soci_set_use_int64(st, name, static_cast<int64_t>(val));
}

SOCI_DECL void soci_set_use_int8(statement_handle st, char const * name, int8_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int8, statement_wrapper::single, "int8"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_int8[name] = val;
}

SOCI_DECL void soci_set_use_uint8(statement_handle st, char const * name, uint32_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint32, statement_wrapper::single, "uint32"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_uint32[name] = val;
}

SOCI_DECL void soci_set_use_int16(statement_handle st, char const * name, int16_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int16, statement_wrapper::single, "int16"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_int16[name] = val;
}

SOCI_DECL void soci_set_use_uint16(statement_handle st, char const * name, uint16_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint16, statement_wrapper::single, "uint16"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_uint16[name] = val;
}

SOCI_DECL void soci_set_use_int32(statement_handle st, char const * name, int32_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int32, statement_wrapper::single, "int32"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_int32[name] = val;
}

SOCI_DECL void soci_set_use_uint32(statement_handle st, char const * name, uint32_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint32, statement_wrapper::single, "uint32"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_uint32[name] = val;
}

SOCI_DECL void soci_set_use_int64(statement_handle st, char const * name, int64_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int64, statement_wrapper::single, "int64"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_int64[name] = val;
}

SOCI_DECL void soci_set_use_uint64(statement_handle st, char const * name, uint64_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint64, statement_wrapper::single, "uint64"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_uint64[name] = val;
}

SOCI_DECL void soci_set_use_double(statement_handle st, char const * name, double val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_double, statement_wrapper::single, "double"))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_doubles[name] = val;
}

SOCI_DECL void soci_set_use_date(statement_handle st, char const * name, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_date, statement_wrapper::single, "date"))
    {
        return;
    }

    std::tm dt = std::tm();
    bool const converted = string_to_date(val, dt, *wrapper);
    if (converted == false)
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok;
    wrapper->use_dates[name] = dt;
}

SOCI_DECL void soci_set_use_blob(statement_handle st, char const * name, blob_handle b)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_blob, statement_wrapper::single, "blob"))
    {
        return;
    }

    soci::indicator &ind = wrapper->use_indicators[name];
    blob_wrapper *&blob = wrapper->use_blob[name];
    if (ind == i_null && blob != nullptr)
        soci_destroy_blob(blob);

    ind = i_ok;
    blob = static_cast<blob_wrapper *>(b);
}

SOCI_DECL int soci_use_get_size_v(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (wrapper->use_kind != statement_wrapper::bulk)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "No vector use elements.";
        return -1;
    }

    if (wrapper->use_indicators_v.empty())
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Empty indicators vector.";
        return -1;
    }

    return isize(wrapper->use_indicators_v.begin()->second);
}

SOCI_DECL void soci_use_resize_v(statement_handle st, int new_size)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (new_size <= 0)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid size.";
        return;
    }

    if (wrapper->use_kind != statement_wrapper::bulk)
    {
        wrapper->is_ok = false;
        wrapper->error_message = "No vector use elements.";
        return;
    }

    resize_in_map(wrapper->use_indicators_v, new_size);
    resize_in_map(wrapper->use_strings_v, new_size);
    resize_in_map(wrapper->use_int8_v, new_size);
    resize_in_map(wrapper->use_uint8_v, new_size);
    resize_in_map(wrapper->use_int16_v, new_size);
    resize_in_map(wrapper->use_uint16_v, new_size);
    resize_in_map(wrapper->use_int32_v, new_size);
    resize_in_map(wrapper->use_uint32_v, new_size);
    resize_in_map(wrapper->use_int64_v, new_size);
    resize_in_map(wrapper->use_uint64_v, new_size);
    resize_in_map(wrapper->use_doubles_v, new_size);
    resize_in_map(wrapper->use_dates_v, new_size);

    wrapper->is_ok = true;
}

SOCI_DECL void soci_set_use_state_v(statement_handle st,
    char const * name, int index, int state)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    typedef std::map<std::string, std::vector<indicator> >::iterator iterator;
    iterator const it = wrapper->use_indicators_v.find(name);
    if (it == wrapper->use_indicators_v.end())
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid name.";
        return;
    }

    std::vector<indicator> & v = it->second;
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    v[index] = (state != 0 ? i_ok : i_null);
}

SOCI_DECL void soci_set_use_string_v(statement_handle st,
    char const * name, int index, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_string, statement_wrapper::bulk, "vector string"))
    {
        return;
    }

    std::vector<std::string> & v = wrapper->use_strings_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_int_v(statement_handle st,
    char const * name, int index, int val)
{
    soci_set_use_int32_v(st, name, index, static_cast<int32_t>(val));
}

SOCI_DECL void soci_set_use_long_long_v(statement_handle st,
    char const * name, int index, long long val)
{
    soci_set_use_int64_v(st, name, index, static_cast<int64_t>(val));
}

SOCI_DECL void soci_set_use_int8_v(statement_handle st,
    char const * name, int index, int8_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int8, statement_wrapper::bulk, "vector int8"))
    {
        return;
    }

    std::vector<int8_t> & v = wrapper->use_int8_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_uint8_v(statement_handle st,
    char const * name, int index, uint8_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint8, statement_wrapper::bulk, "vector uint8"))
    {
        return;
    }

    std::vector<uint8_t> & v = wrapper->use_uint8_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_int16_v(statement_handle st,
    char const * name, int index, int16_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int16, statement_wrapper::bulk, "vector int16"))
    {
        return;
    }

    std::vector<int16_t> & v = wrapper->use_int16_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_uint16_v(statement_handle st,
    char const * name, int index, uint16_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint16, statement_wrapper::bulk, "vector uint16"))
    {
        return;
    }

    std::vector<uint16_t> & v = wrapper->use_uint16_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_int32_v(statement_handle st,
    char const * name, int index, int32_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int32, statement_wrapper::bulk, "vector int32"))
    {
        return;
    }

    std::vector<int32_t> & v = wrapper->use_int32_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_uint32_v(statement_handle st,
    char const * name, int index, uint32_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint32, statement_wrapper::bulk, "vector uint32"))
    {
        return;
    }

    std::vector<uint32_t> & v = wrapper->use_uint32_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_int64_v(statement_handle st,
    char const * name, int index, int64_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int64, statement_wrapper::bulk, "vector int64"))
    {
        return;
    }

    std::vector<int64_t> & v = wrapper->use_int64_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_uint64_v(statement_handle st,
    char const * name, int index, uint64_t val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint64, statement_wrapper::bulk, "vector uint64"))
    {
        return;
    }

    std::vector<uint64_t> & v = wrapper->use_uint64_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_double_v(statement_handle st,
    char const * name, int index, double val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_double, statement_wrapper::bulk, "vector double"))
    {
        return;
    }

    std::vector<double> & v = wrapper->use_doubles_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = val;
}

SOCI_DECL void soci_set_use_date_v(statement_handle st,
    char const * name, int index, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_date, statement_wrapper::bulk, "vector date"))
    {
        return;
    }

    std::vector<std::tm> & v = wrapper->use_dates_v[name];
    if (index_check_failed(v, *wrapper, index))
    {
        return;
    }

    std::tm dt = std::tm();
    bool const converted = string_to_date(val, dt, *wrapper);
    if (converted == false)
    {
        return;
    }

    wrapper->use_indicators_v[name][index] = i_ok;
    v[index] = dt;
}

SOCI_DECL int soci_get_use_state(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    typedef std::map<std::string, indicator>::const_iterator iterator;
    iterator const it = wrapper->use_indicators.find(name);
    if (it == wrapper->use_indicators.end())
    {
        wrapper->is_ok = false;
        wrapper->error_message = "Invalid name.";
        return 0;
    }

    wrapper->is_ok = true;
    return wrapper->use_indicators[name] == i_ok ? 1 : 0;
}

SOCI_DECL char const * soci_get_use_string(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_string, statement_wrapper::bulk, "string"))
    {
        return "";
    }

    return wrapper->use_strings[name].c_str();
}

SOCI_DECL int soci_get_use_int(statement_handle st, char const * name)
{
    return static_cast<int>(soci_get_use_int32(st, name));
}

SOCI_DECL long long soci_get_use_long_long(statement_handle st, char const * name)
{
    return static_cast<long long>(soci_get_use_int64(st, name));
}

SOCI_DECL int8_t soci_get_use_int8(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int8, statement_wrapper::bulk, "int8"))
    {
        return 0;
    }

    return wrapper->use_int8[name];
}

SOCI_DECL uint8_t soci_get_use_uint8(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint8, statement_wrapper::bulk, "uint8"))
    {
        return 0;
    }

    return wrapper->use_uint8[name];
}

SOCI_DECL int16_t soci_get_use_int16(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int16, statement_wrapper::bulk, "int16"))
    {
        return 0;
    }

    return wrapper->use_int16[name];
}

SOCI_DECL uint16_t soci_get_use_uint16(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint16, statement_wrapper::bulk, "uint16"))
    {
        return 0;
    }

    return wrapper->use_uint16[name];
}

SOCI_DECL int32_t soci_get_use_int32(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int32, statement_wrapper::bulk, "int32"))
    {
        return 0;
    }

    return wrapper->use_int32[name];
}

SOCI_DECL uint32_t soci_get_use_uint32(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint32, statement_wrapper::bulk, "uint32"))
    {
        return 0;
    }

    return wrapper->use_uint32[name];
}

SOCI_DECL int64_t soci_get_use_int64(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_int64, statement_wrapper::bulk, "int64"))
    {
        return 0LL;
    }

    return wrapper->use_int64[name];
}

SOCI_DECL uint64_t soci_get_use_uint64(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_uint64, statement_wrapper::bulk, "uint64"))
    {
        return 0LL;
    }

    return wrapper->use_uint64[name];
}

SOCI_DECL double soci_get_use_double(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_double, statement_wrapper::bulk, "double"))
    {
        return 0.0;
    }

    return wrapper->use_doubles[name];
}

SOCI_DECL char const * soci_get_use_date(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_date, statement_wrapper::bulk, "date"))
    {
        return "";
    }

    // format is: "YYYY MM DD hh mm ss"
    std::tm const & d = wrapper->use_dates[name];
    snprintf(wrapper->date_formatted, sizeof(wrapper->date_formatted),
        "%d %d %d %d %d %d",
        d.tm_year + 1900, d.tm_mon + 1, d.tm_mday,
        d.tm_hour, d.tm_min, d.tm_sec);

    return wrapper->date_formatted;
}

SOCI_DECL blob_handle soci_get_use_blob(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper,
            name, db_blob, statement_wrapper::bulk, "blob"))
    {
        return nullptr;
    }

    return wrapper->use_blob[name];
}

SOCI_DECL void soci_prepare(statement_handle st, char const * query)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        wrapper->statement_state = statement_wrapper::executing;

        // bind all into elements

        int const into_elements = isize(wrapper->into_types);
        if (wrapper->into_kind == statement_wrapper::single)
        {
            for (int i = 0; i != into_elements; ++i)
            {
                switch (wrapper->into_types[i])
                {
                case db_string:
                    wrapper->st.exchange(
                        into(wrapper->into_strings[i], wrapper->into_indicators[i]));
                    break;
                case db_wstring:
                    wrapper->st.exchange(
                        into(wrapper->into_wstrings[i], wrapper->into_indicators[i]));
                    break;
                case db_int8:
                    wrapper->st.exchange(
                        into(wrapper->into_int8[i], wrapper->into_indicators[i]));
                    break;
                case db_uint8:
                    wrapper->st.exchange(
                        into(wrapper->into_uint8[i], wrapper->into_indicators[i]));
                    break;
                case db_int16:
                    wrapper->st.exchange(
                        into(wrapper->into_int16[i], wrapper->into_indicators[i]));
                    break;
                case db_uint16:
                    wrapper->st.exchange(
                        into(wrapper->into_uint16[i], wrapper->into_indicators[i]));
                    break;
                case db_int32:
                    wrapper->st.exchange(
                        into(wrapper->into_int32[i], wrapper->into_indicators[i]));
                    break;
                case db_uint32:
                    wrapper->st.exchange(
                        into(wrapper->into_uint32[i], wrapper->into_indicators[i]));
                    break;
                case db_int64:
                    wrapper->st.exchange(
                        into(wrapper->into_int64[i], wrapper->into_indicators[i]));
                    break;
                case db_uint64:
                    wrapper->st.exchange(
                        into(wrapper->into_uint64[i], wrapper->into_indicators[i]));
                    break;
                case db_double:
                    wrapper->st.exchange(
                        into(wrapper->into_doubles[i], wrapper->into_indicators[i]));
                    break;
                case db_date:
                    wrapper->st.exchange(
                        into(wrapper->into_dates[i], wrapper->into_indicators[i]));
                    break;
                case db_blob:
                    wrapper->st.exchange(
                        into(wrapper->into_blob[i]->blob_, wrapper->into_indicators[i]));
                    break;
                case db_xml:
                    // no support for xml
                    break;
                }
            }
        }
        else
        {
            // vector elements
            for (int i = 0; i != into_elements; ++i)
            {
                switch (wrapper->into_types[i])
                {
                case db_string:
                    wrapper->st.exchange(
                        into(wrapper->into_strings_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_wstring:
                    wrapper->st.exchange(
                        into(wrapper->into_wstrings_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_int8:
                    wrapper->st.exchange(
                        into(wrapper->into_int8_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_uint8:
                    wrapper->st.exchange(
                        into(wrapper->into_uint8_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_int16:
                    wrapper->st.exchange(
                        into(wrapper->into_int16_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_uint16:
                    wrapper->st.exchange(
                        into(wrapper->into_uint16_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_int32:
                    wrapper->st.exchange(
                        into(wrapper->into_int32_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_uint32:
                    wrapper->st.exchange(
                        into(wrapper->into_uint32_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_int64:
                    wrapper->st.exchange(
                        into(wrapper->into_int64_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_uint64:
                    wrapper->st.exchange(
                        into(wrapper->into_uint64_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_double:
                    wrapper->st.exchange(
                        into(wrapper->into_doubles_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_date:
                    wrapper->st.exchange(
                        into(wrapper->into_dates_v[i], wrapper->into_indicators_v[i]));
                    break;
                case db_blob:
                case db_xml:
                    // no support for bulk blob and xml
                    break;
                }
            }
        }

        // bind all use elements
        {
            // strings
            for (auto& kv : wrapper->use_strings)
            {
                std::string const & use_name = kv.first;
                std::string & use_string = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_string, use_ind, use_name));
            }
        }
        {
            // int8
            for (auto& kv : wrapper->use_int8)
            {
                std::string const & use_name = kv.first;
                int8_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint8
            for (auto& kv : wrapper->use_uint8)
            {
                std::string const & use_name = kv.first;
                uint8_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int16
            for (auto& kv : wrapper->use_int16)
            {
                std::string const & use_name = kv.first;
                int16_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint16
            for (auto& kv : wrapper->use_uint16)
            {
                std::string const & use_name = kv.first;
                uint16_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int32
            for (auto& kv : wrapper->use_int32)
            {
                std::string const & use_name = kv.first;
                int32_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint32
            for (auto& kv : wrapper->use_uint32)
            {
                std::string const & use_name = kv.first;
                uint32_t & use_int = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int64
            for (auto& kv : wrapper->use_int64)
            {
                std::string const & use_name = kv.first;
                int64_t & use_longlong = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_longlong, use_ind, use_name));
            }
        }
        {
            // uint64
            for (auto& kv : wrapper->use_uint64)
            {
                std::string const & use_name = kv.first;
                uint64_t & use_longlong = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_longlong, use_ind, use_name));
            }
        }
        {
            // doubles
            for (auto& kv : wrapper->use_doubles)
            {
                std::string const & use_name = kv.first;
                double & use_double = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_double, use_ind, use_name));
            }
        }
        {
            // dates
            for (auto& kv : wrapper->use_dates)
            {
                std::string const & use_name = kv.first;
                std::tm & use_date = kv.second;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_date, use_ind, use_name));
            }
        }
        {
            // blobs
            for (auto& kv : wrapper->use_blob)
            {
                std::string const & use_name = kv.first;
                blob &use_blob = kv.second->blob_;
                indicator & use_ind = wrapper->use_indicators[use_name];
                wrapper->st.exchange(use(use_blob, use_ind, use_name));
            }
        }

        // bind all use vector elements
        {
            // strings
            for (auto& kv : wrapper->use_strings_v)
            {
                std::string const & use_name = kv.first;
                std::vector<std::string> & use_string = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_string, use_ind, use_name));
            }
        }
        {
            // int8
            for (auto& kv  : wrapper->use_int8_v)
            {
                std::string const & use_name = kv.first;
                std::vector<int8_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint8
            for (auto& kv  : wrapper->use_uint8_v)
            {
                std::string const & use_name = kv.first;
                std::vector<uint8_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int16
            for (auto& kv  : wrapper->use_int16_v)
            {
                std::string const & use_name = kv.first;
                std::vector<int16_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint16
            for (auto& kv  : wrapper->use_uint16_v)
            {
                std::string const & use_name = kv.first;
                std::vector<uint16_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int32
            for (auto& kv  : wrapper->use_int32_v)
            {
                std::string const & use_name = kv.first;
                std::vector<int32_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // uint32
            for (auto& kv  : wrapper->use_uint32_v)
            {
                std::string const & use_name = kv.first;
                std::vector<uint32_t> & use_int = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_int, use_ind, use_name));
            }
        }
        {
            // int64
            for (auto& kv  : wrapper->use_int64_v)
            {
                std::string const & use_name = kv.first;
                std::vector<int64_t> & use_longlong = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_longlong, use_ind, use_name));
            }
        }
        {
            // uint64
            for (auto& kv  : wrapper->use_uint64_v)
            {
                std::string const & use_name = kv.first;
                std::vector<uint64_t> & use_longlong = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_longlong, use_ind, use_name));
            }
        }
        {
            // doubles
            for (auto& kv  : wrapper->use_doubles_v)
            {
                std::string const & use_name = kv.first;
                std::vector<double> & use_double = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_double, use_ind, use_name));
            }
        }
        {
            // dates
            for (auto& kv  : wrapper->use_dates_v)
            {
                std::string const & use_name = kv.first;
                std::vector<std::tm> & use_date = kv.second;
                std::vector<indicator> & use_ind =
                    wrapper->use_indicators_v[use_name];
                wrapper->st.exchange(use(use_date, use_ind, use_name));
            }
        }

        wrapper->st.alloc();
        wrapper->st.prepare(query);
        wrapper->st.define_and_bind();

        wrapper->is_ok = true;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();
    }
}

SOCI_DECL int soci_execute(statement_handle st, int withDataExchange)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        bool const gotData = wrapper->st.execute(withDataExchange != 0);

        wrapper->is_ok = true;

        return gotData ? 1 : 0;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();

        return 0;
    }
}

SOCI_DECL long long soci_get_affected_rows(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->st.get_affected_rows();
}

SOCI_DECL int soci_fetch(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        bool const gotData = wrapper->st.fetch();

        wrapper->is_ok = true;

        return gotData ? 1 : 0;
    }
    catch (std::exception const & e)
    {
        wrapper->is_ok = false;
        wrapper->error_message = e.what();

        return 0;
    }
}

SOCI_DECL int soci_got_data(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->st.got_data() ? 1 : 0;
}

SOCI_DECL int soci_statement_state(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->is_ok ? 1 : 0;
}

SOCI_DECL char const * soci_statement_error_message(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->error_message.c_str();
}
