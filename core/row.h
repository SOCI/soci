//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_ROW_H_INCLUDED
#define SOCI_ROW_H_INCLUDED

#include "type-holder.h"
#include "soci-backend.h"
#include "type-conversion.h"

#include <cassert>
#include <string>
#include <vector>
#include <map>

namespace soci
{

class SOCI_DECL column_properties
{
    // use getters/setters in case we want to make some
    // of the getters lazy in the future
public:

    std::string get_name() const   { return name_; }
    eDataType get_data_type() const { return dataType_; }

    void set_name(std::string const &name) { name_ = name; }
    void set_data_type(eDataType dataType)  { dataType_ = dataType; }

private:
    std::string name_;
    eDataType dataType_;
};

class SOCI_DECL row
{
public:
//TODO! friend class column_properites
// private
    void add_properties(column_properties const &cp);
    std::size_t size() const;

    eIndicator indicator(std::size_t pos) const;
    eIndicator indicator(std::string const &name) const;

    template <typename T>
    inline void add_holder(T* t, eIndicator* ind)
    {
        holders_.push_back(new details::type_holder<T>(t));
        indicators_.push_back(ind);
    }

    column_properties const & get_properties (std::size_t pos) const;
    column_properties const & get_properties (std::string const &name) const;

    template <typename T>
    T get(std::size_t pos) const
    {
        typedef typename type_conversion<T>::base_type BASE_TYPE;

        assert(holders_.size() >= pos + 1);

        const BASE_TYPE &baseVal = holders_[pos]->get<BASE_TYPE>();

        T ret;

        type_conversion<T>::from_base(baseVal, *indicators_[pos], ret);

        return ret;
    }

    template <typename T>
    T get(std::size_t pos, T const &nullValue) const
    {
        assert(holders_.size() >= pos + 1);

        if (eNull == *indicators_[pos])
        {
            return nullValue;
        }

        return get<T>(pos);
    }

    template <typename T>
    T get(std::string const &name) const
    {
        std::size_t pos = find_column(name);

        return get<T>(pos);
    }

    template <typename T>
    T get(std::string const &name, T const &nullValue) const
    {
        std::size_t pos = find_column(name);

        if (eNull == *indicators_[pos])
        {
            return nullValue;
        }

        return get<T>(pos);
    }

    template <typename T>
    row const & operator>>(T &value) const
    {
        value = get<T>(currentPos_);
        ++currentPos_;
        return *this;
    }

    void skip(size_t num = 1) const
    {
        currentPos_ += num;
    }

    void reset_get_counter() const
    {
        currentPos_ = 0;
    }

    row() : currentPos_(0) {}
    ~row();

private:
    // copy not supported
    row(row const &);
    row operator=(row const &);

    std::size_t find_column(std::string const &name) const;

    std::vector<column_properties> columns_;
    std::vector<details::holder*> holders_;
    std::vector<eIndicator*> indicators_;
    std::map<std::string, std::size_t> index_;

    mutable std::size_t currentPos_;
};

} // namespace soci

#endif
