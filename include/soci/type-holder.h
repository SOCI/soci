//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_HOLDER_H_INCLUDED
#define SOCI_TYPE_HOLDER_H_INCLUDED

#include "soci/soci-platform.h"
// std
#include <cstring>
#include <typeinfo>

namespace soci
{

namespace details
{

// Base class holder + derived class type_holder for storing type data
// instances in a container of holder objects
template <typename T>
class type_holder;

class holder
{
public:
    holder() {}
    virtual ~holder() {}

    template<typename T>
    T get()
    {
        // Using dynamic_cast<> doesn't work when using libc++ and ELF
        // visibility together. Luckily, here we don't need the full power of
        // the cast anyhow, as we only need to check if the types are the same,
        // which can be done by comparing their type info objects: check if
        // they're identical first, as an optimization, and then comparing
        // their names to make it actually work with libc++.
        std::type_info const& ti_this = typeid(*this);
        std::type_info const& ti_type = typeid(type_holder<T>);
        if (&ti_this == &ti_type
                || std::strcmp(ti_this.name(), ti_type.name()) == 0)
        {
            type_holder<T>* p = static_cast<type_holder<T> *>(this);
            return p->template value<T>();
        }
        else
        {
            throw std::bad_cast();
        }
    }

private:

    template<typename T>
    T value();
};

template <typename T>
class type_holder : public holder
{
public:
    type_holder(T * t) : t_(t) {}
    ~type_holder() SOCI_OVERRIDE { delete t_; }

    template<typename TypeValue>
    TypeValue value() const { return *t_; }

private:
    T * t_;
};

} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
