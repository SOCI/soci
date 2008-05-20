//
// Copyright (C) 2008 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE

#include "soci-simple.h"
#include "soci.h"

using namespace soci;


namespace // unnamed
{

struct session_wrapper
{
    session sql;

    bool isOK;
    std::string errorMessage;
};

} // namespace unnamed


SOCI_DECL session_handle soci_create_session(char const * connectionString)
{
    session_wrapper * wrapper = new session_wrapper();
    try
    {
        wrapper->sql.open(connectionString);
        wrapper->isOK = true;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();
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
        wrapper->isOK = true;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();
    }
}

SOCI_DECL void soci_commit(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    try
    {
        wrapper->sql.commit();
        wrapper->isOK = true;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();
    }
}

SOCI_DECL void soci_rollback(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    try
    {
        wrapper->sql.rollback();
        wrapper->isOK = true;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();
    }
}

SOCI_DECL void soci_uppercase_column_names(session_handle s, bool forceToUpper)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);
    wrapper->sql.uppercase_column_names(forceToUpper);
    wrapper->isOK = true;
}

SOCI_DECL bool soci_session_state(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);

    return wrapper->isOK;
}

SOCI_DECL char const * soci_session_error_message(session_handle s)
{
    session_wrapper * wrapper = static_cast<session_wrapper *>(s);

    return wrapper->errorMessage.c_str();
}


// statement


namespace // unnamed
{

struct statement_wrapper
{
    statement_wrapper(session & sql)
        : st(sql), nextPosition(0), dataItemsFixed(false) {}

    statement st;

    // into elements
    int nextPosition;
    std::vector<data_type> into_types;
    std::vector<indicator> into_indicators;
    std::map<int, std::string> into_strings;
    std::map<int, int> into_ints;
    std::map<int, long long> into_longlongs;
    std::map<int, double> into_doubles;
    std::map<int, std::tm> into_dates;

    // use elements
    std::map<std::string, indicator> use_indicators;
    std::map<std::string, std::string> use_strings;
    std::map<std::string, int> use_ints;
    std::map<std::string, long long> use_longlongs;
    std::map<std::string, double> use_doubles;
    std::map<std::string, std::tm> use_dates;

    // format is: "YYYY MM DD hh mm ss"
    char date_formatted[20];

    bool dataItemsFixed;

    bool isOK;
    std::string errorMessage;
};

// helper for checking if the attempt was made to add more into/use elements
// after their set was fixed
bool fixed_check_failed(statement_wrapper & wrapper)
{
    if (wrapper.dataItemsFixed)
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "Cannot add more data items.";
        return true;
    }
    else
    {
        wrapper.isOK = true;
        return false;
    }
}

bool position_check_failed(statement_wrapper & wrapper,
    int position, data_type expectedType, char const * typeName)
{
    if (position < 0 || position >= wrapper.nextPosition)
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "Invalid position.";
        return true;
    }

    if (wrapper.into_types[position] != expectedType)
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "No into ";
        wrapper.errorMessage += typeName;
        wrapper.errorMessage += " element at this position.";
        return true;
    }

    if (wrapper.into_indicators[position] == i_null)
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "Element is null.";
        return true;
    }

    wrapper.isOK = true;
    return false;
}

bool name_unique_check_failed(statement_wrapper & wrapper, char const * name)
{
    typedef std::map<std::string, indicator>::const_iterator iterator;
    iterator const it = wrapper.use_indicators.find(name);
    if (it != wrapper.use_indicators.end())
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "Name of use element should be unique.";
        return true;
    }
    else
    {
        wrapper.isOK = true;
        return false;
    }
}

bool name_exists_check_failed(statement_wrapper & wrapper,
    char const * name, data_type expectedType, char const * typeName)
{
    bool nameExists;
    switch (expectedType)
    {
    case dt_string:
        {
            typedef std::map<std::string, std::string>::const_iterator iterator;
            iterator const it = wrapper.use_strings.find(name);
            nameExists = (it != wrapper.use_strings.end());
        }
        break;
    case dt_integer:
        {
            typedef std::map<std::string, int>::const_iterator iterator;
            iterator const it = wrapper.use_ints.find(name);
            nameExists = (it != wrapper.use_ints.end());
        }
        break;
    case dt_long_long:
        {
            typedef std::map<std::string, long long>::const_iterator iterator;
            iterator const it = wrapper.use_longlongs.find(name);
            nameExists = (it != wrapper.use_longlongs.end());
        }
        break;
    case dt_double:
        {
            typedef std::map<std::string, double>::const_iterator iterator;
            iterator const it = wrapper.use_doubles.find(name);
            nameExists = (it != wrapper.use_doubles.end());
        }
        break;
    case dt_date:
        {
            typedef std::map<std::string, std::tm>::const_iterator iterator;
            iterator const it = wrapper.use_dates.find(name);
            nameExists = (it != wrapper.use_dates.end());
        }
        break;
    default:
        assert(false);
    }

    if (nameExists)
    {
        wrapper.isOK = true;
        return false;
    }
    else
    {
        wrapper.isOK = false;
        wrapper.errorMessage = "No use ";
        wrapper.errorMessage += typeName;
        wrapper.errorMessage += " element with this name.";
        return true;
    }
}

} // namespace unnamed


SOCI_DECL statement_handle soci_create_statement(session_handle s)
{
    session_wrapper * session_w = static_cast<session_wrapper *>(s);
    try
    {
        statement_wrapper * statement_w = new statement_wrapper(session_w->sql);
        statement_w->isOK = true;
        return statement_w;
    }
    catch (std::exception const & e)
    {
        session_w->isOK = false;
        session_w->errorMessage = e.what();
        return NULL;
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

    if (fixed_check_failed(*wrapper))
    {
        return -1;
    }

    wrapper->into_types.push_back(dt_string);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_strings[wrapper->nextPosition]; // create new entry
    return wrapper->nextPosition++;
}

SOCI_DECL int soci_into_int(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper))
    {
        return -1;
    }

    wrapper->into_types.push_back(dt_integer);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_ints[wrapper->nextPosition]; // create new entry
    return wrapper->nextPosition++;
}

SOCI_DECL int soci_into_long_long(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper))
    {
        return -1;
    }

    wrapper->into_types.push_back(dt_long_long);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_longlongs[wrapper->nextPosition]; // create new entry
    return wrapper->nextPosition++;
}

SOCI_DECL int soci_into_double(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper))
    {
        return -1;
    }

    wrapper->into_types.push_back(dt_double);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_doubles[wrapper->nextPosition]; // create new entry
    return wrapper->nextPosition++;
}

SOCI_DECL int soci_into_date(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper))
    {
        return -1;
    }

    wrapper->into_types.push_back(dt_date);
    wrapper->into_indicators.push_back(i_ok);
    wrapper->into_dates[wrapper->nextPosition]; // create new entry
    return wrapper->nextPosition++;
}

SOCI_DECL bool soci_get_into_state(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position < 0 || position >= wrapper->nextPosition)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = "Invalid position.";
        return false;
    }

    wrapper->isOK = true;
    return wrapper->into_indicators[position] == i_ok;
}

SOCI_DECL char const * soci_get_into_string(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper, position, dt_string, "string"))
    {
        return "";
    }

    return wrapper->into_strings[position].c_str();
}

SOCI_DECL int soci_get_into_int(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper, position, dt_integer, "int"))
    {
        return 0;
    }

    return wrapper->into_ints[position];
}

SOCI_DECL long long soci_get_into_long_long(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper, position, dt_long_long, "long long"))
    {
        return 0LL;
    }

    return wrapper->into_longlongs[position];
}

SOCI_DECL double soci_get_into_double(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper, position, dt_double, "double"))
    {
        return 0.0;
    }

    return wrapper->into_doubles[position];
}

SOCI_DECL char const * soci_get_into_date(statement_handle st, int position)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (position_check_failed(*wrapper, position, dt_date, "date"))
    {
        return "";
    }

    // format is: "YYYY MM DD hh mm ss"
    std::tm const & d = wrapper->into_dates[position];
    sprintf(wrapper->date_formatted, "%d %d %d %d %d %d",
        d.tm_year + 1900, d.tm_mon + 1, d.tm_mday,
        d.tm_hour, d.tm_min, d.tm_sec);

    return wrapper->date_formatted;
}

SOCI_DECL void soci_use_string(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper) || name_unique_check_failed(*wrapper, name))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_strings[name]; // create new entry
}

SOCI_DECL void soci_use_int(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper) || name_unique_check_failed(*wrapper, name))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_ints[name]; // create new entry
}

SOCI_DECL void soci_use_long_long(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper) || name_unique_check_failed(*wrapper, name))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_longlongs[name]; // create new entry
}

SOCI_DECL void soci_use_double(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper) || name_unique_check_failed(*wrapper, name))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_doubles[name]; // create new entry
}

SOCI_DECL void soci_use_date(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (fixed_check_failed(*wrapper) || name_unique_check_failed(*wrapper, name))
    {
        return;
    }

    wrapper->use_indicators[name] = i_ok; // create new entry
    wrapper->use_dates[name]; // create new entry
}

SOCI_DECL void soci_set_use_state(statement_handle st, char const * name, bool state)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    typedef std::map<std::string, indicator>::const_iterator iterator;
    iterator const it = wrapper->use_indicators.find(name);
    if (it == wrapper->use_indicators.end())
    {
        wrapper->isOK = false;
        wrapper->errorMessage = "Invalid name.";
        return;
    }

    wrapper->isOK = true;
    wrapper->use_indicators[name] = state ? i_ok : i_null;
}

SOCI_DECL void soci_set_use_string(statement_handle st, char const * name, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_string, "string"))
    {
        return;
    }

    wrapper->use_strings[name] = val;
}

SOCI_DECL void soci_set_use_int(statement_handle st, char const * name, int val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_integer, "int"))
    {
        return;
    }

    wrapper->use_ints[name] = val;
}

SOCI_DECL void soci_set_use_long_long(statement_handle st, char const * name, long long val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_long_long, "long long"))
    {
        return;
    }

    wrapper->use_longlongs[name] = val;
}

SOCI_DECL void soci_set_use_double(statement_handle st, char const * name, double val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_double, "double"))
    {
        return;
    }

    wrapper->use_doubles[name] = val;
}

SOCI_DECL void soci_set_use_date(statement_handle st, char const * name, char const * val)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_integer, "int"))
    {
        return;
    }

    // format is: "YYYY MM DD hh mm ss"
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int const converted = sscanf(val, "%d %d %d %d %d %d",
        &year, &month, &day, &hour, &minute, &second);
    if (converted != 6)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = "Cannot convert date.";
        return;
    }

    wrapper->isOK = true;

    std::tm d;
    d.tm_year = year - 1900;
    d.tm_mon = month - 1;
    d.tm_mday = day;
    d.tm_hour = hour;
    d.tm_min = minute;
    d.tm_sec = second;
    wrapper->use_dates[name] = d;
}

SOCI_DECL bool soci_get_use_state(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    typedef std::map<std::string, indicator>::const_iterator iterator;
    iterator const it = wrapper->use_indicators.find(name);
    if (it == wrapper->use_indicators.end())
    {
        wrapper->isOK = false;
        wrapper->errorMessage = "Invalid name.";
        return false;
    }

    wrapper->isOK = true;
    return wrapper->use_indicators[name] == i_ok;
}

SOCI_DECL char const * soci_get_use_string(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_string, "string"))
    {
        return "";
    }

    return wrapper->use_strings[name].c_str();
}

SOCI_DECL int soci_get_use_int(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_integer, "int"))
    {
        return 0;
    }

    return wrapper->use_ints[name];
}

SOCI_DECL long long soci_get_use_long_long(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_long_long, "long long"))
    {
        return 0LL;
    }

    return wrapper->use_longlongs[name];
}

SOCI_DECL double soci_get_use_double(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_double, "double"))
    {
        return 0.0;
    }

    return wrapper->use_doubles[name];
}

SOCI_DECL char const * soci_get_use_date(statement_handle st, char const * name)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    if (name_exists_check_failed(*wrapper, name, dt_integer, "int"))
    {
        return "";
    }

    // format is: "YYYY MM DD hh mm ss"
    std::tm const & d = wrapper->use_dates[name];
    sprintf(wrapper->date_formatted, "%d %d %d %d %d %d",
        d.tm_year + 1900, d.tm_mon + 1, d.tm_mday,
        d.tm_hour, d.tm_min, d.tm_sec);

    return wrapper->date_formatted;
}

SOCI_DECL void soci_prepare(statement_handle st, char const * query)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        wrapper->dataItemsFixed = true;

        // bind all into elements
        size_t const intoElements = wrapper->into_types.size();
        for (size_t i = 0; i != intoElements; ++i)
        {
            switch (wrapper->into_types[i])
            {
            case dt_string:
                wrapper->st.exchange(
                    into(wrapper->into_strings[i], wrapper->into_indicators[i]));
                break;
            case dt_integer:
                wrapper->st.exchange(
                    into(wrapper->into_ints[i], wrapper->into_indicators[i]));
                break;
            case dt_long_long:
                wrapper->st.exchange(
                    into(wrapper->into_longlongs[i], wrapper->into_indicators[i]));
                break;
            case dt_double:
                wrapper->st.exchange(
                    into(wrapper->into_doubles[i], wrapper->into_indicators[i]));
                break;
            case dt_date:
                wrapper->st.exchange(
                    into(wrapper->into_dates[i], wrapper->into_indicators[i]));
                break;
            default:
                assert(false);
            }
        }

        // bind all use elements
        {
            // strings
            typedef std::map<std::string, std::string>::iterator iterator;
            iterator uit = wrapper->use_strings.begin();
            iterator const uend = wrapper->use_strings.end();
            for ( ; uit != uend; ++uit)
            {            
                std::string const & useName = uit->first;
                std::string & useString = uit->second;
                indicator & useInd = wrapper->use_indicators[useName];
                wrapper->st.exchange(use(useString, useInd, useName));
            }
        }
        {
            // ints
            typedef std::map<std::string, int>::iterator iterator;
            iterator uit = wrapper->use_ints.begin();
            iterator const uend = wrapper->use_ints.end();
            for ( ; uit != uend; ++uit)
            {            
                std::string const & useName = uit->first;
                int & useInt = uit->second;
                indicator & useInd = wrapper->use_indicators[useName];
                wrapper->st.exchange(use(useInt, useInd, useName));
            }
        }
        {
            // longlongs
            typedef std::map<std::string, long long>::iterator iterator;
            iterator uit = wrapper->use_longlongs.begin();
            iterator const uend = wrapper->use_longlongs.end();
            for ( ; uit != uend; ++uit)
            {            
                std::string const & useName = uit->first;
                long long & useLongLong = uit->second;
                indicator & useInd = wrapper->use_indicators[useName];
                wrapper->st.exchange(use(useLongLong, useInd, useName));
            }
        }
        {
            // doubles
            typedef std::map<std::string, double>::iterator iterator;
            iterator uit = wrapper->use_doubles.begin();
            iterator const uend = wrapper->use_doubles.end();
            for ( ; uit != uend; ++uit)
            {            
                std::string const & useName = uit->first;
                double & useDouble = uit->second;
                indicator & useInd = wrapper->use_indicators[useName];
                wrapper->st.exchange(use(useDouble, useInd, useName));
            }
        }
        {
            // dates
            typedef std::map<std::string, std::tm>::iterator iterator;
            iterator uit = wrapper->use_dates.begin();
            iterator const uend = wrapper->use_dates.end();
            for ( ; uit != uend; ++uit)
            {            
                std::string const & useName = uit->first;
                std::tm & useDate = uit->second;
                indicator & useInd = wrapper->use_indicators[useName];
                wrapper->st.exchange(use(useDate, useInd, useName));
            }
        }

        wrapper->st.alloc();
        wrapper->st.prepare(query);
        wrapper->st.define_and_bind();

        wrapper->isOK = true;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();
    }
}

SOCI_DECL bool soci_execute(statement_handle st, bool withDataExchange)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        bool const gotData = wrapper->st.execute(withDataExchange);

        wrapper->isOK = true;

        return gotData;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();

        return false;
    }
}

SOCI_DECL bool soci_fetch(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    try
    {
        bool const gotData = wrapper->st.fetch();

        wrapper->isOK = true;

        return gotData;
    }
    catch (std::exception const & e)
    {
        wrapper->isOK = false;
        wrapper->errorMessage = e.what();

        return false;
    }
}

SOCI_DECL bool soci_got_data(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->st.got_data();
}

SOCI_DECL bool soci_statement_state(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->isOK;
}

SOCI_DECL char const * soci_statement_error_message(statement_handle st)
{
    statement_wrapper * wrapper = static_cast<statement_wrapper *>(st);

    return wrapper->errorMessage.c_str();
}
