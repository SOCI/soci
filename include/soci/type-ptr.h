//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_TYPE_PTR_H_INCLUDED
#define SOCI_TYPE_PTR_H_INCLUDED

#include <algorithm>

namespace soci { namespace details {

template <typename T>
class type_ptr
{
public:
    type_ptr(T * p) : p_(p) {}
    ~type_ptr() { delete p_; }

    T * get() const { return p_; }
    T * release() const
    {
        T * tmp = 0;
        std::swap(tmp, p_);
        return tmp;
    }

    // rhs will loose it ownership
    type_ptr& operator=(const type_ptr& rhs)
    {
        // move sematics
        std::swap(this->p_, rhs.p_);
        if (rhs.p_)
        {
            delete rhs.p_;
            rhs.p_ = 0;
        }
    }

    // rhs will loose it ownership
    type_ptr(const type_ptr& rhs)
        : p_(rhs.p_)
    {
        if (rhs.p_)
        {
            // avoid double free
            rhs.p_ = 0;
        }
    }

private:

    mutable T * p_;
};

} // namespace details
} // namespace soci

#endif // SOCI_TYPE_PTR_H_INCLUDED
