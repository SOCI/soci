//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_INTO_TYPE_H_INCLUDED
#define SOCI_INTO_TYPE_H_INCLUDED

#include "soci-backend.h"
#include "type-ptr.h"

#include <string>
#include <vector>

namespace soci
{
class session;

namespace details
{
class prepare_temp_type;
class standard_into_type_backend;
class vector_into_type_backend;
class statement_impl;

// this is intended to be a base class for all classes that deal with
// defining output data
class into_type_base
{
public:
    virtual ~into_type_base() {}

    virtual void define(statement_impl &st, int &position) = 0;
    virtual void pre_fetch() = 0;
    virtual void post_fetch(bool gotData, bool calledFromFetch) = 0;
    virtual void clean_up() = 0;

    virtual std::size_t size() const = 0;  // returns the number of elements
    virtual void resize(std::size_t /* sz */) {} // used for vectors only
};


// standard types

class SOCI_DECL standard_into_type : public into_type_base
{
public:
    standard_into_type(void *data, eExchangeType type)
        : data_(data), type_(type), ind_(NULL), backEnd_(NULL) {}
    standard_into_type(void *data, eExchangeType type, eIndicator& ind)
        : data_(data), type_(type), ind_(&ind), backEnd_(NULL) {}

    virtual ~standard_into_type();

private:
    virtual void define(statement_impl &st, int &position);
    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, bool calledFromFetch);
    virtual void clean_up();

    virtual std::size_t size() const { return 1; }

    // conversion hook (from base type to arbitrary user type)
    virtual void convert_from() {}

    void *data_;
    eExchangeType type_;
    eIndicator *ind_;

    standard_into_type_backend *backEnd_;
};

// into type base class for vectors
class SOCI_DECL vector_into_type : public into_type_base
{
public:
    vector_into_type(void *data, eExchangeType type)
        : data_(data), type_(type), indVec_(NULL), backEnd_(NULL) {}

    vector_into_type(void *data, eExchangeType type,
        std::vector<eIndicator> &ind)
        : data_(data), type_(type), indVec_(&ind), backEnd_(NULL) {}

    ~vector_into_type();

private:
    virtual void define(statement_impl &st, int &position);
    virtual void pre_fetch();
    virtual void post_fetch(bool gotData, bool calledFromFetch);
    virtual void clean_up();
    virtual void resize(std::size_t sz);
    virtual std::size_t size() const;

    void *data_;
    eExchangeType type_;
    std::vector<eIndicator> *indVec_;

    vector_into_type_backend *backEnd_;

    virtual void convert_from() {}
};

// general case not implemented
template <typename T>
class into_type;

template <>
class into_type<short> : public standard_into_type
{
public:
    into_type(short &s) : standard_into_type(&s, eXShort) {}
    into_type(short &s, eIndicator &ind)
        : standard_into_type(&s, eXShort, ind) {}
};

template <>
class into_type<std::vector<short> > : public vector_into_type
{
public:
    into_type(std::vector<short> &v) : vector_into_type(&v, eXShort) {}
    into_type(std::vector<short> &v, std::vector<eIndicator> &ind)
        : vector_into_type(&v, eXShort, ind) {}
};

template <>
class into_type<int> : public standard_into_type
{
public:
    into_type(int &i) : standard_into_type(&i, eXInteger) {}
    into_type(int &i, eIndicator &ind)
        : standard_into_type(&i, eXInteger, ind) {}
};

template <>
class into_type<std::vector<int> > : public vector_into_type
{
public:
    into_type(std::vector<int> &v) : vector_into_type(&v, eXInteger) {}
    into_type(std::vector<int> &v, std::vector<eIndicator> &ind)
        : vector_into_type(&v, eXInteger, ind) {}
};

template <>
class into_type<char> : public standard_into_type
{
public:
    into_type(char &c) : standard_into_type(&c, eXChar) {}
    into_type(char &c, eIndicator &ind)
        : standard_into_type(&c, eXChar, ind) {}
};

template <>
class into_type<std::vector<char> >: public vector_into_type
{
public:
    into_type(std::vector<char> &v) : vector_into_type(&v, eXChar) {}
    into_type(std::vector<char> &v, std::vector<eIndicator> &vind)
        : vector_into_type(&v, eXChar, vind) {}
};

template <>
class into_type<unsigned long> : public standard_into_type
{
public:
    into_type(unsigned long &ul) : standard_into_type(&ul, eXUnsignedLong) {}
    into_type(unsigned long &ul, eIndicator &ind)
        : standard_into_type(&ul, eXUnsignedLong, ind) {}
};

template <>
class into_type<std::vector<unsigned long> > : public vector_into_type
{
public:
    into_type(std::vector<unsigned long> &v)
        : vector_into_type(&v, eXUnsignedLong) {}
    into_type(std::vector<unsigned long> &v, std::vector<eIndicator> &vind)
        : vector_into_type(&v, eXUnsignedLong, vind) {}
};

template <>
class into_type<double> : public standard_into_type
{
public:
    into_type(double &d) : standard_into_type(&d, eXDouble) {}
    into_type(double &d, eIndicator &ind)
        : standard_into_type(&d, eXDouble, ind) {}
};

template <>
class into_type<std::vector<double> > : public vector_into_type
{
public:
    into_type(std::vector<double> &v)
        : vector_into_type(&v, eXDouble) {}
    into_type(std::vector<double> &v, std::vector<eIndicator> &vind)
        : vector_into_type(&v, eXDouble, vind) {}
};

template <>
class into_type<char*> : public standard_into_type
{
public:
    into_type(char *str, std::size_t bufSize)
        : standard_into_type(&str_, eXCString), str_(str, bufSize) {}
    into_type(char *str, eIndicator &ind, std::size_t bufSize)
        : standard_into_type(&str_, eXCString, ind), str_(str, bufSize) {}

private:
    cstring_descriptor str_;
};


// into types for char arrays (with size known at compile-time)

template <std::size_t N>
class into_type<char[N]> : public into_type<char*>
{
public:
    into_type(char str[]) : into_type<char*>(str, N) {}
    into_type(char str[], eIndicator &ind) : into_type<char*>(str, ind, N) {}
};

template <>
class into_type<std::string> : public standard_into_type
{
public:
    into_type(std::string &s) : standard_into_type(&s, eXStdString) {}
    into_type(std::string &s, eIndicator &ind)
        : standard_into_type(&s, eXStdString, ind) {}
};

template <>
class into_type<std::vector<std::string> > : public vector_into_type
{
public:
    into_type(std::vector<std::string>& v)
        : vector_into_type(&v, eXStdString) {}
    into_type(std::vector<std::string>& v, std::vector<eIndicator> &vind)
        : vector_into_type(&v, eXStdString, vind) {}
};

template <>
class into_type<std::tm> : public standard_into_type
{
public:
    into_type(std::tm &t) : standard_into_type(&t, eXStdTm) {}
    into_type(std::tm &t, eIndicator &ind)
        : standard_into_type(&t, eXStdTm, ind) {}
};

template <>
class into_type<std::vector<std::tm> > : public vector_into_type
{
public:
    into_type(std::vector<std::tm>& v) : vector_into_type(&v, eXStdTm) {}
    into_type(std::vector<std::tm>& v, std::vector<eIndicator> &vind)
        : vector_into_type(&v, eXStdTm, vind) {}
};

} // namespace details

// the into function is a helper for defining output variables

template <typename T>
details::into_type_ptr into(T &t)
{
    return details::into_type_ptr(new details::into_type<T>(t));
}

template <typename T, typename T1>
details::into_type_ptr into(T &t, T1 p1)
{
    return details::into_type_ptr(new details::into_type<T>(t, p1));
}

template <typename T>
details::into_type_ptr into(T &t, std::vector<eIndicator> &indicator)
{
    return details::into_type_ptr(new details::into_type<T>(t, indicator));
}

template <typename T>
details::into_type_ptr into(T &t, eIndicator &indicator)
{
    return details::into_type_ptr(new details::into_type<T>(t, indicator));
}

} // namespace SOCI

#endif
