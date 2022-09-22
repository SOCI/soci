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

// Returns U* as T*, if the dynamic type of the pointer is really T.
//
// This should be used instead of dynamic_cast<> because using it doesn't work
// when using libc++ and ELF visibility together. Luckily, when we don't need
// the full power of the cast, but only need to check if the types are the
// same, it can be done by comparing their type info objects.
//
// This function does _not_ replace dynamic_cast<> in all cases and notably
// doesn't allow the input pointer to be null.
template <typename T, typename U>
T* checked_ptr_cast(U* ptr)
{
    // Check if they're identical first, as an optimization, and then compare
    // their names to make it actually work with libc++.
    std::type_info const& ti_ptr = typeid(*ptr);
    std::type_info const& ti_ret = typeid(T);

    if (&ti_ptr != &ti_ret && std::strcmp(ti_ptr.name(), ti_ret.name()) != 0)
    {
        return NULL;
    }

    return static_cast<T*>(ptr);
}

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
        type_holder<T>* p = checked_ptr_cast<type_holder<T> >(this);
        if (p)
        {
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
    ~type_holder() override { delete t_; }

    template<typename TypeValue>
    TypeValue value() const { return *t_; }

private:
    T * t_;
};

} // namespace details

} // namespace soci

#endif // SOCI_TYPE_HOLDER_H_INCLUDED
