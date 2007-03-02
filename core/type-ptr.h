//
// Copyright (C) 2004-2007 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_PTR_H_INCLUDED
#define SOCI_TYPE_PTR_H_INCLUDED

namespace soci
{
namespace details
{
class into_type_base;
class use_type_base;

template <typename T>
class type_ptr
{
public:
    type_ptr(T *p) : p_(p) {}
    ~type_ptr() { delete p_; }

    T * get() const { return p_; }
    void release() const { p_ = NULL; }

private:
    mutable T *p_;
};

typedef type_ptr<into_type_base> into_type_ptr;
typedef type_ptr<use_type_base> use_type_ptr;

} // namespace details
} // namespace soci

#endif
