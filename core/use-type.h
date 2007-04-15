//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_USE_TYPE_H_INCLUDED
#define SOCI_USE_TYPE_H_INCLUDED

#include "soci-backend.h"
#include "type-ptr.h"
#include "exchange-traits.h"
#include <boost/optional.hpp>

#include <string>
#include <vector>

namespace soci
{
namespace details
{
class statement_impl;

// this is intended to be a base class for all classes that deal with
// binding input data (and OUT PL/SQL variables)
class SOCI_DECL use_type_base
{
public:
    virtual ~use_type_base() {}

    virtual void bind(statement_impl &st, int &position) = 0;
    virtual void pre_use() = 0;
    virtual void post_use(bool gotData) = 0;
    virtual void clean_up() = 0;

    virtual std::size_t size() const = 0;  // returns the number of elements
};


class SOCI_DECL standard_use_type : public use_type_base
{
public:
    standard_use_type(void *data, eExchangeType type,
        std::string const &name = std::string())
        : data_(data), type_(type), ind_(NULL), name_(name), backEnd_(NULL) {}
    standard_use_type(void *data, eExchangeType type, eIndicator &ind,
        std::string const &name = std::string())
        : data_(data), type_(type), ind_(&ind), name_(name), backEnd_(NULL) {}

    virtual ~standard_use_type();
    virtual void bind(statement_impl &st, int &position);
    std::string get_name() const {return name_;}
    virtual void* get_data() {return data_;}
     
    // conversion hook (from arbitrary user type to base type)
    virtual void convert_to() {}
    virtual void convert_from() {} 

protected:
    virtual void pre_use();

private:
    virtual void post_use(bool gotData);
    virtual void clean_up();
    virtual std::size_t size() const { return 1; }

    void *data_;
    eExchangeType type_;
    eIndicator *ind_;
    std::string name_;

    standard_use_type_backend *backEnd_;
};

class SOCI_DECL vector_use_type : public use_type_base
{
public:
    vector_use_type(void *data, eExchangeType type,
        std::string const &name = std::string())
        : data_(data), type_(type), ind_(NULL),
          name_(name), backEnd_(NULL) {}

    vector_use_type(void *data, eExchangeType type,
        std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : data_(data), type_(type), ind_(&ind.at(0)),
          name_(name), backEnd_(NULL) {}

    ~vector_use_type();

private:
    virtual void bind(statement_impl &st, int &position);
    virtual void pre_use();
    virtual void post_use(bool) { /* nothing to do */ }
    virtual void clean_up();
    virtual std::size_t size() const;

    void *data_;
    eExchangeType type_;
    eIndicator const *ind_;
    std::string name_;

    vector_use_type_backend *backEnd_;

    virtual void convert_to() {}
};

// implementation for the basic types (those which are supported by the library
// out of the box without user-provided conversions)

template <typename T>
class use_type : public standard_use_type
{
public:
    use_type(T &t, std::string const &name = std::string())
        : standard_use_type(&t,
            static_cast<eExchangeType>(exchange_traits<T>::eXType),
            name) {}
    use_type(T &t, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&t,
            static_cast<eExchangeType>(exchange_traits<T>::eXType),
            ind, name) {}
};

template <typename T>
class use_type<std::vector<T> > : public vector_use_type
{
public:
    use_type(std::vector<T> &v, std::string const &name = std::string())
        : vector_use_type(&v,
            static_cast<eExchangeType>(exchange_traits<T>::eXType), name) {}
    use_type(std::vector<T> &v, std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : vector_use_type(&v,
            static_cast<eExchangeType>(exchange_traits<T>::eXType), ind, name) {}
};

template <typename T>
class use_type<boost::optional<T> > : public standard_use_type
{
public:
    use_type(boost::optional<T> &t, std::string const &name = std::string())
        : standard_use_type(&val_,
            static_cast<eExchangeType>(exchange_traits<T>::eXType),
            ind_, name), opt_(t) {}

private:
    boost::optional<T> &opt_;
    T val_;
    eIndicator ind_;

    virtual void pre_use()
    {
        if (opt_.is_initialized())
        {
            val_ = opt_.get();
            ind_ = eOK;
        }
        else
        {
            ind_ = eNull;
        }
        standard_use_type::pre_use();
    }
};

// special cases for char* and char[]

template <>
class use_type<char*> : public standard_use_type
{
public:
    use_type(char *str, std::size_t bufSize,
        std::string const &name = std::string())
        : standard_use_type(&str_, eXCString, name), str_(str, bufSize) {}
    use_type(char *str, eIndicator &ind, std::size_t bufSize,
        std::string const &name = std::string())
        : standard_use_type(&str_, eXCString, ind, name), str_(str, bufSize) {}

private:
    cstring_descriptor str_;
};

template <std::size_t N>
class use_type<char[N]> : public use_type<char*>
{
public:
    use_type(char str[], std::string const &name = std::string())
        : use_type<char*>(str, N, name) {}
    use_type(char str[], eIndicator &ind,
        std::string const &name = std::string())
        : use_type<char*>(str, ind, N, name) {}
};

// helper dispatchers for basic types

template <typename T>
use_type_ptr do_use(T &t, std::string const &name, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, name));
}

template <typename T>
use_type_ptr do_use(T &t, eIndicator &indicator,
    std::string const &name, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, indicator, name));
}

template <typename T>
use_type_ptr do_use(T &t, std::vector<eIndicator> &indicator,
    std::string const &name, basic_type_tag)
{
    return use_type_ptr(new use_type<T>(t, indicator, name));
}

} // namespace details

} // namesapce soci

#endif
