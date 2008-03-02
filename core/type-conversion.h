//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_CONVERSION_H_INCLUDED
#define SOCI_TYPE_CONVERSION_H_INCLUDED

#include "type-conversion-traits.h"
#include "into-type.h"
#include "use-type.h"

#include <vector>

namespace soci
{

namespace details
{

// this class is used to ensure correct order of construction
// of into_type and use_type elements that use type_conversion

template <typename T>
struct base_value_holder
{
    typename type_conversion<T>::base_type val_;
};

// Automatically create into_type from a type_conversion

template <typename T>
class conversion_into_type
    : private base_value_holder<T>,
      public into_type<typename type_conversion<T>::base_type>
{
public:
    typedef typename type_conversion<T>::base_type BASE_TYPE;

    conversion_into_type(T &value)
        : into_type<BASE_TYPE>(details::base_value_holder<T>::val_, ownInd_),
          value_(value), ind_(ownInd_) {}
    conversion_into_type(T &value, eIndicator &ind)
        : into_type<BASE_TYPE>(details::base_value_holder<T>::val_, ind),
          value_(value), ind_(ind) {}

private:
    void convert_from_base()
    {
        type_conversion<T>::from_base(
            details::base_value_holder<T>::val_, ind_, value_);
    }

    T &value_;

    eIndicator ownInd_;

    // ind_ refers to either ownInd_, or the one provided by the user
    // in any case, ind_ refers to some valid indicator
    // and can be used by conversion routines
    eIndicator &ind_;
};

// Automatically create use_type from a type_conversion

template <typename T>
class conversion_use_type
    : private details::base_value_holder<T>,
      public use_type<typename type_conversion<T>::base_type>
{
public:
    typedef typename type_conversion<T>::base_type BASE_TYPE;

    conversion_use_type(T &value, std::string const &name = std::string())
        : use_type<BASE_TYPE>(details::base_value_holder<T>::val_,
            ownInd_, name),
        value_(value), ind_(ownInd_), readOnly_(false) {}
    conversion_use_type(const T &value, std::string const &name = std::string())
        : use_type<BASE_TYPE>(details::base_value_holder<T>::val_,
            ownInd_, name),
        value_(const_cast<T &>(value)), ind_(ownInd_), readOnly_(true) {}
    conversion_use_type(T &value, eIndicator &ind, std::string const &name
            = std::string())
        : use_type<BASE_TYPE>(details::base_value_holder<T>::val_, ind, name),
        value_(value), ind_(ind), readOnly_(false) {}
    conversion_use_type(const T &value, eIndicator &ind, std::string const &name
            = std::string())
        : use_type<BASE_TYPE>(details::base_value_holder<T>::val_, ind, name),
        value_(const_cast<T &>(value)), ind_(ind), readOnly_(true) {}

    void convert_from_base()
    {
        if (readOnly_ == false)
        {
            type_conversion<T>::from_base(
                details::base_value_holder<T>::val_, ind_, value_);
        }
    }

    void convert_to_base()
    {
        type_conversion<T>::to_base(value_,
            details::base_value_holder<T>::val_, ind_);
    }

private:
    T &value_;

    eIndicator ownInd_;

    // ind_ refers to either ownInd_, or the one provided by the user
    // in any case, ind_ refers to some valid indicator
    // and can be used by conversion routines
    eIndicator &ind_;

    bool readOnly_;
};

// this class is used to ensure correct order of construction
// of vector based into_type and use_type elements that use type_conversion

template <typename T>
struct base_vector_holder
{
    base_vector_holder(std::size_t sz = 0) : vec_(sz) {}
    mutable std::vector<typename type_conversion<T>::base_type> vec_;
};

// Automatically create a std::vector based into_type from a type_conversion

template <typename T>
class conversion_into_type<std::vector<T> >
    : private details::base_vector_holder<T>,
      public into_type<std::vector<typename type_conversion<T>::base_type> >
{
public:
    typedef typename std::vector<typename type_conversion<T>::base_type>
        BASE_TYPE;

    conversion_into_type(std::vector<T> &value)
        : details::base_vector_holder<T>(value.size()),
          into_type<BASE_TYPE>(details::base_vector_holder<T>::vec_, ownInd_),
          value_(value), ind_(ownInd_) {}

    conversion_into_type(std::vector<T> &value, std::vector<eIndicator> &ind)
        : details::base_vector_holder<T>(value.size()),
          into_type<BASE_TYPE>(details::base_vector_holder<T>::vec_, ind),
          value_(value), ind_(ind) {}

    virtual std::size_t size() const
    {
        // the user might have resized his vector in the meantime
        // -> synchronize the base-value mirror to have the same size

        std::size_t const userSize = value_.size();
        details::base_vector_holder<T>::vec_.resize(userSize);
        return userSize;
    }

    virtual void resize(std::size_t sz)
    {
        value_.resize(sz);
        ind_.resize(sz);
        details::base_vector_holder<T>::vec_.resize(sz);
    }

private:
    void convert_from_base()
    {
        std::size_t const sz = details::base_vector_holder<T>::vec_.size();

        for (std::size_t i = 0; i != sz; ++i)
        {
            type_conversion<T>::from_base(
                details::base_vector_holder<T>::vec_[i], ind_[i], value_[i]);
        }
    }

    std::vector<T> &value_;

    std::vector<eIndicator> ownInd_;

    // ind_ refers to either ownInd_, or the one provided by the user
    // in any case, ind_ refers to some valid vector of indicators
    // and can be used by conversion routines
    std::vector<eIndicator> &ind_;
};


// Automatically create a std::vector based use_type from a type_conversion

template <typename T>
class conversion_use_type<std::vector<T> >
     : private details::base_vector_holder<T>,
       public use_type<std::vector<typename type_conversion<T>::base_type> >
{
public:
    typedef typename std::vector<typename type_conversion<T>::base_type>
        BASE_TYPE;

    conversion_use_type(std::vector<T> &value,
        std::string const &name=std::string())
        : details::base_vector_holder<T>(value.size()),
          use_type<BASE_TYPE>(details::base_vector_holder<T>::vec_,
              ownInd_, name),
          value_(value), ind_(ownInd_) {}

    conversion_use_type(std::vector<T> &value,
        std::vector<eIndicator> const &ind,
        std::string const &name=std::string())
        : details::base_vector_holder<T>(value.size()),
          use_type<BASE_TYPE>(details::base_vector_holder<T>::vec_,
              ind, name),
          value_(value), ind_(ind) {}

private:
    void convert_from_base()
    {
        std::size_t const sz = details::base_vector_holder<T>::vec_.size();
        value_.resize(sz);
        ind_.resize(sz);
        for (std::size_t i = 0; i != sz; ++i)
        {
            type_conversion<T>::from_base(
                details::base_vector_holder<T>::vec_[i], value_[i], ind_[i]);
        }
    }

    void convert_to_base()
    {
        std::size_t const sz = value_.size();
        details::base_vector_holder<T>::vec_.resize(sz);
        ind_.resize(sz);
        for (std::size_t i = 0; i != sz; ++i)
        {
            type_conversion<T>::to_base(value_[i],
                details::base_vector_holder<T>::vec_[i], ind_[i]);
        }
    }

    std::vector<T> &value_;

    std::vector<eIndicator> ownInd_;

    // ind_ refers to either ownInd_, or the one provided by the user
    // in any case, ind_ refers to some valid vector of indicators
    // and can be used by conversion routines
    std::vector<eIndicator> &ind_;
};

template <typename T>
into_type_ptr do_into(T &t, user_type_tag)
{
    return into_type_ptr(new conversion_into_type<T>(t));
}

template <typename T>
into_type_ptr do_into(T &t, eIndicator &indicator, user_type_tag)
{
    return into_type_ptr(new conversion_into_type<T>(t, indicator));
}

template <typename T>
use_type_ptr do_use(T &t, std::string const &name, user_type_tag)
{
    return use_type_ptr(new conversion_use_type<T>(t, name));
}

template <typename T>
use_type_ptr do_use(const T &t, std::string const &name, user_type_tag)
{
    return use_type_ptr(new conversion_use_type<T>(t, name));
}

template <typename T>
use_type_ptr do_use(T &t, eIndicator &indicator,
    std::string const &name, user_type_tag)
{
    return use_type_ptr(new conversion_use_type<T>(t, indicator, name));
}

template <typename T>
use_type_ptr do_use(const T &t, eIndicator &indicator,
    std::string const &name, user_type_tag)
{
    return use_type_ptr(new conversion_use_type<T>(t, indicator, name));
}

// type_conversion specializations must use a stock type as the base_type.
// Each such specialization automatically creates a use_type and an into_type.
// This code is commented out, since it causes problems in those environments
// where std::time_t is an alias to int.
//
// template<>
// struct type_conversion<std::time_t>
// {
//     typedef std::tm base_type;
//
//     static void from_base(base_type const &in, eIndicator ind, std::time_t &out)
//     {
//         if (ind == eNull)
//         {
//             throw soci_error("Null value not allowed for this type");
//         }
//         out = mktime(&in);
//     }
//
//     static void to_base(T const &in, base_type &out, eIndicator &ind)
//     {
//         out = *localtime(&in);
//         ind = eOK;
//     }
// };

} // namespace details

} // namespace soci

#endif
