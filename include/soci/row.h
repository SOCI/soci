//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ROW_H_INCLUDED
#define SOCI_ROW_H_INCLUDED

#include "soci/type-traits.h"
#include "soci/soci-backend.h"
#include "soci/type-conversion.h"
// std
#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <limits>
#include <typeinfo>

namespace soci
{
namespace details
{
class statement_impl;
class data_holder;
}

class SOCI_DECL column_properties
{
    // use getters/setters in case we want to make some
    // of the getters lazy in the future
public:

    std::string get_name() const { return name_; }
    data_type get_data_type() const { return dataType_; }

    void set_name(std::string const& name) { name_ = name; }
    void set_data_type(data_type dataType)  { dataType_ = dataType; }

private:
    std::string name_;
    data_type dataType_;
};

class SOCI_DECL row
{
public:
    friend class details::statement_impl;

public:
    row();
    ~row();

    void uppercase_column_names(bool forceToUpper);
    void add_properties(column_properties const& cp);
    std::size_t size() const;
    void clean_up();

    indicator get_indicator(std::size_t pos) const;
    indicator get_indicator(std::string const& name) const;

    column_properties const& get_properties(std::size_t pos) const;
    column_properties const& get_properties(std::string const& name) const;

    template <typename T>
    T get(std::size_t pos) const
    {
        typedef typename type_conversion<T>::base_type base_type;
        base_type baseVal;
        do_get(pos, baseVal);

        T ret;
        type_conversion<T>::from_base(baseVal, get_indicator(pos), ret);
        return ret;
    }

    template <typename T>
    T get(std::size_t pos, T const &nullValue) const
    {
        if (i_null == get_indicator(pos))
        {
            return nullValue;
        }

        return get<T>(pos);
    }

    template <typename T>
    T get(std::string const &name) const
    {
        std::size_t const pos = find_column(name);
        return get<T>(pos);
    }

    template <typename T>
    T get(std::string const &name, T const &nullValue) const
    {
        std::size_t const pos = find_column(name);

        if (i_null == get_indicator(pos))
        {
            return nullValue;
        }

        return get<T>(pos);
    }

    template <typename T>
    row const& operator>>(T& value) const
    {
        value = get<T>(currentPos_);
        ++currentPos_;
        return *this;
    }

    void skip(std::size_t num = 1) const
    {
        currentPos_ += num;
    }

    void reset_get_counter() const
    {
        currentPos_ = 0;
    }

private:
    SOCI_NOT_COPYABLE(row)

    std::size_t find_column(std::string const& name) const;

    std::pair<std::string*, indicator*> alloc_data_holder_string();
    std::pair<std::tm*, indicator*> alloc_data_holder_tm();
    std::pair<double*, indicator*> alloc_data_holder_double();
    std::pair<int*, indicator*> alloc_data_holder_int();
    std::pair<long long*, indicator*> alloc_data_holder_llong();
    std::pair<unsigned long long*, indicator*> alloc_data_holder_ullong();

    data_type get_data_holder_type(std::size_t pos) const SOCI_NOEXCEPT;
    double get_data_holder_double(std::size_t pos) const SOCI_NOEXCEPT;
    int get_data_holder_int(std::size_t pos) const SOCI_NOEXCEPT;
    long long get_data_holder_llong(std::size_t pos) const SOCI_NOEXCEPT;
    unsigned long long get_data_holder_ullong(std::size_t pos) const SOCI_NOEXCEPT;

    template<typename Dst>
    struct numeric_cast_t
    {
        template<typename Src>
        typename soci::enable_if<soci::is_same<Dst, Src>::value, Dst>::type
        operator()(Src src) const
        {
            return src;
        }
        template<typename Src>
        typename soci::enable_if<!soci::is_same<Dst, Src>::value, Dst>::type
        operator()(Src) const
        {
            throw std::bad_cast();
        }
    };

    void do_get(std::size_t pos, std::string &baseVal) const;
    void do_get(std::size_t pos, std::tm &baseVal) const;
    template <typename T>
    typename soci::enable_if<std::numeric_limits<T>::is_specialized, void>::type
    do_get(std::size_t pos, T &baseVal) const
    {
        return get_number(pos, baseVal, numeric_cast_t<T>());
    }
    template <typename T, typename NumericCast>
    void get_number(std::size_t pos, T &baseVal, const NumericCast &cast) const
    {
        switch (get_data_holder_type(pos))
        {
            case dt_double:
                baseVal = cast(get_data_holder_double(pos));
                break;
            case dt_integer:
                baseVal = cast(get_data_holder_int(pos));
                break;
            case dt_long_long:
                baseVal = cast(get_data_holder_llong(pos));
                break;
            case dt_unsigned_long_long:
                baseVal = cast(get_data_holder_ullong(pos));
                break;
            default:
                throw std::bad_cast();
        }
    }

    std::vector<column_properties> columns_;
    std::vector<details::data_holder *> holders_;
    std::map<std::string, std::size_t> index_;

    bool uppercaseColumnNames_;
    mutable std::size_t currentPos_;
};

} // namespace soci

#endif // SOCI_ROW_H_INCLUDED
