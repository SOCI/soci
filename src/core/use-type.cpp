//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_SOURCE
#include "use-type.h"
#include "statement.h"
#include "soci-platform.h"
#include <cstdio>

using namespace soci;
using namespace soci::details;

void use_type_base::to_string(std::string & str, void * data, exchange_type type)
{
	int n = -1;
	switch (type)
	{
	case x_short:
		str.resize(6);
		n = snprintf(const_cast<char*>(str.data()), str.size(), "%d", static_cast<int>(*static_cast<short*>(data)));
		break;
	case x_integer:
		str.resize(11);
		n = snprintf(const_cast<char*>(str.data()), str.size(), "%d", *static_cast<int*>(data));
		break;
	case x_long_long:
		str.resize(21);
		n = snprintf(const_cast<char*>(str.data()), str.size(), "%lld", *static_cast<long long*>(data));
		break;
	case x_unsigned_long_long:
		str.resize(20);
		n = snprintf(const_cast<char*>(str.data()), str.size(), "%llu", *static_cast<unsigned long long*>(data));
		break;
	case x_double:
		str.resize(100);
		n = snprintf(const_cast<char*>(str.data()), str.size(), "%.20g", *static_cast<double*>(data));
		break;
	case x_stdtm:
		{
			str.resize(19);
			const std::tm* val = static_cast<std::tm*>(data);
			n = snprintf(const_cast<char*>(str.data()), str.size(), "%04d-%02d-%02d %02d:%02d:%02d",
				val->tm_year + 1900, val->tm_mon + 1, val->tm_mday, 
				val->tm_hour, val->tm_min, val->tm_sec);
		}
		break;
	case x_statement:
		str = "s";
		break;
	case x_rowid:
		str = "r";
		break;
	case x_blob:
		str = "b";
		break;
	default:
		str = "?";
		break;
	}
	if (n < 0 || n > static_cast<int>(str.size()))
		str.clear();
	else
		str.resize(n);
}

standard_use_type::~standard_use_type()
{
    delete backEnd_;
}

void standard_use_type::bind(statement_impl & st, int & position)
{
    backEnd_ = st.make_use_type_backend();
    if (name_.empty())
    {
        backEnd_->bind_by_pos(position, data_, type_, readOnly_);
    }
    else
    {
        backEnd_->bind_by_name(name_, data_, type_, readOnly_);
    }
}

void standard_use_type::pre_use()   
{
    // Handle IN direction of parameters of SQL statements and procedures
    convert_to_base();
    backEnd_->pre_use(ind_);
}

void standard_use_type::post_use(bool gotData)
{
    // Handle OUT direction of IN/OUT parameters of stored procedures
    backEnd_->post_use(gotData, ind_);
    convert_from_base();

    // IMPORTANT:
    // This treatment of input ("use") parameter as output data sink may be 
    // confusing, but it is necessary to store OUT data back in the same 
    // object as IN, of IN/OUT parameter.
    // As there is no symmetry for IN/OUT in SQL and there are no OUT/IN
    // we do not perform convert_to_base() for output ("into") parameter.
    // See conversion_use_type<T>::convert_from_base() for more details.
}

void standard_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}

const char * standard_use_type::to_string(std::size_t & length, std::size_t /*index*/)
{
	length = 0;
	if (ind_ && *ind_ == i_null)
	{
		return NULL;
	}

	const char * str = backEnd_->c_str(length);
	if (str == NULL)
	{
		if (type_ == x_char)
		{
			str = static_cast<char*>(data_);
			length = 1;
		}
		else if (type_ == x_stdstring)
		{
			std::string* s = static_cast<std::string*>(data_); 
			str = s->c_str();
			length = s->length();
		}
		else
		{
			use_type_base::to_string(str_, data_, type_);
			str = str_.c_str();
			length = str_.length();
		}
	}
	return str;
}

vector_use_type::~vector_use_type()
{
    delete backEnd_;
}

void vector_use_type::bind(statement_impl & st, int & position)
{
    backEnd_ = st.make_vector_use_type_backend();
    if (name_.empty())
    {
        backEnd_->bind_by_pos(position, data_, type_);
    }
    else
    {
        backEnd_->bind_by_name(name_, data_, type_);
    }
}

void vector_use_type::pre_use()
{
    convert_to_base();

    backEnd_->pre_use(ind_ ? &ind_->at(0) : NULL);
}

std::size_t vector_use_type::size() const
{
    return backEnd_->size();
}

void vector_use_type::clean_up()
{
    if (backEnd_ != NULL)
    {
        backEnd_->clean_up();
    }
}

exchange_type vector_use_type::get_type() const
{
	return type_;
}

const char * vector_use_type::to_string(std::size_t & length, std::size_t index)
{
	length = 0;
	if (ind_ && ind_->at(index) == i_null)
	{
		return NULL;
	}

	const char * str = backEnd_->c_str(length, index);
	if (str == NULL)
	{
		if (type_ == x_char)
		{
			str = static_cast<char*>(data_);
			length = 1;
		}
		else if (type_ == x_stdstring)
		{
			std::string* s = static_cast<std::string*>(data_); 
			str = s->c_str();
			length = s->length();
		}
		else
		{
			if (strs_.empty())
			{
				strs_.resize(size());
			}
			std::string & strs = strs_.at(index);
			use_type_base::to_string(strs, data_, type_);
			str = strs.c_str();
			length = strs.length();
		}
	}
	return str;
}
