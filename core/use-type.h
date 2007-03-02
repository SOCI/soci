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

private:
    virtual void pre_use();
    virtual void post_use(bool gotData);
    virtual void clean_up();
    virtual std::size_t size() const { return 1; }

    void *data_;
    eExchangeType type_;
    eIndicator *ind_;
    std::string name_;

    standard_use_type_backend *backEnd_;
};

// general case not implemented
template <typename T>
class use_type;


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

template <>
class use_type<short> : public standard_use_type
{
public:
    use_type(short &s, std::string const &name = std::string())
        : standard_use_type(&s, eXShort, name) {}
    use_type(short &s, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&s, eXShort, ind, name) {}
};

template <>
class use_type<std::vector<short> > : public vector_use_type
{
public:
    use_type(std::vector<short> &v, std::string const &name = std::string())
        : vector_use_type(&v, eXShort, name) {}
    use_type(std::vector<short> &v, std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXShort, ind, name) {}
};

template <>
class use_type<int> : public standard_use_type
{
public:
    use_type(int &i, std::string const &name = std::string())
        : standard_use_type(&i, eXInteger, name) {}
    use_type(int &i, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&i, eXInteger, ind, name) {}
};

template <>
class use_type<std::vector<int> > : public vector_use_type
{
public:
    use_type(std::vector<int> &v, std::string const &name = std::string())
        : vector_use_type(&v, eXInteger, name) {}
    use_type(std::vector<int> &v, std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXInteger, ind, name) {}
};

template <>
class use_type<char> : public standard_use_type
{
public:
    use_type(char &c, std::string const &name = std::string())
        : standard_use_type(&c, eXChar, name) {}
    use_type(char &c, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&c, eXChar, ind, name) {}
};

template <>
class use_type<std::vector<char> >: public vector_use_type
{
public:
    use_type(std::vector<char> &v, std::string const &name = std::string())
        : vector_use_type(&v, eXChar, name) {}
    use_type(std::vector<char> &v, std::vector<eIndicator> const &vind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXChar, vind, name) {}
};

template <>
class use_type<unsigned long> : public standard_use_type
{
public:
    use_type(unsigned long &ul, std::string const &name = std::string())
        : standard_use_type(&ul, eXUnsignedLong, name) {}
    use_type(unsigned long &ul, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&ul, eXUnsignedLong, ind, name) {}
};

template <>
class use_type<std::vector<unsigned long> > : public vector_use_type
{
public:
    use_type(std::vector<unsigned long> &v,
        std::string const &name = std::string())
        : vector_use_type(&v, eXUnsignedLong, name) {}
    use_type(std::vector<unsigned long> &v,
        std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXUnsignedLong, ind, name) {}
};

template <>
class use_type<double> : public standard_use_type
{
public:
    use_type(double &d, std::string const &name = std::string())
        : standard_use_type(&d, eXDouble, name) {}
    use_type(double &d, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&d, eXDouble, ind, name) {}
};

template <>
class use_type<std::vector<double> > : public vector_use_type
{
public:
    use_type(std::vector<double> &v, std::string const &name = std::string())
        : vector_use_type(&v, eXDouble, name) {}
    use_type(std::vector<double> &v, std::vector<eIndicator> const &ind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXDouble, ind, name) {}
};

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

template <>
class use_type<std::string> : public standard_use_type
{
public:
    use_type(std::string &s, std::string const &name = std::string())
        : standard_use_type(&s, eXStdString, name) {}
    use_type(std::string &s, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&s, eXStdString, ind, name) {}
};

template <>
class use_type<std::vector<std::string> > : public vector_use_type
{
public:
    use_type(std::vector<std::string>& v,
        std::string const &name = std::string())
        : vector_use_type(&v, eXStdString, name) {}
    use_type(std::vector<std::string>& v, std::vector<eIndicator> const &ind,
            std::string const &name = std::string())
        : vector_use_type(&v, eXStdString, ind, name) {}
};

template <>
class use_type<std::tm> : public standard_use_type
{
public:
    use_type(std::tm &t, std::string const &name = std::string())
        : standard_use_type(&t, eXStdTm, name) {}
    use_type(std::tm &t, eIndicator &ind,
        std::string const &name = std::string())
        : standard_use_type(&t, eXStdTm, ind, name) {}
};

template <>
class use_type<std::vector<std::tm> > : public vector_use_type
{
public:
    use_type(std::vector<std::tm>& v, std::string const &name = std::string())
        : vector_use_type(&v, eXStdTm, name) {}
    use_type(std::vector<std::tm>& v, std::vector<eIndicator> const &vind,
        std::string const &name = std::string())
        : vector_use_type(&v, eXStdTm, vind, name) {}
};

} // namespace details

// the use function is a helper for binding input variables
// (and output PL/SQL parameters)

template <typename T>
details::use_type_ptr use(T &t)
{
    return details::use_type_ptr(new details::use_type<T>(t));
}

template <typename T, typename T1>
details::use_type_ptr use(T &t, T1 p1)
{
    return details::use_type_ptr(new details::use_type<T>(t, p1));
}

template <typename T>
details::use_type_ptr use(T &t, std::vector<eIndicator> const &indicator)
{
    return details::use_type_ptr(new details::use_type<T>(t, indicator));
}

template <typename T>
details::use_type_ptr use(T &t, eIndicator &indicator)
{
    return details::use_type_ptr(new details::use_type<T>(t, indicator));
}

} // namesapce soci

#endif

